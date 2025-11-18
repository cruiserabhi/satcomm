/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023,2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xTxReportSampleApp.cpp
 *
 * @brief: Application that demonstrates how to enable/disable CV2X Tx status report.
 */

#include <iostream>
#include <future>
#include <cstring>
#include <mutex>
#include <memory>
#include <stdlib.h>
#include <sys/time.h>
#include <csignal>

#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include "Cv2xTxReportSampleApp.hpp"
#include "../../../common/utils/Utils.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::promise;
using std::string;
using std::mutex;
using std::make_shared;
using std::shared_ptr;
using std::lock_guard;
using std::condition_variable;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::TxStatusReport;

#define DEFAULT_PORT (5000)
#define DEFAULT_LENGTH (200)
#define DEFAULT_INTERVAL (100)
#define DEFAULT_SERVICE_ID (1)

class Cv2xTxStatusReportListener : public ICv2xTxStatusReportListener {
public:
    void onTxStatusReport(const TxStatusReport & info) {
        cout << "Recv Tx report:";
        cout << "Ota:" << info.otaTiming;
        cout << ", rf0 status:" << static_cast<int>(info.rfInfo[0].status);
        cout << ", rf0 tx pwr(10dBm):" << info.rfInfo[0].power;
        cout << ", rf1 status:" << static_cast<int>(info.rfInfo[1].status);
        cout << ", rf1 tx pwr(10dBm):" << info.rfInfo[1].power;
        cout << ", txType:" << static_cast<int>(info.txType);
        cout << ", segType:" << static_cast<int>(info.segType);
        cout << ", segNum:" << +info.segNum << endl;
    }
};

Cv2xTxStatusReportApp::Cv2xTxStatusReportApp() {
    cout << "Running CV2X Tx Report Sample App"<< endl;
}

int Cv2xTxStatusReportApp::init() {
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

    // get handle of cv2x radio manager and wait for readiness
    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto radioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!radioMgr) {
        cerr << "Failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            cerr << "Cv2x Radio Manager initialization failed!" << endl;
            return EXIT_FAILURE;
        }
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

    auto radio = radioMgr->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);
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
        }
    }

    // get initial CV2X status
    promise<Cv2xStatus> prom;
    auto res = radioMgr->requestCv2xStatus([&prom](Cv2xStatus status, ErrorCode code)
                                                    {
                                                        prom.set_value(status);
                                                    });
    if (Status::SUCCESS != res) {
        cerr << "Request for Cv2x status failed!" << endl;
        return EXIT_FAILURE;
    };

    // ensure cv2x active before running the test
    Cv2xStatus status = prom.get_future().get();
    if (status.txStatus != Cv2xStatusType::ACTIVE and
        status.rxStatus != Cv2xStatusType::ACTIVE) {
        cerr << "CV2X Tx/Rx status not active!" << endl;
        return EXIT_FAILURE;
    }

    // alloc buffer for Tx pkt
    buf_ = (char*)malloc(DEFAULT_LENGTH * sizeof(char));
    if (!buf_) {
        cerr << "Alloc Tx buffer failed!" << endl;
        return EXIT_FAILURE;
    }

    // register Tx flow
    if (EXIT_SUCCESS != registerTxFlow(radio)) {
        // delete created listener if Tx flow registration failed
        deleteTxReportListener(radio);
        return EXIT_FAILURE;
    }

    // create listener with same port number as the Tx flow src port
    if (EXIT_SUCCESS != createTxReportListener(radio)) {
        return EXIT_FAILURE;
    }

    radio_ = radio;
    return EXIT_SUCCESS;
}

int Cv2xTxStatusReportApp::deinit() {
    cout << "Exiting..." << endl;

    // deregister Tx flow
    deregisterTxFlow(radio_);

    // deregister report listener
    deleteTxReportListener(radio_);

    // free allocated tx buffer
    if (buf_) {
        free(buf_);
        buf_ = nullptr;
    }
    if (radio_) {
        radio_ = nullptr;
    }

    exit(0);
}

int Cv2xTxStatusReportApp::registerTxFlow(std::shared_ptr<ICv2xRadio> &radio) {
    cout << "Registering Tx event Flow" << endl;

    promise<ErrorCode> p;
    shared_ptr<ICv2xTxFlow>txFlow = nullptr;
    auto createTxEventFlowCallback = [&p, &txFlow](shared_ptr<ICv2xTxFlow> txEventFlow,
                                        ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            txFlow = txEventFlow;
        }
        p.set_value(error);
    };

    EventFlowInfo flowInfo;
    auto status = radio->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                        DEFAULT_SERVICE_ID,
                                        flowInfo,
                                        DEFAULT_PORT,
                                        createTxEventFlowCallback);

    if (Status::SUCCESS != status or
        ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Failed to create Tx flow!" << endl;
        return EXIT_FAILURE;
    }

    txFlow_ = txFlow;
    txFlowValid_ = true;
    cout << "Succeeded in creating Tx Flow, create sock:" << txFlow_->getSock();
    cout << " , port:"<< DEFAULT_PORT << endl;
    return EXIT_SUCCESS;
}

