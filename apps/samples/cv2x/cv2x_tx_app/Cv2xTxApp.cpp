/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 * @file: Cv2xTxApp.cpp
 *
 * @brief: Simple application that demonstrates Tx in Cv2x
 */

#include <assert.h>
#include <ifaddrs.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <memory>

#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xUtil.hpp>

#include "../../../common/utils/Utils.hpp"

using std::array;
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
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::SpsFlowInfo;
using telux::cv2x::DataSessionSettings;
using telux::cv2x::Cv2xUtil;

static constexpr uint32_t TX_SERVICE_ID = 1u;
static constexpr uint16_t SPS_PORT_NUM = 2500u;
static uint16_t EVENT_PORT_NUM = 2600u;
static constexpr uint32_t G_BUF_LEN = 128;
static uint32_t NUM_TEST_ITERATIONS = 1;
static constexpr int      PRIORITY = 3;

static constexpr char TEST_VERNO_MAGIC = 'Q';
static constexpr char UEID = 1;

enum class TxFlowType {
    SpsOnly,
    EventOnly,
    Combine
};

static Cv2xStatus gCv2xStatus;
static promise<ErrorCode> gCallbackPromise;
static shared_ptr<ICv2xTxFlow> gSpsFlow = nullptr;
static shared_ptr<ICv2xTxFlow> gEvtFlow = nullptr;
static std::weak_ptr<ICv2xTxFlow> gTxFlow;
static array<char, G_BUF_LEN> gBuf;
static TxFlowType gFlowType = TxFlowType::SpsOnly;
static bool gAutoRetransMode = true;

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

// Callback function for ICv2xRadio->createTxSpsFlow()
static void createSpsFlowCallback(shared_ptr<ICv2xTxFlow> txSpsFlow,
                                  shared_ptr<ICv2xTxFlow> evtFlow,
                                  ErrorCode spsError,
                                  ErrorCode evtError) {
    if (ErrorCode::SUCCESS == spsError) {
        gSpsFlow = txSpsFlow;
    }
    if (ErrorCode::SUCCESS == evtError) {
        gEvtFlow = evtFlow;
    }
    auto ec = (ErrorCode::SUCCESS == spsError || ErrorCode::SUCCESS == evtError) ?
        ErrorCode::SUCCESS : ErrorCode::GENERIC_FAILURE;
    gCallbackPromise.set_value(ec);
}

// Callback function for ICv2xRadio->createTxEventFlow()
static void createEventFlowCallback(shared_ptr<ICv2xTxFlow> txEventFlow,
                                  ErrorCode eventError) {
    if (ErrorCode::SUCCESS == eventError) {
        gEvtFlow = txEventFlow;
    }
    gCallbackPromise.set_value(eventError);
}

// Callback function for ICv2xRadio->changeEventFlowInfo()
static void changeEventFlowInfoCallback(shared_ptr<ICv2xTxFlow> txEventFlow,
                                  ErrorCode eventError) {
    if (ErrorCode::SUCCESS == eventError) {
        gEvtFlow = txEventFlow;
    }
    gCallbackPromise.set_value(eventError);
}

// Callback function for ICv2xRadio->changeSpsFlowInfo()
static void changeSpsFlowInfoCallback(shared_ptr<ICv2xTxFlow> txSpsFlow,
                                  ErrorCode spsError) {
    if (ErrorCode::SUCCESS == spsError) {
        gSpsFlow = txSpsFlow;
    }
    gCallbackPromise.set_value(spsError);
}

// Callback function for ICv2xRadio->requestSpsFlowInfo()
static void requestSpsFlowInfoCallback(shared_ptr<ICv2xTxFlow> txSpsFlow,
                                  const SpsFlowInfo & spsInfo,
                                  ErrorCode spsError) {
    if (ErrorCode::SUCCESS == spsError) {
        cout << "Priority: " << static_cast<int>(spsInfo.priority)
            << ", Periodicity: " << static_cast<int>(spsInfo.periodicity)
            << ", NbytesReserved: "<< spsInfo.nbytesReserved
            << ", Traffic class:"
            << static_cast<int>(Cv2xUtil::priorityToTrafficClass(spsInfo.priority)) << endl;
    }
    gCallbackPromise.set_value(spsError);
}

// Callback function for ICv2xRadio->requestDataSessionSettings()
static void requestDataSessionSettingsCallback(const DataSessionSettings & settings,
                                  ErrorCode spsError) {
    if (ErrorCode::SUCCESS == spsError) {
        if (settings.mtuValid) {
            cout << "MTU size: " << settings.mtu << endl;
        }
    }
    gCallbackPromise.set_value(spsError);
}

// Returns current timestamp
static uint64_t getCurrentTimestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ull + (uint16_t)tv.tv_usec;
}

