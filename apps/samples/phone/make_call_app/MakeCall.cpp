/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to make a call. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ICallManager instance from the PhoneFactory.
 * 3. Wait for the call manager service to become available.
 * 4. Trigger a call.
 * 5. Receive status of the call in callback.
 * 6. Wait while the call is in progress.
 * 7. Finally, when the use case is over, hangup the call.
 *
 * Usage:
 * # ./make_call_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/CallManager.hpp>

class CallMaker : public telux::tel::IMakeCallCallback,
                public std::enable_shared_from_this<CallMaker> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        callMgr_ = phoneFactory.getCallManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!callMgr_) {
            std::cout << "Can't get ICallManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Call manager service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int triggerCall() {
        telux::common::Status status;
        int phoneId = DEFAULT_PHONE_ID;
        std::string phoneNumber = "+1xxxxxxxxxx";

        /* Step - 4 */
        status = callMgr_->makeCall(phoneId, phoneNumber, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't call, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Call initiated" << std::endl;
        return 0;
    }

    int terminateCall() {
        telux::common::Status status;

        /* Step - 7 */
        status = dialedCall_->hangup();
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Failed to hangup, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Call termination initiated" << std::endl;
        return 0;
    }

    /* Step - 5 */
    void makeCallResponse(telux::common::ErrorCode ec,
            std::shared_ptr<telux::tel::ICall> call) override {
        std::cout << "makeCallResponse()" << std::endl;

        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to call, err " << static_cast<int>(ec) << std::endl;
            return;
        }

        dialedCall_ = call;

        std::cout << "Index " << call->getCallIndex() <<
            " direction " << static_cast<int>(call->getCallDirection()) <<
            " number " << call->getRemotePartyNumber() << std::endl;
    }

 private:
    std::shared_ptr<telux::tel::ICall> dialedCall_;
    std::shared_ptr<telux::tel::ICallManager> callMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<CallMaker> app;

    try {
        app = std::make_shared<CallMaker>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate CallMaker" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->triggerCall();
    if (ret < 0) {
        return ret;
    }

    /* Step - 6 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::minutes(1));

    ret = app->terminateCall();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses */
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "\nMake call app exiting" << std::endl;
    return 0;
}
