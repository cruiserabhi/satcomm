/*
 *  Copyright (c) 2017-2019, 2021 The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to send a SMS messages and receive
 * delivery status. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ISmsManager instance from the PhoneFactory.
 * 3. Wait for the SMS service to become available.
 * 4. Send SMS message.
 * 5. Receive message sent status.
 * 6. Receive delivery sent status.
 *
 * Usage:
 * # ./send_sms_app <config-parser>
 *
 * Configuration file (<config-parser>) is optional. The message and phone numbers
 * can be either defined as preprocessor macros in this file or passed through a
 * configuration file.
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/SmsManager.hpp>

#include "ConfigParser.hpp"

#define DEFAULT_RECEIVER_PHONE_NUMBER "+1xxxxxxxxxx"
#define DEFAULT_MESSAGE "Default test msg"

class SMSSentStatusReceiver : public telux::common::ICommandResponseCallback {
 public:
    /* Step - 5 */
    void commandResponse(telux::common::ErrorCode ec) override {
        if(ec == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Message sent successfully" << std::endl;
            return;
        }
        std::cout << "Can't send msg, err " << static_cast<int>(ec) << std::endl;
    }
};

class SMSDeliveryStatusReceiver : public telux::common::ICommandResponseCallback {
 public:
    /* Step - 6 */
    void commandResponse(telux::common::ErrorCode ec) override {
        if(ec == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Message delivered successfully" << std::endl;
            return;
        }
        std::cout << "Can't deliver msg, err " << static_cast<int>(ec) << std::endl;
    }
};

class SMSSender : public std::enable_shared_from_this<SMSSender> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        smsManager_ = phoneFactory.getSmsManager(DEFAULT_SLOT_ID,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!smsManager_) {
            std::cout << "Can't get ISMSManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "SMS service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int sendMessage(std::shared_ptr<ConfigParser> configParser) {
        telux::common::Status status;
        std::string message;
        std::string receiverAddress;
        std::shared_ptr<telux::common::ICommandResponseCallback> smsSentCb;
        std::shared_ptr<telux::common::ICommandResponseCallback> smsDeliveryCb;

        try {
            smsSentCb = std::make_shared<SMSSentStatusReceiver>();
            smsDeliveryCb = std::make_shared<SMSDeliveryStatusReceiver>();
        } catch (const std::exception& e) {
            std::cout << "Can't allocate msg status receiver" << std::endl;
            return -ENOMEM;
        }

        message = configParser->getValue(std::string("MESSAGE"));
        receiverAddress = configParser->getValue(std::string("RECEIVER_NUMBER"));

        if (receiverAddress.empty() || message.empty()) {
            receiverAddress = DEFAULT_RECEIVER_PHONE_NUMBER;
            message = DEFAULT_MESSAGE;
            std::cout << "Using default phone number" << std::endl;
        }

        /* Step - 4 */
        status = smsManager_->sendSms(message, receiverAddress, smsSentCb, smsDeliveryCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't send message, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        /* Wait for SMS message sent and delivery status, app specific logic goes here */
        /* This wait is just an example */
        std::this_thread::sleep_for(std::chrono::minutes(1));

        return 0;
    }

 private:
    std::shared_ptr<telux::tel::ISmsManager> smsManager_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<SMSSender> app;
    std::shared_ptr<ConfigParser> configParser;

    try {
        app = std::make_shared<SMSSender>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate SMSSender" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    if (argc == 2) {
        configParser = std::make_shared<ConfigParser>(argv[1]);
    } else {
        configParser = std::make_shared<ConfigParser>();
    }

    ret = app->sendMessage(configParser);
    if (ret < 0) {
        return ret;
    }

    std::cout << "Send sms app exiting" << std::endl;
    return 0;
}
