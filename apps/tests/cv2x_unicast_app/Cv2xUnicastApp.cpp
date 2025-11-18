/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xUnicastApp.cpp
 *
 * @brief: Simple application that demonstrates Tx/Rx Unicast in Cv2x
 */

#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <ifaddrs.h>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <atomic>
#include <telux/cv2x/Cv2xRadio.hpp>

#include "../../common/utils/Utils.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::future;
using std::promise;
using std::shared_ptr;
using std::atomic;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::TrafficCategory;
using telux::cv2x::SpsFlowInfo;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::ICv2xRxSubscription;
using telux::cv2x::TrafficIpType;
using telux::cv2x::ICv2xRadioListener;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadioManager;

static constexpr uint32_t BROADCAST_SERVICE_ID = 1u;
static constexpr uint32_t UNICAST_SERVICE_ID = 10u;  // differ from broadcast SID
static constexpr uint16_t DEFAULT_BROADCAST_PORT = 5000u;
static constexpr uint16_t DEFUALT_UNICAST_PORT = 6000u; // differ from broadcast port
static constexpr uint32_t BROADCAST_PACKET_LEN = 128u;
static constexpr uint32_t UNICAST_PACKET_LEN = 256u;
static constexpr uint32_t DUMMY_PACKET_LEN = 10000; //cv2x msg buffer length
static constexpr uint32_t SUCCESSIVE_MISSED_ECHO_NUM = 6;
static constexpr char TEST_VERNO_MAGIC = 'U'; // magic word for WSA and unicast msgs
static constexpr int PRIORITY = 3; // WSA priority
static constexpr char RSU_ID = 1;
static constexpr char OBU_ID = 2;

static std::mutex gCv2xStatusMutex;
static Cv2xStatus gCv2xStatus;
static condition_variable gStatusCv;
static promise<ErrorCode> gCallbackPromise;
static uint32_t gTxUnicastCount = 0;
static uint32_t gRxUnicastCount = 0;
static uint32_t gMissedTxUnicastCount = 0;
static uint32_t gMissedRxUnicastCount = 0;
static struct sockaddr_in6 gRsuAddr = { 0 };
static bool gWsaReceived = false;
static uint32_t gUnicastPktLen = UNICAST_PACKET_LEN;
static uint32_t gSuccessiveMissedEcho = 0;
static char gUnicastBuf[DUMMY_PACKET_LEN];
static char gBroadcastBuf[BROADCAST_PACKET_LEN];
static uint32_t gBroadcastServiceId = BROADCAST_SERVICE_ID;
static uint16_t gBroadcastPort = DEFAULT_BROADCAST_PORT;
static shared_ptr<ICv2xTxFlow> gTxBroadcastFlow = nullptr;
static shared_ptr<ICv2xRxSubscription> gRxBroadcastFlow = nullptr;
static shared_ptr<ICv2xTxFlow> gTxUnicastFlow = nullptr;
static shared_ptr<ICv2xRxSubscription> gRxUnicastFlow = nullptr;
static shared_ptr<ICv2xRadioManager> gCv2xRadioMgr = nullptr;
static shared_ptr<ICv2xRadio> gCv2xRadio = nullptr;
static shared_ptr<ICv2xRadioListener> gRadioListener = nullptr;
static shared_ptr<ICv2xListener> gStatusListener = nullptr;
static bool gWsaThreadValid = false;
static future<void> gWsaThread;
static future<void> gTerminateThread;
static atomic<int> gTerminate{0};
static int gTerminatePipe[2];
static mutex gOperationMutex;

// Two operation modes are supported by this app.
// If in OBU mode, after receiving broadcast WSA msg from RSU, it starts transmitting
// unicast msgs and waits for the unicast echo msgs from RSU.
// If in RSU mode, it transmits broadcast WSA msgs periodically and sends unicast echo
// msg to OBU for each received unicast msg.
enum class UnicastTestMode {
    OBU,
    RSU
};

static UnicastTestMode gUnicastTestMode;

class RadioListener : public ICv2xRadioListener {
public:
    void onL2AddrChanged(uint32_t newL2Address) {
        cout << "Src L2 Addr updated to:" << newL2Address << endl;
    }
};

