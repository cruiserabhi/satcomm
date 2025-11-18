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
 * @file: Cv2xTcpApp.cpp
 *
 * @brief: Simple application that demonstrates Tx/Rx TCP packets in Cv2x
 */

#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <ifaddrs.h>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <telux/cv2x/Cv2xRadio.hpp>

#include "../../../common/utils/Utils.hpp"

using std::array;
using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::promise;
using std::shared_ptr;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xTxRxSocket;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::TrafficCategory;
using telux::cv2x::SocketInfo;
using telux::cv2x::EventFlowInfo;

static constexpr uint32_t SERVIC_ID = 1u;
static constexpr uint8_t TCP_CLIENT = 0u;
static constexpr uint8_t TCP_SERVER = 1u;
static constexpr uint16_t DEFAULT_PORT = 5000u;
static constexpr int      PRIORITY = 5;
static constexpr uint32_t PACKET_LEN = 128u;
static constexpr uint32_t PACKET_NUM = 2u;
static constexpr uint32_t MAX_DUMMY_PACKET_LEN = 10000;

static constexpr char TEST_VERNO_MAGIC = 'Q';
static constexpr char CLIENT_UEID = 1;
static constexpr char SERVER_UEID = 2;


static std::shared_ptr<ICv2xRadio> gCv2xRadio;
static Cv2xStatus gCv2xStatus;
static promise<ErrorCode> gCallbackPromise;
static shared_ptr<ICv2xTxRxSocket> gTcpSock;
static array<char, MAX_DUMMY_PACKET_LEN> gBuf;

static uint8_t gTcpMode = TCP_CLIENT;
static uint16_t gSrcPort = DEFAULT_PORT;
static uint16_t gDstPort = DEFAULT_PORT;
static string gDstAddr;
static int32_t gAcceptedSock = -1;
static uint32_t gServiceId = SERVIC_ID;
static uint32_t gPacketLen = PACKET_LEN;

static int g_terminate = 0;
static int g_terminate_pipe[2];

// Resets the global callback promise
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback function for ICv2xRadioManager->requestCv2xStatus()
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus = status;
    }
    gCallbackPromise.set_value(error);
}

// Callback function for ICv2xRadio->createCv2xTcpSocket()
static void createTcpSocketCallback(shared_ptr<ICv2xTxRxSocket> sock,
                                     ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gTcpSock = sock;
    }
    gCallbackPromise.set_value(error);
}

// Returns current timestamp
static uint64_t getCurrentTimestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ull + (uint64_t)tv.tv_usec;
}

// Fills buffer with dummy data
static void fillBuffer(void) {

    static uint16_t seq_num = 0u;
    auto timestamp = getCurrentTimestamp();

    // Very first payload is test Magic number, this is  where V2X Family ID would normally be.
    gBuf[0] = TEST_VERNO_MAGIC;

    // Next byte is the UE equipment ID
    if (gTcpMode == TCP_CLIENT) {
        gBuf[1] = CLIENT_UEID;
    } else {
        gBuf[1] = SERVER_UEID;
    }

    // Sequence number
    auto dataPtr = gBuf.data() + 2;
    uint16_t tmp = htons(seq_num++);
    memcpy(dataPtr, reinterpret_cast<char *>(&tmp), sizeof(uint16_t));
    dataPtr += sizeof(uint16_t);

    // Timestamp
    dataPtr += snprintf(dataPtr, gPacketLen - (2 + sizeof(uint16_t)),
                        "<%llu> ", static_cast<long long unsigned>(timestamp));

    // Dummy payload
    constexpr int NUM_LETTERS = 26;
    auto i = 2 + sizeof(uint16_t) + sizeof(long long unsigned);
    for (; i < gPacketLen; ++i) {
        gBuf[i] = 'a' + ((seq_num + i) % NUM_LETTERS);
    }
}

// Function for transmitting data
static int sampleTx(void) {
    static uint32_t txCount = 0u;
    int sock = -1;

    if (gTcpMode == TCP_CLIENT) {
        // For TCP client, use the created socket for send/recv on successful connection
        sock = gTcpSock->getSocket();
    } else {
        // For TCP server, use the socket accepted for send/recv
        sock = gAcceptedSock;
    }

    cout << "sampleTx(" << sock << ")" << endl;

    struct msghdr message = {0};
    struct iovec iov[1] = {0};
    struct cmsghdr * cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    // Send data using sendmsg to provide IPV6_TCLASS per packet
    iov[0].iov_base = gBuf.data();
    iov[0].iov_len = gPacketLen;
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
    cout << "TX count: " << txCount << " bytes:" << sendBytes << " UEID:" << (int)gBuf[1] << endl;
    return 0;
}

