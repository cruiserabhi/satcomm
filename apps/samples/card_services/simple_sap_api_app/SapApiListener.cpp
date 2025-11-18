/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This application demonstrates how to use SAP card services APIs like
 * getting slot ids, applications, ATR and transmit APDU. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ISapCardManager instance from the PhoneFactory.
 * 3. Wait for the SAP service to become available.
 * 4. Open SAP connection.
 * 5. Wait for connection to get open.
 * 6. Request ATR.
 * 7. Wait for request ATR response.
 * 8. Transmit APDU.
 * 9. Wait for APDU getting transmitted.
 * 10. Close SAP connection.
 * 11. Wait for SAP connection to get closed.
 *
 * Usage:
 * # ./simple_sap_api_app
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/SapCardManager.hpp>

/* SAP events */
enum SapEvent {
   OPEN_SAP_CONNECTION = 1,  /* SAP Open connection */
   CLOSE_SAP_CONNECTION = 2, /* SAP disconnection */
   SAP_GET_ATR = 3,          /* SAP Answer To Reset */
   SAP_TRANSMIT_APDU = 4     /* Transmit of APDU in SAP mode */
};

class SAPListener : public telux::tel::ISapCardCommandCallback,
                    public telux::tel::IAtrResponseCallback,
                    public telux::common::ICommandResponseCallback,
                    public std::enable_shared_from_this<SAPListener> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        sapCardMgr_ = phoneFactory.getSapCardManager(DEFAULT_SLOT_ID,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!sapCardMgr_) {
            std::cout << "Can't get ISapCardManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "SAP service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int sapOpenConnection() {
        telux::common::Status status;

        /* Step - 4 */
        status = sapCardMgr_->openConnection(
            telux::tel::SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't open SAP connection, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForSapEvent(SapEvent::OPEN_SAP_CONNECTION) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to open SAP connection" << std::endl;
            return -EIO;
        }

        std::cout << "Opened SAP connection\n" << std::endl;
        return 0;
    }

    int sapCloseConnection() {
        telux::common::Status status;

        /* Step - 10 */
        status = sapCardMgr_->closeConnection(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't close SAP connection, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForSapEvent(SapEvent::CLOSE_SAP_CONNECTION) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to close SAP connection" << std::endl;
            return -EIO;
        }

        std::cout << "Closed SAP connection\n" << std::endl;
        return 0;
    }

    int requestATR() {
        telux::common::Status status;

        /* Step - 6 */
        status = sapCardMgr_->requestAtr(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request ATR, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForSapEvent(SapEvent::SAP_GET_ATR) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to request ATR" << std::endl;
            return -EIO;
        }

        std::cout << "ATR requested\n" << std::endl;
        return 0;
    }

    int transmitAPDU() {
        telux::common::Status status;

        /* Sample SAP APDU to open master file */
        /* APDU Command - 00 A4 00 04 02 3F 00 */
        const uint8_t CLA = 0;
        const uint8_t INSTRUCTION = 164;
        const uint8_t P1 = 0;
        const uint8_t P2 = 4;
        const uint8_t LC = 2;
        const std::vector<uint8_t> DATA = {63, 0};

        /* Step - 8 */
        status = sapCardMgr_->transmitApdu(
            CLA, INSTRUCTION, P1, P2, LC, DATA, 0, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't transmit APDU, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForSapEvent(SapEvent::SAP_TRANSMIT_APDU) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to transmit APDU" << std::endl;
            return -EIO;
        }

        std::cout << "APDU transmitted\n" << std::endl;
        return 0;
    }

    bool waitForSapEvent(SapEvent sapEvent) {
        int const DEFAULT_TIMEOUT_SECONDS = 5;
        std::unique_lock<std::mutex> lock(eventMutex_);

        auto cvStatus = eventCV_.wait_for(lock,
            std::chrono::seconds(DEFAULT_TIMEOUT_SECONDS));

        if (cvStatus == std::cv_status::timeout) {
            std::cout << "Timedout" << std::endl;
            errorCode_ = telux::common::ErrorCode::TIMEOUT_ERROR;
            return false;
        }

        return true;
    }

    /* Step - 5,11 */
    void commandResponse(telux::common::ErrorCode error) override {
        std::lock_guard<std::mutex> lock(eventMutex_);
        std::cout << "commandResponse()" << std::endl;
        std::cout << "Error: " << static_cast<int>(error) << std::endl;
        errorCode_ = error;
        eventCV_.notify_one();
    }

    /* Step - 7 */
    void atrResponse(std::vector<int> responseAtr, telux::common::ErrorCode error) override {
        std::lock_guard<std::mutex> lock(eventMutex_);
        std::cout << "atrResponse()" << std::endl;
        std::cout << "Error: " << static_cast<int>(error) << std::endl;

        if(eventExpected_ == SapEvent::SAP_GET_ATR) {
            std::cout << "\tATR.data:";
            for(int val : responseAtr) {
                std::cout << " " << val;
            }
        }

        errorCode_ = error;
        eventCV_.notify_one();
    }

    /* Step - 9 */
    void onResponse(telux::tel::IccResult result, telux::common::ErrorCode error) override {
        std::lock_guard<std::mutex> lock(eventMutex_);
        std::cout << "onResponse()" << std::endl;
        std::cout << "Error: " << static_cast<int>(error) << std::endl;
        std::cout << "ICC result: " << result.toString() << std::endl;
        errorCode_ = error;
        eventCV_.notify_one();
    }

 private:
    std::mutex eventMutex_;
    std::condition_variable eventCV_;
    SapEvent eventExpected_;
    telux::common::ErrorCode errorCode_;
    std::shared_ptr<telux::tel::ISapCardManager> sapCardMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<SAPListener> app;

    try {
        app = std::make_shared<SAPListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate SAPListener" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->sapOpenConnection();
    if (ret < 0) {
        return ret;
    }

    ret = app->requestATR();
    if (ret < 0) {
        app->sapCloseConnection();
        return ret;
    }

    ret = app->transmitAPDU();
    if (ret < 0) {
        app->sapCloseConnection();
        return ret;
    }

    ret = app->sapCloseConnection();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nSAP app exiting" << std::endl;
    return 0;
}