class Cv2xStatusListener : public ICv2xListener {
public:
    void onStatusChanged(Cv2xStatus status) override {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        if (status.rxStatus != gCv2xStatus.rxStatus
            or status.txStatus != gCv2xStatus.txStatus) {
            cout << "cv2x status changed, Tx: " << static_cast<int>(status.txStatus);
            cout << ", Rx: " << static_cast<int>(status.rxStatus) << endl;
            gCv2xStatus = status;

            if (status.rxStatus == Cv2xStatusType::ACTIVE and
                status.txStatus == Cv2xStatusType::ACTIVE) {
                gStatusCv.notify_all();
            }
        }
    }
};

static bool isV2xReady() {
    lock_guard<mutex> lock(gCv2xStatusMutex);
    if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus and
        Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
        return true;
    }

    cout << "cv2x Tx/Rx not active!" << endl;
    return false;
}

static void waitV2xStatusActive() {
    std::unique_lock<std::mutex> cvLock(gCv2xStatusMutex);
    while (!gTerminate and
           (Cv2xStatusType::ACTIVE != gCv2xStatus.rxStatus or
            Cv2xStatusType::ACTIVE != gCv2xStatus.txStatus)) {
        cout << "wait for Cv2x status active." << endl;
        gStatusCv.wait(cvLock);
    }
}

// Resets the global callback promise
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback function for ICv2xRadioManager->requestCv2xStatus()
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        gCv2xStatus = status;
    }
    gCallbackPromise.set_value(error);
}

// Returns current timestamp
static uint64_t getCurrentTimestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ull + (uint64_t)tv.tv_usec;
}

struct cv2x_common_message_t {
    char familyId;
    char ueId;
    uint16_t seqNum;
    uint64_t timestamp;
};

struct cv2x_message_t {
    struct cv2x_common_message_t contents;
    uint32_t length;
    char *buffer;
};

// Fills buffer
void createBuffer(cv2x_message_t &cv2xMsg) {
    static uint16_t seq = 0u;
    uint64_t timestamp = getCurrentTimestamp();
    cv2xMsg = { 0 };

    if (gUnicastTestMode == UnicastTestMode::OBU) {
        cv2xMsg.length = gUnicastPktLen;
        cv2xMsg.buffer = gUnicastBuf;
    } else {
        cv2xMsg.length = BROADCAST_PACKET_LEN;
        cv2xMsg.buffer = gBroadcastBuf;
    }
    memset(cv2xMsg.buffer, 0, cv2xMsg.length);

    // Very first payload is test magic number
    cv2xMsg.contents.familyId = TEST_VERNO_MAGIC;
    cv2xMsg.buffer[0] = cv2xMsg.contents.familyId;

    // Next byte is the UEID value
    if (gUnicastTestMode == UnicastTestMode::OBU) {
        cv2xMsg.contents.ueId = OBU_ID;
    } else {
        cv2xMsg.contents.ueId = RSU_ID;
    }
    cv2xMsg.buffer[1] = cv2xMsg.contents.ueId;

    // Sequence number
    cv2xMsg.contents.seqNum = seq++;
    auto dataPtr = &cv2xMsg.buffer[2];
    *(uint16_t *)dataPtr = htons(cv2xMsg.contents.seqNum);
    dataPtr += sizeof(uint16_t);

    // Timestamp
    cv2xMsg.contents.timestamp = timestamp;
    *(uint64_t *)dataPtr = htobe64(cv2xMsg.contents.timestamp);

    // Dummy payload
    constexpr int NUM_LETTERS = 26;
    auto i = sizeof(struct cv2x_common_message_t);
    for (; i < cv2xMsg.length; ++i) {
        cv2xMsg.buffer[i] = 'a' + (i % NUM_LETTERS);
    }
}

// Function for transmitting data
static int sampleTx(int sock,
                    cv2x_message_t &cv2xMsg,
                    bool isUnicast,
                    struct sockaddr_in6 dstAddr) {
    static uint32_t txCount = 0u;

    cout << "sampleTx(" << sock << ")" << endl;

    struct sockaddr_in6 dest_sockaddr = { 0 };
    if (isUnicast) {
        dest_sockaddr = dstAddr;
    }

    struct msghdr message = { 0 };
    struct iovec iov[1] = { 0 };
    struct cmsghdr *cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    // Send data using sendmsg to provide IPV6_TCLASS per packet
    iov[0].iov_base = cv2xMsg.buffer;
    iov[0].iov_len = cv2xMsg.length;

    message.msg_name = &dest_sockaddr;
    message.msg_namelen = sizeof(dest_sockaddr);
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    // Fill ancillary data
    int priority = PRIORITY;
    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsghp), &priority, sizeof(int));

    // Send data
    auto sendBytes = sendmsg(sock, &message, 0);

    // Check bytes sent
    if (sendBytes <= 0) {
        cerr << "Error occurred sending to sock:" << sock << " err:" << strerror(errno) << endl;
        return -1;
    }

    ++txCount;
    cout << "TX count: " << txCount << " bytes:" << sendBytes << endl;
    return 0;
}