// Function for reading from Rx socket
static int sampleRx(void) {
    static uint32_t rxCount = 0u;
    int sock = -1;

    if (gTcpMode == TCP_CLIENT) {
        // For TCP client, use the created socket for send/recv
        sock = gTcpSock->getSocket();
    } else {
        // For TCP server, use the socket accepted for send/recv
        sock = gAcceptedSock;
    }

    cout << "sampleRx(" << sock << ")" << endl;

    // Attempt to read from socket
    int recvBytes = recv(sock, gBuf.data(), gBuf.max_size(), 0);

    if (recvBytes <= 0) {
        cerr << "Error occurred reading from sock:" << sock << " err:" << strerror(errno) << endl;
        return -1;
    }

    ++rxCount;
    cout << "RX count: " << rxCount << " bytes:" << recvBytes << " UEID:" << (int)gBuf[1] << endl;
    return 0;
}

// Callback for ICv2xRadio->closeCv2xTcpSocket()
static void closeTcpSocketCallback(shared_ptr<ICv2xTxRxSocket> chan, ErrorCode error) {
    gCallbackPromise.set_value(error);
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << endl;
    cout << "-d <dstAddr>       Destination IPV6 address used for connecting" << endl;
    cout << "-m <tcpMode>       0--Client, 1--Server" << endl;
    cout << "-s <srcPort>       Source port used for binding, default is 5000" << endl;
    cout << "-t <dstPort>       Destination port used for connecting, default is 5000" << endl;
    cout << "-p <service ID>    Service ID used for Tx and Rx flows, default is " << gServiceId << endl;
    cout << "-l <packet length> Tx Packet length, default is " << gPacketLen <<endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?d:m:s:t:p:l:")) != -1) {
        switch (c) {
        case 'd':
            if (optarg) {
                gDstAddr = optarg;
                cout << "dstAddr: " << gDstAddr << endl;
            }
            break;
        case 'm':
            if (optarg) {
                gTcpMode = atoi(optarg);
                cout << "tcpMode: " << gTcpMode << endl;
            }
            break;
        case 's':
            if (optarg) {
                gSrcPort = atoi(optarg);
                cout << "srcPort: " << gSrcPort << endl;
            }
            break;
        case 't':
            if (optarg) {
                gDstPort = atoi(optarg);
                cout << "dstPort: " << gDstPort << endl;
            }
            break;
        case 'p':
            if (optarg) {
                gServiceId = atoi(optarg);
                cout << "service ID: " << gServiceId << endl;
            }
            break;
        case 'l':
            if (optarg) {
                gPacketLen = atoi(optarg);
                cout << "packet length: " << gPacketLen << endl;
            }
            break;
        case '?':
        default:
            rc = -1;
            printUsage(argv[0]);
            return rc;
        }
    }

    if (gTcpMode == TCP_CLIENT && gDstAddr.empty()) {
        cout << "error Destination IP Addr." << endl;
        rc = -1;
    }

    return rc;
}

static void termination_cleanup() {
    cout << "terminating" << endl;

    // For TCP server, close the socket accepted before releasing the listening socket
    if (gAcceptedSock >= 0) {
        cout << "closing client sock:" << gAcceptedSock << endl;
        close(gAcceptedSock);
    }

    // Release resources of TCP socket
    if (gTcpSock) {
        cout << "closing Tcp socket, fd:" << gTcpSock->getSocket() << endl;
        resetCallbackPromise();
        if(Status::SUCCESS != gCv2xRadio->closeCv2xTcpSocket(gTcpSock, closeTcpSocketCallback) ||
           ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cout << "close Tcp socket err" << endl;
        }
    }
    if (gCv2xRadio) {
        gCv2xRadio = nullptr;
    }
    exit(0);
}

static void termination_handler(int signum)
{
    g_terminate = 1;
    write(g_terminate_pipe[1], &g_terminate, sizeof(int));
}