// Fills buffer with dummy data
static void fillBuffer(void) {

    static uint16_t seq_num = 0u;
    auto timestamp = getCurrentTimestamp();

    // Very first payload is test Magic number, this is  where V2X Family ID would normally be.
    gBuf[0] = TEST_VERNO_MAGIC;

    // Next byte is the UE equipment ID
    gBuf[1] = UEID;

    // Sequence number
    auto dataPtr = gBuf.data() + 2;
    uint16_t tmp = htons(seq_num++);
    memcpy(dataPtr, reinterpret_cast<char *>(&tmp), sizeof(uint16_t));
    dataPtr += sizeof(uint16_t);

    // Timestamp
    dataPtr += snprintf(dataPtr, G_BUF_LEN - (2 + sizeof(uint16_t)),
                        "<%llu> ", static_cast<long long unsigned>(timestamp));

    // Dummy payload
    constexpr int NUM_LETTERS = 26;
    auto i = 2 + sizeof(uint16_t) - sizeof(long long unsigned);
    for (; i < G_BUF_LEN; ++i) {
        gBuf[i] = 'a' + ((seq_num + i) % NUM_LETTERS);
    }
}

// Function for transmitting data
static void sampleTx(shared_ptr<ICv2xTxFlow> txFlow) {

    static uint32_t txCount = 0u;
    if (not txFlow) {
        return;
    }
    int sock = txFlow->getSock();

    cout << "sampleSpsTx(" << sock << ")" << endl;

    struct msghdr message = {0};
    struct iovec iov[1] = {0};
    struct cmsghdr * cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    // Send data using sendmsg to provide IPV6_TCLASS per packet
    iov[0].iov_base = gBuf.data();
    iov[0].iov_len = G_BUF_LEN;
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
    auto bytes_sent = sendmsg(sock, &message, 0);
    cout << "bytes_sent=" << bytes_sent << endl;

    // Check bytes sent
    if (bytes_sent < 0) {
        cerr << "Error sending message: " << bytes_sent << endl;
        bytes_sent = -1;
    } else {
        if (bytes_sent == G_BUF_LEN) {
           ++txCount;
        } else {
            cerr << "Error : " << bytes_sent << " bytes sent." << endl;
        }
    }

    cout << "TX count: " << txCount << endl;
}