// Function for reading from Rx socket
static int sampleRx(int sock, cv2x_message_t &cv2xMsg, struct sockaddr_in6 &srcAddr) {
    static uint32_t rxCount = 0u;
    struct sockaddr_in6 from;
    socklen_t fromLen = sizeof(from);

    // Attempt to read from socket
    int recvBytes = recvfrom(sock, gUnicastBuf, DUMMY_PACKET_LEN, 0,
                             (struct sockaddr *)&from, &fromLen);

    if (recvBytes <= 0) {
        // EAGAIN and EWOULDBLOCK are possible return values when
        // our Rx socket has timed out. This can be an indication that
        // cv2x status has changed and we need to recheck v2x status before
        // continuing to wait for a message over the socket.
        if (errno != EAGAIN || errno != EWOULDBLOCK) {
            cerr << "Error occurred reading from sock:" << sock;
            cerr << " err:" << strerror(errno) << endl;
            return -1;
        }

        return 0;
    }

    cout << "sampleRx(" << sock << ")" << " bytes:" << recvBytes << endl;

    cv2xMsg.length = recvBytes;
    cv2xMsg.buffer = gUnicastBuf;

    // check the magic number for WSA and unicast msgs
    cv2xMsg.contents.familyId = cv2xMsg.buffer[0];
    if (cv2xMsg.contents.familyId != TEST_VERNO_MAGIC) {
        cout << "Ignore msg with mismatched magic character."<< endl;
        return -1;
    }

    // get ueId
    cv2xMsg.contents.ueId = cv2xMsg.buffer[1];

    // get seq num
    auto dataPtr = &cv2xMsg.buffer[2];
    cv2xMsg.contents.seqNum = ntohs(*(uint16_t *)dataPtr);
    dataPtr += sizeof(uint16_t);

    // get timestamp
    cv2xMsg.contents.timestamp = be64toh(*(uint64_t *)dataPtr);

    srcAddr = from;

    ++rxCount;
    cout << "RX count: " << rxCount << " bytes:" << recvBytes << endl;
    return recvBytes;
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << "\n"
         << "-m<Mode>             0--OBU 1--RSU, default to OBU\n"
         << "-s<Broadcast Service Id>  Broadcast service Id, default to 1\n"
         << "-p<Broadcast Port>  Broadcast port used, default to 5000\n"
         << "-l<Unicast packet length>  default to 256 Bytes" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?m:p:s:l:")) != -1) {
        switch (c) {
        case 'm':
            if (optarg) {
                gUnicastTestMode = static_cast<UnicastTestMode>(atoi(optarg));
                cout << "mode: " << static_cast<int>(gUnicastTestMode) << endl;
            }
            break;
        case 'p':
            if (optarg) {
                gBroadcastPort = atoi(optarg);
                cout << "broadcast port: " << gBroadcastPort << endl;
            }
            break;
        case 's':
            if (optarg) {
                gBroadcastServiceId = atoi(optarg);
                cout << "broadcast service id: " << gBroadcastServiceId << endl;
            }
            break;
        case 'l':
            if (optarg) {
                gUnicastPktLen = atoi(optarg);
                cout << "unicast packet length: " << gUnicastPktLen << endl;
            }
            break;
        case '?':
        default:
            rc = -1;
            printUsage(argv[0]);
            return rc;
        }
    }

    return rc;
}

static void terminationHandler(int signum) {
    cout << "terminate handler" << endl;
    gTerminate = 1;
    write(gTerminatePipe[1], &gTerminate, sizeof(int));

    // notify threads waiting for active status
    gStatusCv.notify_all();
}