static void install_signal_handler()
{
    struct sigaction sig_action;

    sig_action.sa_handler = termination_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGHUP, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X TCP app" << endl;
    std::vector<std::string> groups{"system", "diag", "radio", "logd"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    if (pipe(g_terminate_pipe) == -1) {
        cout << "Pipe error" << endl;
        return EXIT_FAILURE;
    }

    install_signal_handler();

    auto f = std::async(std::launch::async, []()
        {
            int terminate = 0;
            read(g_terminate_pipe[0], &terminate, sizeof(int));
            cout << "Read terminate:" << terminate << endl;
            termination_cleanup();
        });

    // Parse parameters
    if (parseOpts(argc, argv) < 0) {
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
    // Get handle to Cv2xRadioManager
    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioManager) {
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

    // Get C-V2X status and make sure Tx is enabled
    assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    if (Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
        cout << "C-V2X TX/RX status is active" << endl;
    } else {
        cerr << "C-V2X TX/RX is inactive" << endl;
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

    gCv2xRadio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);

    if (not gCv2xRadio) {
        cerr << "C-V2X Radio creation failed." << endl;
        return EXIT_FAILURE;
    }

    {
        // Wait for cv2x radio to complete initialization
        std::unique_lock<std::mutex> lc(mtx);
        cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

        if (cv2xRadioStatus != ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X Radio initialization failed." << endl;
            gCv2xRadio = nullptr;
            return EXIT_FAILURE;
        } else {
            cout << "C-V2X Radio is ready" << endl;
        }
    }

    // Create TCP Socket
    cout << "creating Tcp Socket" << endl;
    SocketInfo tcpInfo;
    tcpInfo.serviceId = gServiceId;
    tcpInfo.localPort = gSrcPort;
    EventFlowInfo eventInfo;
    resetCallbackPromise();
    if (Status::SUCCESS != gCv2xRadio->createCv2xTcpSocket(eventInfo, tcpInfo,
                                                           createTcpSocketCallback) ||
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "Tcp Socket creation failed." << endl;
        gCv2xRadio = nullptr;
        return EXIT_FAILURE;
    }

    int sock = gTcpSock->getSocket();
    if (gTcpMode == TCP_CLIENT) {
        // For TCP client, establish connection with the created sock
        struct sockaddr_in6 dstSockAddr = {0}; //must reset the sockaddr
        dstSockAddr.sin6_port = htons((uint16_t)gDstPort);
        inet_pton(AF_INET6, gDstAddr.c_str(), (void *)&dstSockAddr.sin6_addr);
        dstSockAddr.sin6_family = AF_INET6;

        cout << "connecting sock:" << sock << endl;
        if (connect(sock, (struct sockaddr *)&dstSockAddr, sizeof(struct sockaddr_in6))) {
            cout << "connect err:" << strerror(errno) << endl;
            goto bail;
        }

        // Send message on successful connection
        cout << "sending msg" << endl;
        for (uint32_t i = 0; i < PACKET_NUM; ++i) {
            fillBuffer();
            if (sampleTx() < 0) {
                break;
            }

            usleep(100000u);
        }

        // Recv the last message before closing the created sock
        cout << "recving the last msg" << endl;
        sampleRx();
    } else {
        // For TCP server, mark the created socket as listending sock
        cout << "listening sock" << sock << endl;
        if (listen(sock, 5) < 0) {
            cout << "connect err:" << strerror(errno) << endl;
            goto bail;
        }

        // Accept connection requests from clients circularly
        for (;;) {
            // Accept connection request
            cout << "accepting connection..." << endl;
            struct sockaddr_in6 tmpAddr = {0};
            socklen_t socklen = sizeof(tmpAddr);
            gAcceptedSock = accept(sock, (struct sockaddr *)&tmpAddr, &socklen);
            if (gAcceptedSock < 0) {
                cout << "accept err:" << strerror(errno) << endl;
                goto bail;
            }
            cout << "accepted client sock:" << gAcceptedSock << endl;

            // Use the accepted socket to recv message
            cout << "recving msg" << endl;
            for (uint32_t i = 0; i < PACKET_NUM; ++i) {
                if (sampleRx() < 0) {
                    break;
                }
            }

            // Use the accepted socket to send the last message
            cout << "sending the last msg" << endl;
            fillBuffer();
            sampleTx();

            // Wait until the client side close the socket
            cout << "wait until the peer side close socket" << endl;
            if (sampleRx() < 0) {
                close(gAcceptedSock);
                gAcceptedSock = -1;
            }
        }
    }

bail:
    // teminate
    g_terminate = 1;
    write(g_terminate_pipe[1], &g_terminate, sizeof(int));
    pause();
    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
