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
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xRxApp.cpp
 *
 * @brief: Simple application that demonstrates non-IP Rx mode in Cv2x
 */

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include "Cv2xRxApp.hpp"
#include "../../../common/utils/Utils.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::promise;
using std::shared_ptr;
using std::make_shared;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::common::ErrorCode;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;
using telux::cv2x::Cv2xUtil;
using telux::cv2x::Priority;

static constexpr uint32_t G_BUF_LEN = 3000u;
static constexpr uint8_t MAX_SID_NUM = 10u;
static bool gExiting = false;

void Cv2xRxApp::printUsage() {
    cout << "Usage: " << endl;
    cout << "-m <Rx mode>        Rx mode 0:wildcard, 1:catchall, 2:specific SID" << endl;
    cout << "-p <Rx port>        Rx port number, default is "<< RX_PORT_NUM << endl;
    cout << "-s <SID1>,<SID2>... SID/SIDs used for specific SID/catchall Rx mode" << endl;
}

int Cv2xRxApp::parseSidList(char* param) {
    char* saveptr = NULL;
    char* tok = param;
    char* endptr = NULL;
    int i = 0;
    unsigned int sid = 0;

    tok = strtok_r(tok, ",", &saveptr);
    while ((tok != NULL) && (i < MAX_SID_NUM)) {
        sid = (unsigned int)strtoul(tok, &endptr, 0);
        if (errno == ERANGE) {
            cerr << "range err for sid!" << endl;
            return EXIT_FAILURE;
        } else if (endptr == tok || *endptr != '\0') {
            cerr << "sid convert fail!" << endl;
            return EXIT_FAILURE;
        }

        idVector_.push_back(sid);
        tok = strtok_r(NULL, ",", &saveptr);
    }

    if (idVector_.size() == 0) {
        printf("Invalid parameters for SID subsrprition\n");
        return EXIT_FAILURE;
    }

    cout << "Set Rx SID:";
    for (std::vector<unsigned int>::size_type i = 0; i < idVector_.size(); ++i) {
        cout << " " << idVector_[i];
    }
    cout << endl;
    return EXIT_SUCCESS;
}

int Cv2xRxApp::parseOptions(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "hm:p:s:")) != -1) {
        switch (c) {
        case 'm':
            if (optarg) {
                auto mode = atoi(optarg);
                cout << "Set Rx mode " << mode << endl;
                rxMode_ = static_cast<RxModeType>(mode);
            }
            break;
        case 'p':
            if (optarg) {
                port_ = atoi(optarg);
                cout << "Set Rx port " << port_ << endl;
            }
            break;
        case 's':
            if (optarg) {
                if (EXIT_FAILURE == parseSidList(optarg)) {
                    return EXIT_FAILURE;
                }
            }
            break;
        case 'h':
        default:
            printUsage();
            return EXIT_FAILURE;
        }
    }

    // user must set SID/SIDs for SPECIFIC_SID/CATCHALL Rx mode
    if ((RxModeType::SPECIFIC_SID == rxMode_ or
        RxModeType::CATCHALL == rxMode_) and
        0 == idVector_.size()) {
        cerr << "No sid specified for Rx mode " << static_cast<int>(rxMode_) << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int Cv2xRxApp::init() {
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
    if (not cv2xRadioManager) {
        cerr << "failed to get Cv2xRadioManager!" << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            cerr << "C-V2X Radio Manager initialization failed!" << endl;
            return EXIT_FAILURE;
        }
    }

    // Get C-V2X status and make sure Rx is enabled
    promise<ErrorCode> p;
    Cv2xStatus cv2xStatus;
    auto ret = cv2xRadioManager->requestCv2xStatus(
        [&p, &cv2xStatus](Cv2xStatus status, ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                cv2xStatus = status;
            }
            p.set_value(error);
    });
    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Get C-V2X status failed!" << endl;
        return EXIT_FAILURE;
    }

    if (Cv2xStatusType::ACTIVE == cv2xStatus.rxStatus) {
        cout << "C-V2X RX status is active" << endl;
    } else {
        cerr << "C-V2X RX status is not active!" << endl;
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

    cv2xRadio_ = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);

    if (not cv2xRadio_) {
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

    // creat Rx buffer
    buf_ = (char *)malloc(G_BUF_LEN * sizeof(char));
    if (not buf_) {
        cerr << "malloc Rx buffer failed!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// For specific SID subscription, create a Tx flow with same SID and same
// port number as used for Rx flow registeration. If user wants to creat
// a SPS&Event Tx flow, set SPS port number to Rx port number.
int Cv2xRxApp::registerTxFlow() {
    promise<ErrorCode> p;
    shared_ptr<ICv2xTxFlow> txFlow = nullptr;
    auto ret = cv2xRadio_->createTxEventFlow(
        TrafficIpType::TRAFFIC_NON_IP,
        idVector_[0],
        port_,
        [&p, &txFlow](shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                txFlow = flow;
            }
            p.set_value(error);
        });

    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Create Tx flow failed!" << endl;
        return EXIT_FAILURE;
    }

    txFlow_ = txFlow;
    cout << "register Tx flow success" << endl;
    return EXIT_SUCCESS;
}