static void installSignalHandler() {
    struct sigaction sig_action;

    sig_action.sa_handler = terminationHandler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGHUP, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

static Status registerBroadcastFlows() {
    Status status = Status::SUCCESS;
    bool spsFlowCreated = false;

    if (!gCv2xRadio) {
        cerr << "Invalid radio!" << endl;
        return Status::FAILED;
    }

    if (gUnicastTestMode == UnicastTestMode::OBU) {
        // If in OBU mode, register tx/rx flow to listen to WSA
        cout << "Registering broadcast Tx event Flow" << endl;
        auto createTxEventFlowCallback = [](shared_ptr<ICv2xTxFlow> txEventFlow,
                                            ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                gTxBroadcastFlow = txEventFlow;
            }
            gCallbackPromise.set_value(error);
        };

        // Reset global callback
        resetCallbackPromise();

        // Register non-Ip Tx event flow with broadcast SID and broadcast port
        EventFlowInfo flowInfo;
        status = gCv2xRadio->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                               gBroadcastServiceId,
                                               flowInfo,
                                               gBroadcastPort,
                                               createTxEventFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to create broadcast Tx event flow!" << endl;
            return Status::FAILED;
        }

        cout << "Succeeded in creating broadcast Tx event Flow, sock:";
        cout << gTxBroadcastFlow->getSock();
        cout << " , port:"<< gBroadcastPort << endl;

        cout << "Registering broadcast Rx flow" << endl;

        // Callback function for ICv2xRadio->createRxSubscription()
        auto createRxSubscriptionCallback = [](shared_ptr<ICv2xRxSubscription> rxSub,
                                               ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                gRxBroadcastFlow = rxSub;
            }
            gCallbackPromise.set_value(error);
        };

        // Reset global callback
        resetCallbackPromise();

        // Subscribe to broadcast SID and broadcast port
        auto idList = std::make_shared<std::vector<uint32_t>>(1, gBroadcastServiceId);
        status = gCv2xRadio->createRxSubscription(TrafficIpType::TRAFFIC_NON_IP,
                                                  gBroadcastPort,
                                                  createRxSubscriptionCallback,
                                                  idList);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to create broadcast Rx flow." << endl;
            return Status::FAILED;
        }

        cout << "Succeeded in creating broadcast Rx Flow, sock:";
        cout << gRxBroadcastFlow->getSock();
        cout << " , port:"<< gBroadcastPort << endl;

    } else {
        // If in RSU mode, register tx sps flow for the sending of WSA
        cout << "Registering broadcast Tx SPS Flow" << endl;

        auto createTxSpsFlowCallback = [](shared_ptr<ICv2xTxFlow> txSpsFlow,
                                          shared_ptr<ICv2xTxFlow> txEventFlow,
                                          ErrorCode spsError,
                                          ErrorCode unused) {
            if (ErrorCode::SUCCESS == spsError) {
                gTxBroadcastFlow = txSpsFlow;
            }
            gCallbackPromise.set_value(spsError);
        };

        // Reset global callback
        resetCallbackPromise();

        // Register non-Ip Tx sps flow with broadcast SID and broadcast port
        SpsFlowInfo spsInfo;
        spsInfo.priority = Priority::PRIORITY_2;
        spsInfo.periodicity = Periodicity::PERIODICITY_100MS;
        spsInfo.nbytesReserved = BROADCAST_PACKET_LEN;
        spsInfo.autoRetransEnabledValid = true;
        spsInfo.autoRetransEnabled = true;
        status = gCv2xRadio->createTxSpsFlow(TrafficIpType::TRAFFIC_NON_IP,
                                             gBroadcastServiceId,
                                             spsInfo, gBroadcastPort,
                                             true, gBroadcastPort+1,
                                             createTxSpsFlowCallback);
        if (Status::SUCCESS == status) {
            auto error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS == error) {
                spsFlowCreated = true;
            } else {
                cerr << "Failed to create broadcast Tx SPS Flow." << endl;
                if (ErrorCode::V2X_ERR_EXCEED_MAX != error) {
                    return Status::FAILED;
                }
            }
        } else {
            cerr << "Failed to create broadcast Tx SPS Flow." << endl;
            return Status::FAILED;
        }

        if (not spsFlowCreated) {
            cout << "now try with event flow" << endl;
            resetCallbackPromise();
            auto txEventFlowCallback = [](std::shared_ptr<ICv2xTxFlow> txEventFlow,
                                          telux::common::ErrorCode error) {
                if (ErrorCode::SUCCESS == error) {
                    gTxBroadcastFlow = txEventFlow;
                }
                gCallbackPromise.set_value(error);
            };
            status = gCv2xRadio->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                                   gBroadcastServiceId,
                                                   gBroadcastPort,
                                                   txEventFlowCallback);
            if (Status::SUCCESS != status or
                ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
                cerr << "Failed to create broadcast Tx event Flow." << endl;
                return Status::FAILED;
            }
        }
        cout << "Succeeded in creating broadcast Tx Flow, sock:";
        cout << gTxBroadcastFlow->getSock();
        cout << " , port:"<< gBroadcastPort << endl;
    }

    return Status::SUCCESS;
}

