/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This application demonstrates how to make a call. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ICallManager instance from the PhoneFactory.
 * 3. Wait for the call manager service to become available.
 * 4. Register call listener to get notification of events from call Manager.
 * 5. Trigger a Real Time Text (RTT) call.
 * 6. Receive status of the call in callback.
 * 7. Wait while the call to be in active state.
 * 8. Send text message to remote party.
 * 9. Receive status of the sendRtt in callback.
 * 10. Wait while the call is in progress.
 * 11. Finally, when the use case is over, hangup the call.
 *
 * Usage:
 * # ./make_rtt_call_app
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
#include <telux/tel/CallListener.hpp>

std::promise<bool> callInfoChange{};

// To get call state change notifications
class MyCallListener : public telux::tel::ICallListener {
 public:
    void onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {
       std::cout << " Call State: " << (int)(call->getCallState()) << std::endl;
        if(call->getCallState() == telux::tel::CallState::CALL_ACTIVE) {
            std::cout << " Call State is ACTIVE " << std::endl;
            callInfoChange.set_value(true);
        }
    }
};

// Response callback function for sendRtt
/* Step - 9 */
void sendRttMessageResponse(telux::common::ErrorCode error) {
    if(error == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Send RTT data request is successful \n ";
    } else {
        std::cout << " Send RTT data request request failed with error ";
    }
}

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
        } else {
            /* Step - 4 */
            callListener_ = std::make_shared<MyCallListener>();
            // registering listener
            telux::common::Status status = callMgr_->registerListener(callListener_);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register Call Manager listener" << std::endl;
                return false;
            }
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int triggerCall() {
        telux::common::Status status;
        int phoneId = DEFAULT_PHONE_ID;
        std::string phoneNumber = "6666";

        /* Step - 5 */
        status = callMgr_->makeRttCall(phoneId, phoneNumber, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't call, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Call initiated" << std::endl;
        return 0;
    }

    int sendMessage() {
        telux::common::Status status;
        int phoneId = DEFAULT_PHONE_ID;
        std::string message = "Hello World";

        /* Step - 8 */
        status = callMgr_->sendRtt(phoneId, message, sendRttMessageResponse);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't send message, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Real Time Text is sent " << std::endl;
        return 0;
    }

    int terminateCall() {
        telux::common::Status status;

        /* Step - 10 */
        status = dialedCall_->hangup();
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Failed to hangup, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Call termination initiated" << std::endl;
        return 0;
    }

    /* Step - 6 */
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
            " number " << call->getRemotePartyNumber() << " rtt mode of call" <<
            (int)call->getRttMode() << std::endl;
    }

 private:
    std::shared_ptr<telux::tel::ICall> dialedCall_;
    std::shared_ptr<telux::tel::ICallManager> callMgr_;
    std::shared_ptr<telux::tel::ICallListener> callListener_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<CallMaker> app;
    bool isCallActive = false;

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
    // Wait for call state to change to CallState::ACTIVE
    /* Step - 7 */
    isCallActive = callInfoChange.get_future().get();
    if(isCallActive == true) {
        ret = app->sendMessage();
        if (ret < 0) {
            return ret;
        }
    }

    /* Step - 10 */
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