// Callback for ICv2xRadio->closeTxFlow()
static void closeFlowCallback(shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
    gCallbackPromise.set_value(error);
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << "\n"
         << "-c combine tx flow type\n"
         << "-e event tx flow type\n"
         << "-i <iterations> packets number going to send\n"
         << "-r <auto-retrans mode>  0--disable 1--enable, default to enable\n"
         << "-t <eventFlow port> event flow port number, applicable only if event flow exist\n"
         << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?cei:r:t:")) != -1) {
        switch (c) {
        case 'c':
            gFlowType = TxFlowType::Combine;
            cout << "Create Combine flow" << endl;
            break;
        case 'e':
            gFlowType = TxFlowType::EventOnly;
            cout << "Create Tx event flow" << endl;
            break;
        case 'i':
            if (optarg) {
                auto iterations = atoi(optarg);
                if (iterations >= 0) {
                    NUM_TEST_ITERATIONS = static_cast<uint32_t>(iterations);
                } else {
                    std::cerr << "Ignore iterations " << static_cast<int>(iterations) << endl;
                }
                cout << "NUM_TEST_ITERATIONS: " << NUM_TEST_ITERATIONS << endl;
            }
            break;
        case 'r':
            if (optarg) {
               gAutoRetransMode = static_cast<bool>(atoi(optarg));
               cout << "auto retrans mode: " << static_cast<int>(gAutoRetransMode) << endl;
               }
           break;
        case 't':
            if (optarg) {
               auto eventPort = atoi(optarg);
               if (eventPort >= 1024) {
                   EVENT_PORT_NUM = static_cast<uint16_t>(eventPort);
               } else {
                   std::cerr << "Ignore event portnum " << static_cast<int>(eventPort) << endl;
               }
               cout << "EVENT_PORT_NUM: " << static_cast<int>(EVENT_PORT_NUM) << endl;
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

static int createTxFlow(std::shared_ptr<telux::cv2x::ICv2xRadio> &radio) {
    if (gFlowType == TxFlowType::SpsOnly || gFlowType == TxFlowType::Combine) {
        SpsFlowInfo spsInfo;
        spsInfo.priority = Priority::PRIORITY_2;
        spsInfo.periodicity = Periodicity::PERIODICITY_100MS;
        spsInfo.nbytesReserved = G_BUF_LEN;
        auto createEvtFlow = (gFlowType == TxFlowType::Combine) ? true : false;

        resetCallbackPromise();
        if(Status::SUCCESS != radio->createTxSpsFlow(TrafficIpType::TRAFFIC_NON_IP,
                                                         TX_SERVICE_ID,
                                                         spsInfo,
                                                         SPS_PORT_NUM,
                                                         createEvtFlow,
                                                         EVENT_PORT_NUM,
                                                         createSpsFlowCallback)
            || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to create tx sps flow" << endl;
            return EXIT_FAILURE;
        }
        gTxFlow = gSpsFlow ? gSpsFlow : gEvtFlow;

        resetCallbackPromise();
        if (gSpsFlow) {
            if (Status::SUCCESS != radio->requestSpsFlowInfo(
                                    gSpsFlow, requestSpsFlowInfoCallback)
                || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
                cerr << "Failed to request for sps flow info" << endl;
                return EXIT_FAILURE;
            }

            if(!gAutoRetransMode) {
                spsInfo.autoRetransEnabled = false;
                resetCallbackPromise();
                if(Status::SUCCESS != radio->changeSpsFlowInfo(gSpsFlow,
                                                   spsInfo,
                                                   changeSpsFlowInfoCallback)
                    || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
                    cerr << "Failed to request to change sps flow info" << endl;
                    return EXIT_FAILURE;
                }
            }
        }
    } else if (gFlowType == TxFlowType::EventOnly) {
        EventFlowInfo eventInfo;

        resetCallbackPromise();
        if(Status::SUCCESS != radio->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                               TX_SERVICE_ID,
                                               eventInfo,
                                               EVENT_PORT_NUM,
                                               createEventFlowCallback)
            || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to create tx event flow" << endl;
            return EXIT_FAILURE;
        }
        gTxFlow = gEvtFlow;

        if(!gAutoRetransMode) {
            eventInfo.autoRetransEnabled = false;
            resetCallbackPromise();
            if(Status::SUCCESS != radio->changeEventFlowInfo(gEvtFlow,
                                                   eventInfo,
                                                   changeEventFlowInfoCallback)
                || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
                cerr << "Failed to request to change event flow info" << endl;
                return EXIT_FAILURE;
            }
        }
    } else {
        cerr << "Incorrect tx flow type." << endl;
        return EXIT_FAILURE;
    }
    if (auto flow = gTxFlow.lock()) {
        cout << "TX flow: ipType= " << static_cast<int>(flow->getIpType())
            << ", ServiceId= " << flow->getServiceId()
            << ", PortNum= " << flow->getPortNum() << endl;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X TX app" << endl;

    if (parseOpts(argc, argv) < 0) {
        return EXIT_FAILURE;
    }

    std::vector<std::string> groups{"system", "diag", "radio", "logd"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    // Get handle to Cv2xRadioManager
    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
            ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](ServiceStatus status) {
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
        if (ServiceStatus::SERVICE_AVAILABLE != cv2xRadioManagerStatus
            || ServiceStatus::SERVICE_AVAILABLE != cv2xRadioManager->getServiceStatus()) {
            cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
            return EXIT_FAILURE;
        }
    }

    // Get C-V2X status and make sure Tx is enabled
    if(Status::SUCCESS != cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback)
        || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Failed to request for Cv2x status" << endl;
        return EXIT_FAILURE;
    }

    if (Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
        cout << "C-V2X TX status is active" << endl;
    }
    else {
        cerr << "C-V2X TX is inactive" << endl;
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

    auto radio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);
    if (not radio) {
        cerr << "C-V2X Radio creation failed." << endl;
        return EXIT_FAILURE;
    }

    {
        // Wait for cv2x radio to complete initialization
        std::unique_lock<std::mutex> lc(mtx);
        cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

        if (cv2xRadioStatus != ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X Radio initialization failed." << endl;
            return EXIT_FAILURE;
        } else {
            cout << "C-V2X Radio is ready" << endl;
        }
    }

    resetCallbackPromise();
    if(Status::SUCCESS != radio->requestDataSessionSettings(requestDataSessionSettingsCallback)
        || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Failed to request for data session settings" << endl;
        return EXIT_FAILURE;
    }

    if (EXIT_SUCCESS == createTxFlow(radio)) {
        // Send message in a loop
        for (uint16_t i = 0; i < NUM_TEST_ITERATIONS; ++i) {
            fillBuffer();
            sampleTx(gTxFlow.lock());
            usleep(100000u);
        }
    }

    // Deregister TX flow
    if (gSpsFlow) {
        resetCallbackPromise();
        if(Status::SUCCESS != radio->closeTxFlow(gSpsFlow, closeFlowCallback)
            || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to request to close tx flow" << endl;
            return EXIT_FAILURE;
        }
    }
    if (gEvtFlow) {
        resetCallbackPromise();
        if(Status::SUCCESS != radio->closeTxFlow(gEvtFlow, closeFlowCallback)
            || ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to request to close tx flow" << endl;
            return EXIT_FAILURE;
        }
    }

    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