static Status registerUnicastFlows() {
    Status status = Status::SUCCESS;

    if (!gCv2xRadio) {
        cerr << "Invalid radio!" << endl;
        return Status::FAILED;
    }

    cout << "Registering Unicast Tx Flow" << endl;

    auto createTxEventFlowCallback = [](shared_ptr<ICv2xTxFlow> txEventFlow,
                                        ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            gTxUnicastFlow = txEventFlow;
        }
        gCallbackPromise.set_value(error);
    };

    // Reset global callback
    resetCallbackPromise();

    // Register unicast non-Ip Tx flow with unicast port, SID is ignored for unicast flows
    EventFlowInfo flowInfo;
    flowInfo.isUnicast = true;
    status = gCv2xRadio->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                           UNICAST_SERVICE_ID,
                                           flowInfo,
                                           DEFUALT_UNICAST_PORT,
                                           createTxEventFlowCallback);
    if (Status::SUCCESS != status or
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Failed to create unicast Tx Flow." << endl;
        return Status::FAILED;
    }

    cout << "Succeeded in creating unicast Tx event Flow, sock:";
    cout << gTxUnicastFlow->getSock();
    cout << " , port:"<< DEFUALT_UNICAST_PORT << endl;

    cout << "Registering unicast Rx Flow" << endl;

    auto createRxSubCallback = [](shared_ptr<ICv2xRxSubscription> rxSub,
                                  ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            gRxUnicastFlow = rxSub;
        }
        gCallbackPromise.set_value(error);
    };

    // Reset global callback
    resetCallbackPromise();

    // Subscribe to unicast port
    // Unicast SID is specified to avoid creating wildcard for broadcast Rx
    auto idList = std::make_shared<std::vector<uint32_t>>(1, UNICAST_SERVICE_ID);
    status = gCv2xRadio->createRxSubscription(TrafficIpType::TRAFFIC_NON_IP,
                                              DEFUALT_UNICAST_PORT,
                                              createRxSubCallback,
                                              idList);
    if (Status::SUCCESS != status or
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Failed to create unicast Rx Flow!" << endl;
        return Status::FAILED;
    }

    // Adding 500ms timeout to avoid indefinite read wait
    // Expand the timeout value can avoid Rx failure due to transmission delay in tunnel mode
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    if (setsockopt(gTxUnicastFlow->getSock(),
                   SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "Failed to set Tx socket timeout!" << endl;
        return Status::FAILED;
    }

    if (setsockopt(gRxUnicastFlow->getSock(),
                   SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "Failed to set Rx socket timeout!" << endl;
        return Status::FAILED;
    }

    cout << "Succeeded in creating unicast Rx Flow, sock:";
    cout << gRxUnicastFlow->getSock();
    cout << " , port:"<< DEFUALT_UNICAST_PORT << endl;

    return Status::SUCCESS;
}

static Status deregisterBroadcastFlows() {
    Status status = Status::SUCCESS;

    auto closeTxFlowCallback = [](shared_ptr<ICv2xTxFlow> txFlow, ErrorCode error) {
        gCallbackPromise.set_value(error);
    };

    auto closeRxSubCallback = [](shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
        gCallbackPromise.set_value(error);
    };

    if (gCv2xRadio and gTxBroadcastFlow) {
        cout << "closing broadcast tx flow, sock:" << gTxBroadcastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = gCv2xRadio->closeTxFlow(gTxBroadcastFlow, closeTxFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close broadcast tx flow!" << endl;
            status = Status::FAILED;
        }
        gTxBroadcastFlow = nullptr;
    }

    if (gCv2xRadio and gRxBroadcastFlow) {
        cout << "closing broadcast rx flow, sock:" << gRxBroadcastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = gCv2xRadio->closeRxSubscription(gRxBroadcastFlow, closeRxSubCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close broadcast rx flow!" << endl;
            status = Status::FAILED;
        }
        gRxBroadcastFlow = nullptr;
    }

    return status;
}

static Status deregisterUnicastFlows() {
    Status status = Status::SUCCESS;

    auto closeTxFlowCallback = [](shared_ptr<ICv2xTxFlow> txFlow, ErrorCode error) {
        gCallbackPromise.set_value(error);
    };

    auto closeRxSubCallback = [](shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
        gCallbackPromise.set_value(error);
    };

    if (gCv2xRadio and gTxUnicastFlow) {
        cout << "closing unicast tx flow, sock:" << gTxUnicastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = gCv2xRadio->closeTxFlow(gTxUnicastFlow, closeTxFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close unicast tx flow!" << endl;
            status = Status::FAILED;
        }
        gTxUnicastFlow = nullptr;
    }

    if (gCv2xRadio and gRxUnicastFlow) {
        cout << "closing unicast rx flow, sock:" << gRxUnicastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = gCv2xRadio->closeRxSubscription(gRxUnicastFlow, closeRxSubCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close unicast rx flow!" << endl;
            status = Status::FAILED;
        }
        gRxUnicastFlow = nullptr;
    }

    return status;
}

void terminationCleanup() {
    lock_guard<mutex> lock(gOperationMutex);

    cout << "Terminating" << endl;

    // close broadcast flows
    deregisterBroadcastFlows();

    // close unicast flows
    deregisterUnicastFlows();

    // Deregister listener
    if (gCv2xRadio and gRadioListener) {
        gCv2xRadio->deregisterListener(gRadioListener);
    }

    if (gCv2xRadioMgr and gStatusListener) {
        gCv2xRadioMgr->deregisterListener(gStatusListener);
    }

    cout << "Unicast Tx count:" << gTxUnicastCount << endl;
    cout << "Unicast Rx count:" << gRxUnicastCount << endl;
    cout << "Missed unicast Tx count:" << gMissedTxUnicastCount << endl;
    cout << "Missed unicast Rx count:" << gMissedRxUnicastCount << endl;
}

static int init() {
    lock_guard<mutex> lock(gOperationMutex);

    if (gTerminate) {
        return EXIT_FAILURE;
    }

    // Get handle to Cv2xRadioManager
    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xRadioManagerStatusUpdated = true;
        cv2xRadioManagerStatus = status;
        cv.notify_all();
    };

    auto & cv2xFactory = Cv2xFactory::getInstance();
    gCv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!gCv2xRadioMgr) {
        cout << "Error: failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
            return EXIT_FAILURE;
        }
    }

    // Get C-V2X status and make sure Tx/Rx enabled
    assert(Status::SUCCESS == gCv2xRadioMgr->requestCv2xStatus(cv2xStatusCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    // ensure V2X Tx and Rx active status before running the test
    if (!isV2xReady()) {
        return EXIT_FAILURE;
    }

    // Get handle to Cv2xRadio
    bool cv2x_radio_status_updated = false;
    telux::common::ServiceStatus cv2xRadioStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    auto cb = [&](ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2x_radio_status_updated = true;
        cv2xRadioStatus = status;
        cv.notify_all();
    };

    gCv2xRadio = gCv2xRadioMgr->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);

    if (not gCv2xRadio) {
        cerr << "C-V2X Radio creation failed." << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lc(mtx);
        cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

        if (cv2xRadioStatus != ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X Radio initialization failed." << endl;
            return EXIT_FAILURE;
        } else {
            cout << "C-V2X Radio is ready" << endl;
        }
    }

    // Register for Src L2 Id Update callbacks
    gRadioListener = std::make_shared<RadioListener>();
    if (Status::SUCCESS != gCv2xRadio->registerListener(gRadioListener)) {
        cerr << "Radio listener registration failed." << endl;
        return EXIT_FAILURE;
    }

    // Register for cv2x status update
    gStatusListener = std::make_shared<Cv2xStatusListener>();
    if (Status::SUCCESS != gCv2xRadioMgr->registerListener(gStatusListener)) {
        cerr << "Status listener registration failed." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Register broadcast and unicast flows
static int setupFlows() {
    lock_guard<mutex> lock(gOperationMutex);

    if (gTerminate) {
        return EXIT_FAILURE;
    }

    // deregister flows if exist
    if (Status::SUCCESS != deregisterBroadcastFlows() or
        Status::SUCCESS != deregisterUnicastFlows()) {
        return EXIT_FAILURE;
    }

    // register flows
    if (Status::SUCCESS != registerBroadcastFlows() or
        Status::SUCCESS != registerUnicastFlows()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// If in RSU mode, create thread for broadcasting WSA with interval 100ms
static void startTxWSARSU() {
    cout << "Start sending WSA." << endl;
    int sock = gTxBroadcastFlow->getSock();
    gWsaThread = std::async(std::launch::async, [sock]() {
        cv2x_message_t cv2xMsg;
        struct sockaddr_in6 dstAddr = {0};
        while (!gTerminate) {
            waitV2xStatusActive();
            if (!isV2xReady()) {
                continue;
            }
            createBuffer(cv2xMsg);
            sampleTx(sock, cv2xMsg, false, dstAddr);
            usleep(100000u);
        }
    });
    gWsaThreadValid = true;
}

int startRxWSAOBU() {
    cv2x_message_t cv2xMsg = { 0 };

    cout << "Waiting for WSA from RSU." << endl;
    if (sampleRx(gRxBroadcastFlow->getSock(), cv2xMsg, gRsuAddr) <= 0) {
        return EXIT_FAILURE;
    }

    cout << "Received WSA from RSU L2 ID: ";
    cout << ntohl(gRsuAddr.sin6_addr.s6_addr32[3]) << endl;

    // Modify port number to unicast port for the Tx of unicast msgs
    gRsuAddr.sin6_port = htons((uint16_t)DEFUALT_UNICAST_PORT);
    gWsaReceived = true;

    return EXIT_SUCCESS;
}

static int startUnicastRSU() {
    cv2x_message_t cv2xMsg = { 0 };

    cout << "Waiting for unicast msg from OBU." << endl;

    // On error or timeout of socket resume waiting for unicast Tx
    struct sockaddr_in6 obuAddr = { 0 };
    if (sampleRx(gRxUnicastFlow->getSock(), cv2xMsg, obuAddr) <= 0) {
        return EXIT_FAILURE;
    }

    // latency in milliseconds
    uint64_t timestamp = getCurrentTimestamp();
    uint64_t latency = 0;
    if (timestamp > cv2xMsg.contents.timestamp) {
        latency = (timestamp - cv2xMsg.contents.timestamp)/1000;
    } else {
        cerr << " negative latency!" << endl;
    }

    gRxUnicastCount++;
    cout << "Received unicast msg from OBU L2 ID: ";
    cout << ntohl(obuAddr.sin6_addr.s6_addr32[3]);
    cout << ", Rx seq num:" << cv2xMsg.contents.seqNum;
    cout << ", latency:" << latency << "ms" << endl;

    cout << "Sending unicast echo msg to OBU." << endl;
    if (sampleTx(gTxUnicastFlow->getSock(), cv2xMsg, true, obuAddr) < 0) {
        cerr << "Failed to echo OBU!" << endl;
        gMissedTxUnicastCount++;
        return EXIT_FAILURE;
    }

    gTxUnicastCount++;
    return EXIT_SUCCESS;
}

static int startUnicastOBU() {
    cv2x_message_t cv2xMsg = { 0 };

    // send unicast msg and wait for the echo from RSU
    cout << "Sending unicast msg to RSU L2 ID:";
    cout << ntohl(gRsuAddr.sin6_addr.s6_addr32[3]) << endl;

    createBuffer(cv2xMsg);
    if (sampleTx(gTxUnicastFlow->getSock(), cv2xMsg, true, gRsuAddr) < 0) {
        cerr << "Failed to send unicast msg!" << endl;
        gMissedTxUnicastCount++;
        return EXIT_FAILURE;
    }
    gTxUnicastCount++;

    cout << "Waiting for unicast echo msg from RSU." << endl;
    cv2x_message_t echoCv2xMsg = { 0 };
    struct sockaddr_in6 tmpRsuAddr = { 0 };
    bool recvEcho = false;
    while (sampleRx(gRxUnicastFlow->getSock(), echoCv2xMsg, tmpRsuAddr) > 0) {
        gRxUnicastCount++;
        gSuccessiveMissedEcho = 0;

        // received matched echo
        if (cv2xMsg.length == echoCv2xMsg.length and
            0 == memcmp(&cv2xMsg.contents, &echoCv2xMsg.contents,
                        sizeof(cv2x_common_message_t))) {
            cout << "Received echo msg from RSU L2 ID:";
            cout << ntohl(tmpRsuAddr.sin6_addr.s6_addr32[3]) << endl;

            // update RSU's address according to echo msg.
            // echo msg always has the latest RSU's address
            if (memcmp(&tmpRsuAddr, &gRsuAddr, sizeof(struct sockaddr_in6))) {
                cout << "RSU L2 address updated." << endl;
                gRsuAddr = tmpRsuAddr;
            }
            recvEcho = true;
            break;
        } else {
            // received mismatched echo
            cout << "Received mismatched echo msg from RSU L2 ID: ";
            cout << ntohl(tmpRsuAddr.sin6_addr.s6_addr32[3]);
            cout << ", Tx seq num: " << cv2xMsg.contents.seqNum;
            cout << ", Rx seq num: " << echoCv2xMsg.contents.seqNum << endl;

            // received out-of-sequence pkt, might caused by RSU suspended status
            // continually Rx unicast msg until Rx failure or Rx matched seq num
            if (echoCv2xMsg.contents.seqNum < cv2xMsg.contents.seqNum) {
                if (gMissedRxUnicastCount > 0) {
                    gMissedRxUnicastCount--;
                }
                continue;
            }
        }
    }

    if (!recvEcho) {
        gMissedRxUnicastCount++;
        gSuccessiveMissedEcho++;
        cerr << "Failed to receive echo msg of seq num: ";
        cerr << cv2xMsg.contents.seqNum;
        cerr << ", successive missed num:" << gSuccessiveMissedEcho << endl;

        if (gSuccessiveMissedEcho >= SUCCESSIVE_MISSED_ECHO_NUM) {
            // the RSU's address might have changed,
            // re-regiter flows and wait for new WSA
            cout << "Re-create flows to get new WSA." << endl;
            if (setupFlows()) {
                return EXIT_FAILURE;
            }
            gWsaReceived = false;
            gSuccessiveMissedEcho = 0;
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X Unicast app" << endl;
    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    if (pipe(gTerminatePipe) == -1) {
        cout << "Pipe error" << endl;
        return EXIT_FAILURE;
    }

    installSignalHandler();

    // create async task to handle termination
    gTerminateThread = std::async(std::launch::async, []() {
        int terminate = 0;
        read(gTerminatePipe[0], &terminate, sizeof(int));
        cout << "Read terminate:" << terminate << endl;
        terminationCleanup();
    });

    // parse options
    if (parseOpts(argc, argv)) {
        goto bail;
    }

    // get cv2x handles
    if (init()) {
        goto bail;
    }

    // setup flows for broadcast and unicast
    if (setupFlows()) {
        goto bail;
    }

    // create async task in RSU mode to Tx WSA
    if (UnicastTestMode::RSU == gUnicastTestMode) {
        startTxWSARSU();
    }

    // main operation loop
    while (!gTerminate) {
        // wait for V2X active status before Tx/Rx
        waitV2xStatusActive();
        if (!isV2xReady()) {
            continue;
        }

        if (UnicastTestMode::OBU == gUnicastTestMode) {
            // wait for WSA to get the original RSU's address
            if (not gWsaReceived) {
                if (startRxWSAOBU()) {
                    continue;
                }
            }

            // start Tx unicast msg with interval 100ms and wait for echo
            if (EXIT_SUCCESS == startUnicastOBU()) {
                usleep(100000u);
            }
        } else {
            // wait for unicast msg and send echo
            startUnicastRSU();
        }
    }

bail:
    // teminate
    gTerminate = 1;
    write(gTerminatePipe[1], &gTerminate, sizeof(int));
    gTerminateThread.get();
    if (gWsaThreadValid) {
        gWsaThread.get();
    }
    if (gCv2xRadio) {
        gCv2xRadio = nullptr;
    }
    if (gCv2xRadioMgr) {
        gCv2xRadioMgr = nullptr;
    }

    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