int Cv2xTxStatusReportApp::deregisterTxFlow(std::shared_ptr<ICv2xRadio> &radio) {
    int ret = EXIT_SUCCESS;
    if (radio && txFlowValid_) {
        cout << "Deregistering Tx flow, close sock:" << txFlow_->getSock() << endl;

        promise<ErrorCode> p;
        auto closeTxFlowCallback = [&p](shared_ptr<ICv2xTxFlow> txFlow, ErrorCode error) {
            p.set_value(error);
        };

        auto status = radio->closeTxFlow(txFlow_, closeTxFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != p.get_future().get()) {
            cerr << "Failed to deregister Tx flow!" << endl;
            ret = EXIT_FAILURE;
        }
        txFlowValid_ = false;
    }
    return ret;
}

// Fills Tx buffer using same sequence as in acme
int Cv2xTxStatusReportApp::fillTxBuffer(char* buf, uint16_t length) {
    if (!buf or length < 6) {
        cerr << "Invalid Tx Buffer!" << endl;
        return EXIT_FAILURE;
    }

    uint16_t len = 0;
    memset(buf, 0, length);

    // Very first payload is test magic number
    buf[0] = 'Q';
    len++;

    // reserve 2 bytes for non-dummy payload data size
    len += sizeof(uint16_t);

    // UEID value
    buf[len] = 1;
    len++;

    // Sequence number
    *(uint16_t *)(&buf[len]) = htons(txCount_);
    len += sizeof(uint16_t);

    // Add timestamp if buffer size allowed
    uint64_t timestamp = Utils::getCurrentTimestamp();
    char format[] = "<%llu> ";
    uint16_t tmp = snprintf(nullptr, 0, format, timestamp);
    if (tmp + len <= length) {
        len += snprintf(buf+len, tmp, format, timestamp);
    }

    // Fill non-dummy message length
    tmp = htons(len);
    memcpy(buf+1, &tmp, sizeof(uint16_t));

    // Dummy payload
    for (int i = len; i < length; ++i) {
        buf[i] = 'a' + (i % 26);
    }

    return EXIT_SUCCESS;
}

// Function for transmitting data
 int Cv2xTxStatusReportApp::sampleTx(int sock, char* buf, uint16_t length) {
    // Send data using sendmsg to provide IPV6_TCLASS per packet
    struct msghdr message = { 0 };
    struct iovec iov[1] = { 0 };
    struct cmsghdr *cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];
    iov[0].iov_base = buf;
    iov[0].iov_len = length;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    // Fill ancillary data
    int priority = 3;
    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsghp), &priority, sizeof(int));

    // Send data
    auto bytes = sendmsg(sock, &message, 0);

    // Check bytes sent
    if (bytes <= 0) {
        cerr << "Error occurred sending to sock:" << sock << " err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    ++txCount_;
    cout << "TX count: " << txCount_ << " bytes:" << bytes << endl;
    return EXIT_SUCCESS;
}

void Cv2xTxStatusReportApp::startTxPkts() {
    cout << "Start Tx..." << endl;

    while (1) {
        {
            if (not txFlowValid_) {
                cout << "Tx flow has been deregistered" << endl;
                break;
            }

            if (fillTxBuffer(buf_, DEFAULT_LENGTH) or
                sampleTx(txFlow_->getSock(), buf_, DEFAULT_LENGTH)) {
                break;
            }
        }

        usleep(DEFAULT_INTERVAL*1000);
    }
}

int Cv2xTxStatusReportApp::createTxReportListener(std::shared_ptr<ICv2xRadio> &radio) {
    promise<ErrorCode> p;
    txReportListener_ = make_shared<Cv2xTxStatusReportListener>();
    auto status = radio->registerTxStatusReportListener(
        DEFAULT_PORT,
        txReportListener_,
        [&p](ErrorCode code)
        {
            p.set_value(code);
        });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Register CV2X Tx status report listener failed!" << endl;
        return EXIT_FAILURE;
    }
    cout << "Start listening to Tx Status Report..." << endl;

    return EXIT_SUCCESS;
}

int Cv2xTxStatusReportApp::deleteTxReportListener(std::shared_ptr<ICv2xRadio> &radio) {
    if (not radio || not txReportListener_) {
        cerr << "Tx status report listener not exist" << endl;
        return EXIT_FAILURE;
    }

    cout << "Stop listening to Tx Status Report" << endl;
    promise<ErrorCode> p;
    auto status = radio->deregisterTxStatusReportListener(
        DEFAULT_PORT,
        [&p](ErrorCode code)
        {
            p.set_value(code);
        });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Deregister CV2X Tx status report listener failed!" << endl;
        return EXIT_FAILURE;
    }

    txReportListener_ = nullptr;
    return EXIT_SUCCESS;
}

Cv2xTxStatusReportApp & Cv2xTxStatusReportApp::getInstance() {
    static Cv2xTxStatusReportApp instance;
    return instance;
}

static void signalHandler(int signum) {
    std::cout << " Interrupt signal (" << signum << ") received.." << std::endl;
    Cv2xTxStatusReportApp::getInstance().deinit();
}

int main(int argc, char *argv[]) {
    std::vector<std::string> groups{"system", "diag", "radio", "logd"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    auto & app = Cv2xTxStatusReportApp::getInstance();
    if (EXIT_SUCCESS != app.init()){
        cout << "Error: Initialization failed!" << endl;
        return EXIT_FAILURE;
    }

    signal(SIGINT, signalHandler);

    // start Tx packets
    app.startTxPkts();

    // release radio resources when exit from mainloop
    app.deinit();
}
