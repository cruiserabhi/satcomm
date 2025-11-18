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
 * This application demonstrates how to use the card services APIs like
 * getting slot ids, applications, transmit APDU. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ICardManager instance from the PhoneFactory.
 * 3. Wait for the card service to become available.
 * 4. Get slot count.
 * 5. Get slot id.
 * 6. Get ICC card.
 * 7. Get applications supported by the card.
 * 8. Open a logical channel.
 * 9. Transmit APDU over logical channel.
 * 10. Close logical channel.
 * 11. Transmit APDU over basic channel.
 *
 * Usage:
 * # ./card_services_app
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
#include <telux/tel/CardManager.hpp>

#include <telux/tel/CardApp.hpp>

/* Card events */
enum class CardEvent {
   OPEN_LOGICAL_CHANNEL = 1,  /* Open Logical channel */
   CLOSE_LOGICAL_CHANNEL = 2, /* Close Logical channel */
   TRANSMIT_APDU_CHANNEL = 3  /* Transmit of APDU on channel */
};

class CardListener : public telux::tel::ICardChannelCallback,
                     public telux::tel::ICardCommandCallback,
                     public telux::common::ICommandResponseCallback,
                     public std::enable_shared_from_this<CardListener> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        cardMgr_ = phoneFactory.getCardManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!cardMgr_) {
            std::cout << "Can't get ICardManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Card service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int getSlotIdAndGetCard() {
        int slotCount = 0;
        std::vector<int> slotIds;
        telux::common::Status status;

        /* Step - 4 */
        status = cardMgr_->getSlotCount(slotCount);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't get slot count, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "slot count: " << slotCount << std::endl;

        /* Step - 5 */
        status = cardMgr_->getSlotIds(slotIds);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't get slot Ids, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Slot Ids: { ";
        for(auto id : slotIds) {
            std::cout << id << " ";
        }
        std::cout << "}" << std::endl;

        /* Step - 6 */
        card_ = cardMgr_->getCard(slotIds.front(), &status);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't create card, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int getSupportedApplications() {
        telux::common::Status status;

        /* Step - 7 */
        applications_ = card_->getApplications(&status);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't get supported application, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!applications_.size()) {
            std::cout << "No applications" << std::endl;
            return 0;
        }

        std::cout << "\nFound applications:" << std::endl;
        for (auto cardApp : applications_) {
            std::cout << "AppId: " << cardApp->getAppId() << std::endl;
        }

        return 0;
    }

    int logicalChannelOpen() {
        std::string aid;
        telux::common::Status status;

        for(auto app : applications_) {
            if (app->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
                aid = app->getAppId();
                break;
            }
        }

        /* Step - 8 */
        status = card_->openLogicalChannel(aid, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't open channel, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForCardEvent(CardEvent::OPEN_LOGICAL_CHANNEL) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to open channel" << std::endl;
            return -EIO;
        }

        std::cout << "Opened logical channel\n" << std::endl;
        return 0;
    }

    int logicalChannelClose() {
        telux::common::Status status;

        /* Step - 10 */
        status = card_->closeLogicalChannel(openedChannel_, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't close channel, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForCardEvent(CardEvent::CLOSE_LOGICAL_CHANNEL) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to close channel" << std::endl;
            return -EIO;
        }

        std::cout << "Closed logical channel\n" << std::endl;
        return 0;
    }

    int txAPDULogicalChannel() {
        telux::common::Status status;

        /* Sample SAP APDU to open master file */
        /* APDU Command - 00 A4 00 04 02 3F 00 */
        const uint8_t CLA = 0;
        const uint8_t INSTRUCTION = 164;
        const uint8_t P1 = 0;
        const uint8_t P2 = 4;
        const uint8_t P3 = 2;
        const std::vector<uint8_t> DATA = {63, 0};

        /* Step - 9 */
        status = card_->transmitApduLogicalChannel(
            openedChannel_, CLA, INSTRUCTION, P1, P2, P3, DATA, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't transmit logical APDU, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForCardEvent(CardEvent::TRANSMIT_APDU_CHANNEL) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to transmit logical APDU" << std::endl;
            return -EIO;
        }

        std::cout << "Logical APDU transmitted\n" << std::endl;
        return 0;
    }

    int txAPDUBasicChannel() {
        telux::common::Status status;

        /* Sample SAP APDU to open Master File */
        /* APDU Command - 00 A4 00 04 02 3F 00 */
        const uint8_t CLA = 0;
        const uint8_t INSTRUCTION = 164;
        const uint8_t P1 = 0;
        const uint8_t P2 = 4;
        const uint8_t P3 = 2;
        const std::vector<uint8_t> DATA = {63, 0};

        /* Step - 11 */
        status = card_->transmitApduBasicChannel(
            CLA, INSTRUCTION, P1, P2, P3, DATA, shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't transmit basic APDU, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForCardEvent(CardEvent::TRANSMIT_APDU_CHANNEL) ||
            (errorCode_ != telux::common::ErrorCode::SUCCESS)) {
            std::cout << "Failed to transmit basic APDU" << std::endl;
            return -EIO;
        }

        std::cout << "Basic APDU transmitted\n" << std::endl;
        return 0;
    }

    bool waitForCardEvent(CardEvent cardEvent) {
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

    void onChannelResponse(int channel, telux::tel::IccResult result,
            telux::common::ErrorCode error) override {
        std::lock_guard<std::mutex> lock(eventMutex_);
        std::cout << "onChannelResponse()" << std::endl;
        std::cout << "Error: " << static_cast<int>(error) << std::endl;
        std::cout << "ICC result: " << result.toString() << std::endl;
        std::cout << "Channel: " << channel << std::endl;
        openedChannel_ = channel;
        errorCode_ = error;
        eventCV_.notify_one();
    }

    void commandResponse(telux::common::ErrorCode error) override {
        std::lock_guard<std::mutex> lock(eventMutex_);
        std::cout << "commandResponse()" << std::endl;
        std::cout << "Error: " << static_cast<int>(error) << std::endl;
        errorCode_ = error;
        eventCV_.notify_one();
    }

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
    CardEvent eventExpected_;
    int openedChannel_ = -1;
    telux::common::ErrorCode errorCode_;
    std::shared_ptr<telux::tel::ICard> card_;
    std::vector<std::shared_ptr<telux::tel::ICardApp>> applications_;
    std::shared_ptr<telux::tel::ICardManager> cardMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<CardListener> app;

    try {
        app = std::make_shared<CardListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate CardListener" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getSlotIdAndGetCard();
    if (ret < 0) {
        return ret;
    }

    ret = app->getSupportedApplications();
    if (ret < 0) {
        return ret;
    }

    ret = app->logicalChannelOpen();
    if (ret < 0) {
        return ret;
    }

    ret = app->txAPDULogicalChannel();
    if (ret < 0) {
        app->logicalChannelClose();
        return ret;
    }

    ret = app->logicalChannelClose();
    if (ret < 0) {
        return ret;
    }

    ret = app->txAPDUBasicChannel();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nCard app exiting" << std::endl;
    return 0;
}