int Cv2xRxApp::registerRxFlow() {
    shared_ptr<std::vector<uint32_t>> idList = nullptr;
    if (0 < idVector_.size()) {
        // create SID list for SPECIFIC_SID or CATCHALL mode
        idList = make_shared<std::vector<uint32_t>>(idVector_);
    }

    promise<ErrorCode> p;
    shared_ptr<ICv2xRxSubscription> rxFlow = nullptr;
    auto ret = cv2xRadio_->createRxSubscription(
        TrafficIpType::TRAFFIC_NON_IP,
        port_,
        [&p, &rxFlow](shared_ptr<ICv2xRxSubscription> flow, ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                rxFlow = flow;
            }
            p.set_value(error);
        },
        idList);
    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Create Rx flow failed!" << endl;
        return EXIT_FAILURE;
    }
    rxFlow_ = rxFlow;
    cout << "register Rx flow success" << endl;

    // set 100ms timeout for Rx socket
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(rxFlow_->getSock(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "set Rx socket timeout failed!" << endl;
        deregisterRxFlow();
        return EXIT_FAILURE;
    }
    int flag = 1;
    if (setsockopt(rxFlow_->getSock(), IPPROTO_IPV6, IPV6_RECVTCLASS, &flag, sizeof(flag)) < 0) {
        cerr << "set sockopt(IPV6_RECVTCLASS) failed" << endl;
        deregisterRxFlow();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int Cv2xRxApp::deregisterTxFlow() {
    promise<ErrorCode> p;
    auto ret = cv2xRadio_->closeTxFlow(
        txFlow_,
        [&p](shared_ptr<ICv2xTxFlow> unused, ErrorCode error) {
            p.set_value(error);
        });
    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Deregister Tx flow failed!" << endl;
        return EXIT_FAILURE;
    }

    cout << "deregister Tx flow success" << endl;
    return EXIT_SUCCESS;
}

int Cv2xRxApp::deregisterRxFlow() {
    promise<ErrorCode> p;
    auto ret = cv2xRadio_->closeRxSubscription(
        rxFlow_,
        [&p](shared_ptr<ICv2xRxSubscription> unused, ErrorCode error) {
            p.set_value(error);
        });
    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Deregister Rx flow failed!" << endl;
        return EXIT_FAILURE;
    }

    cout << "deregister Rx flow success" << endl;
    return EXIT_SUCCESS;
}

int Cv2xRxApp::registerFlow() {
    // if Rx mode is SPECIFIC_SID, register Tx flow before Rx flow using
    // same port number
    if (rxMode_ == RxModeType::SPECIFIC_SID) {
        if (EXIT_SUCCESS != registerTxFlow()) {
            return EXIT_FAILURE;
        }
    }

    if (EXIT_SUCCESS != registerRxFlow()) {
        if (txFlow_) {
            deregisterTxFlow();
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xRxApp::deregisterFlow() {
    if (txFlow_) {
        deregisterTxFlow();
    }

    if (rxFlow_) {
        deregisterRxFlow();
    }
}

void Cv2xRxApp::deinit() {
    if (cv2xRadio_) {
        deregisterFlow();
        cv2xRadio_ = nullptr;
    }
    if (buf_) {
        free(buf_);
        buf_ = nullptr;
    }
}

int Cv2xRxApp::sampleTx(int length) {
    if (not txFlow_ or length <= 0) {
        return EXIT_FAILURE;
    }
    int rc = send(txFlow_->getSock(), buf_, length, 0);
    if (rc < 0) {
        cerr << "Error occurred sending to socket!" << endl;
        return EXIT_FAILURE;
    }

    ++txCount_;
    cout << "Transmitted " << rc << " bytes, count:" << txCount_ << endl;
    return EXIT_SUCCESS;
}

int Cv2xRxApp::sampleRx(int& length) {
    if (not rxFlow_) {
        cerr << "Rx flow not created!" << endl;
        return EXIT_FAILURE;
    }

    cout << "sampleRx( sock is " << rxFlow_->getSock() << ", port number is"
        << rxFlow_->getPortNum() << ")" << endl;

    // Attempt to read from socket
    struct sockaddr_in6 from;
    socklen_t fromLen = sizeof(from);
    struct msghdr message = {0};
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov[1] = {0};
    iov[0].iov_base = buf_;
    iov[0].iov_len = G_BUF_LEN;
    message.msg_name = &from;
    message.msg_namelen = fromLen;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    length = recvmsg(rxFlow_->getSock(), &message, 0);
    if (length < 0) {
        // only return failure if it's not socket recv timeout
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            cerr << "Error occurred reading from socket!" << endl;
            return EXIT_FAILURE;
        }
    } else {
        struct cmsghdr *cmsghp = CMSG_FIRSTHDR(&message);
        Priority priority = Priority::PRIORITY_UNKNOWN;
        if (cmsghp) {
            int tclass = 0;
            // get traffic class
            if (cmsghp->cmsg_level == IPPROTO_IPV6 &&
                cmsghp->cmsg_type == IPV6_TCLASS) {
                memcpy(&tclass, CMSG_DATA(cmsghp), sizeof(tclass));
                priority = Cv2xUtil::TrafficClassToPriority(tclass);
            }
        }
        ++rxCount_;
        cout << "Received " << length << " bytes, count:" << rxCount_
            << ",  priority " << static_cast<int>(priority) << endl;
    }

    return EXIT_SUCCESS;
}

static void signalHandler(int signum) {
    std::cout << " Interrupt signal (" << signum << ") received.." << std::endl;
    gExiting = true;
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X RX app" << endl;
    std::vector<std::string> groups{"system", "diag", "radio", "logd"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGTERM, signalHandler);

    Cv2xRxApp app;
    if (EXIT_SUCCESS != app.parseOptions(argc, argv)) {
        return EXIT_FAILURE;
    }

    do {
        if (EXIT_SUCCESS != app.init()) {
            break;
        }

        if (EXIT_SUCCESS != app.registerFlow()) {
            break;
        }

        cout << "start receiving..." << endl;
        while (not gExiting) {
            int len = 0;
            if (EXIT_SUCCESS != app.sampleRx(len)) {
                break;
            }

            // send back received packets if Rx mode is specific SID
            app.sampleTx(len);
        }
    } while(0);

    app.deinit();

    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
