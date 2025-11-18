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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file    CardControl.cpp
 * @brief   This file contains the implementation for Card Control functions.
 */

#include "CardControl.hpp"
#include "RemoteSimUtils.hpp"
#include "SimConnection.hpp"

#define APDU_MSG_MIN_LEN 6
#define APDU_MSG_CASE_3_LEN 8
#define SUBSYSTEM_READY_TIMEOUT_SEC 5
#define ASYNC_RESPONSE_TIMEOUT_SEC 5

using namespace telux::common;
using namespace telux::tel;

void CardListener::onCardInfoChanged(int slotId)
{
    LOGD("Received card info changed notification for slot %d.\n", slotId);

    Status status;
    CardState cardState;

    std::shared_ptr<ICard> card = CardControl::getInstance().getSimCard(slotId, &status);
    if (status != Status::SUCCESS) {
        LOGE("Getting card from CardManager failed!\n");
        return;
    }

    if (card->getState(cardState) != Status::SUCCESS) {
        LOGE("Getting card state failed!\n");
        return;
    }

    if (cardState == CardState::CARDSTATE_ABSENT) {
        LOGD("Card was removed.\n");
        CardControl::getInstance().setCardPresent(false);
        CardControl::getInstance().setCardConnected(false);
        SimConnection::getInstance().sendCardRemovedMessage();
    } else if (cardState == CardState::CARDSTATE_PRESENT) {
        LOGD("Card was inserted.\n");
        CardControl::getInstance().setCardPresent(true);
        CardControl::getInstance().connectAfterCardInsertion();
    } else {
        LOGD("Card state = %d.\n", static_cast<int>(cardState));
    }
}

void CardListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        LOGD("Received Service Unavailable notification.\n");
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Received Service Available notification.\n");
    } else {
        LOGD("Received unknown service status notification.\n");
    }
}

ApduResponseCallback::ApduResponseCallback(uint8_t apduId)
{
    apduId_ = apduId;
}

void ApduResponseCallback::onResponse(IccResult result, ErrorCode error)
{
    LOGD("Received APDU response from modem with errorcode %d.\n", static_cast<int>(error));

    SimConnection::getInstance().sendApduResponse(result.data, apduId_);
    CardControl::getInstance().eraseFromApduRespCbMap(apduId_);
}

void OpenConnectionCallback::commandResponse(ErrorCode errorCode)
{
    LOGD("Received open connection response from modem with errorcode %d.\n",
         static_cast<int>(errorCode));

    if (errorCode == ErrorCode::SUCCESS) {
        CardControl::getInstance().setOpenConnPromiseValue(true);
    } else {
        CardControl::getInstance().setOpenConnPromiseValue(false);
    }
}

void CloseConnectionCallback::commandResponse(ErrorCode errorCode)
{
    LOGD("Received close connection response from modem with errorcode %d.\n",
         static_cast<int>(errorCode));
}

void PowerOnCallback::commandResponse(ErrorCode errorCode)
{
    LOGD("Received power on response from modem with errorcode %d.\n", static_cast<int>(errorCode));

    if (errorCode == ErrorCode::SUCCESS) {
        CardControl::getInstance().setPowerOnPromiseValue(true);
    } else {
        CardControl::getInstance().setPowerOnPromiseValue(false);
    }
}

void PowerOffCallback::commandResponse(ErrorCode errorCode)
{
    LOGD("Received power off response from modem with errorcode %d.\n",
         static_cast<int>(errorCode));
}

void ResetCallback::commandResponse(ErrorCode errorCode)
{
    LOGD("Received reset response from modem with errorcode %d.\n", static_cast<int>(errorCode));

    if (errorCode == ErrorCode::SUCCESS) {
        CardControl::getInstance().requestAtrAfterReset();
    }
}

AtrResponseCallback::AtrResponseCallback(uint8_t msgType)
{
    msgType_ = msgType;
}

void AtrResponseCallback::atrResponse(std::vector<int> responseAtr, ErrorCode error)
{
    LOGD("Received AtR response from modem with errorcode %d.\n", static_cast<int>(error));

    if (msgType_ == CARD_RESET_MSG) {
        SimConnection::getInstance().sendCardResetResponse(responseAtr);
    } else if (msgType_ == CARD_INSERTED_MSG) {
        SimConnection::getInstance().sendCardInsertedMessage(responseAtr);
    } else {
        LOGE("Unkown Answer to Reset message type!\n");
    }
}

CardControl & CardControl::getInstance()
{
    static CardControl instance;
    return instance;
}

Status CardControl::init()
{
    std::promise<ServiceStatus> prom;
    auto &phoneFactory = PhoneFactory::getInstance();
    sapCardMgr_ = phoneFactory.getSapCardManager(
        DEFAULT_SLOT_ID,[&](telux::common::ServiceStatus status) {
        prom.set_value(status);
    });
    if (!sapCardMgr_) {
        LOGE("ERROR - Failed to get SapCardManager instance \n");
        return Status::FAILED;
    }
    telux::common::ServiceStatus sapCardMgrStatus = sapCardMgr_->getServiceStatus();
    if (sapCardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("SapCardManager subsystem is not ready , Please wait\n");
    }
    sapCardMgrStatus = prom.get_future().get();
    if (sapCardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("SapCardManager subsystem is ready\n");
    } else {
        LOGE("ERROR - Unable to initialize SapCardManager subsystem\n");
        return Status::FAILED;
    }

    std::promise<telux::common::ServiceStatus> cardMgrprom;
    cardMgr_ = PhoneFactory::getInstance().
        getCardManager([&](telux::common::ServiceStatus status) {
        cardMgrprom.set_value(status);
    });
    if (!cardMgr_) {
        LOGE("Failed to create CardManager!\n");
        return Status::FAILED;
    }

    telux::common::ServiceStatus cardMgrStatus = cardMgr_->getServiceStatus();
    if (cardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Card subsystem not ready yet, waiting...\n");
    }
    cardMgrStatus = cardMgrprom.get_future().get();
    if (cardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Card subsystem is ready now.\n");
    } else {
        LOGD("Card subsystem failed to initialize.\n");
        return Status::FAILED;
    }

    openConnCb_ = std::make_shared<OpenConnectionCallback>();
    closeConnCb_ = std::make_shared<CloseConnectionCallback>();
    powerOnCb_ = std::make_shared<PowerOnCallback>();
    powerOffCb_ = std::make_shared<PowerOffCallback>();
    resetCb_ = std::make_shared<ResetCallback>();
    atrResetRespCb_ = std::make_shared<AtrResponseCallback>(CARD_RESET_MSG);
    atrInsertedRespCb_ = std::make_shared<AtrResponseCallback>(CARD_INSERTED_MSG);

    Status status = Status::FAILED;
    std::shared_ptr<ICard> card = CardControl::getInstance().getSimCard(DEFAULT_SLOT_ID, &status);
    if (status != Status::SUCCESS) {
        LOGE("Getting card from CardManager failed after subsystem ready!\n");
        return status;
    }

    CardState cardState = CardState::CARDSTATE_UNKNOWN;
    status = card->getState(cardState);
    if (status != Status::SUCCESS) {
        LOGE("Getting card state failed!\n");
        return status;
    }

    switch (cardState) {
        case CardState::CARDSTATE_ABSENT: {
            LOGD("Card is not present.\n");
            setCardPresent(false);
            setCardConnected(false);
        } break;
        case CardState::CARDSTATE_PRESENT: {
            LOGD("Card is present.\n");
            setCardPresent(true);
        } break;
        default:
            LOGD("Card state = %d.\n", static_cast<int>(cardState));
            break;
    }

    listener_ = std::make_shared<CardListener>();
    if (cardMgr_->registerListener(listener_) != Status::SUCCESS) {
        LOGE("Listener registration failed!\n");
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

void CardControl::sendApduToSim(uint8_t *buf, int bytes)
{
    /* APDU message format - ETSI TS 102 221 specification, section 10.1
     *
     * Case     Structure
     *  1       CLA INS P1 P2
     *  2       CLA INS P1 P2 Le
     *  3       CLA INS P1 P2 Lc Data
     *  4       CLA INS P1 P2 Lc Data Le
     *
     * The APDU message structure is 4 bytes at a minimum. The buffer must then hold at
     * least 6 bytes (buf[0] indicates the message type, while buf[1] stores the unique
     * APDU ID that must match between the request and response.
     */

    if (!cardConnected_) {
        LOGE("Cannot transmit APDU - no open connection to card!\n");
        return;
    }

    if (bytes < APDU_MSG_MIN_LEN) {
        LOGE("APDU transfer msg length is too short!\n");
        return;
    }

    uint8_t apduId = buf[1];

    uint8_t cla = buf[2];
    uint8_t instruction = buf[3];
    uint8_t p1 = buf[4];
    uint8_t p2 = buf[5];

    // If the message is less than 8 bytes, Lc is not included.
    uint8_t lc;
    if (bytes < APDU_MSG_CASE_3_LEN) {
        lc = 0;
    } else {
        lc = buf[6];
    }

    // If Lc is present and the message is less than 7 + Lc bytes, there is less
    // data than Lc indicated.
    if (lc > 0 && bytes < lc + 7) {
        LOGE("APDU transfer msg length is shorter than indicated!\n");
        return;
    }

    std::vector<uint8_t> data;

    for (unsigned int i = 0; i < lc; i++) {
        data.push_back(buf[i + 7]);
    }

    // If the message is Case 2 or 4 formats, Le is included.
    uint8_t le = 0;
    if (bytes == 7) {
        le = buf[6];
    } else if (bytes > lc + 7) {
        le = buf[lc + 7];
    }

    apduRespCbMap_[apduId] = std::make_shared<ApduResponseCallback>(apduId);

    if (sapCardMgr_->transmitApdu(cla, instruction, p1, p2, lc, data, le, apduRespCbMap_[apduId])
        != Status::SUCCESS) {
        LOGE("Failed to send transmit APDU request to the modem!\n");
    }
}

void CardControl::connectToCard()
{
    if (!cardConnected_) {
        if (openSapConn() != Status::SUCCESS) {
            return;
        }
    }

    if (sapCardMgr_->requestAtr(atrResetRespCb_) != Status::SUCCESS) {
        LOGE("Failed to send AtR request to the modem!\n");
    }
}

void CardControl::disconnectFromCard()
{
    if (!cardConnected_) {
        LOGE("Cannot close connection - card is already not connected!\n");
        return;
    }

    if (sapCardMgr_->closeConnection(closeConnCb_) != Status::SUCCESS) {
        LOGE("Failed to send close connection request to the modem!\n");
    }
}

void CardControl::powerUpCard()
{
    if (requestPowerOn() == Status::SUCCESS) {
        if (sapCardMgr_->requestAtr(atrResetRespCb_) != Status::SUCCESS) {
            LOGE("Failed to send AtR request to the modem!\n");
        }
    }
}

void CardControl::powerDownCard()
{
    if (!cardConnected_) {
        LOGE("Cannot power down card - no open connection to card!\n");
        return;
    }

    if (sapCardMgr_->requestSimPowerOff(powerOffCb_) != Status::SUCCESS) {
        LOGE("Failed to send power off request to the modem!\n");
    }
}

void CardControl::resetCard()
{
    if (!cardConnected_) {
        LOGE("Cannot reset card - no open connection to card!\n");
        return;
    }

    if (sapCardMgr_->requestSimReset(resetCb_) != Status::SUCCESS) {
        LOGE("Failed to send reset request to the modem!\n");
    }
}

void CardControl::setCardPresent(bool isPresent)
{
    cardPresent_ = isPresent;
}

void CardControl::setCardConnected(bool isConnected)
{
    cardConnected_ = isConnected;
}

void CardControl::connectAfterCardInsertion()
{
    if (!cardConnected_ && SimConnection::getInstance().isDaemonConnected()) {
        if (openSapConn() != Status::SUCCESS) {
            return;
        }
    }

    if (sapCardMgr_->requestAtr(atrInsertedRespCb_) != Status::SUCCESS) {
        LOGE("Failed to send AtR request to the modem!\n");
    }
}

void CardControl::requestAtrAfterReset()
{
    if (sapCardMgr_->requestAtr(atrResetRespCb_) != Status::SUCCESS) {
        LOGE("Failed to send AtR request to the modem!\n");
    }
}

void CardControl::eraseFromApduRespCbMap(uint8_t apduId)
{
    apduRespCbMap_.erase(apduId);
}

void CardControl::setOpenConnPromiseValue(bool isSuccess)
{
    openConnPromise_.set_value(isSuccess);
}

void CardControl::setPowerOnPromiseValue(bool isSuccess)
{
    powerOnPromise_.set_value(isSuccess);
}

std::shared_ptr<ICard> CardControl::getSimCard(int slotId, Status *status)
{
    return cardMgr_->getCard(slotId, status);
}

Status CardControl::openSapConn()
{
    if (!cardPresent_) {
        LOGE("Cannot open connection, card is not present!\n");
        return Status::FAILED;
    }

    if (sapCardMgr_->openConnection(SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA, openConnCb_)
        != Status::SUCCESS) {
        LOGE("Failed to send open connection request to the modem!\n");
        return Status::FAILED;
    }

    int timeoutSec = ASYNC_RESPONSE_TIMEOUT_SEC;
    openConnPromise_ = std::promise<bool>();
    auto openConnFuture = openConnPromise_.get_future();

    if (openConnFuture.wait_for(std::chrono::seconds(timeoutSec)) != std::future_status::ready) {
        LOGE("SIM connection did not open within %d seconds!\n", timeoutSec);
        return Status::FAILED;
    }

    if (openConnFuture.get() == true) {
        setCardConnected(true);
        return Status::SUCCESS;
    } else {
        LOGE("Open connection failed!\n");
        return Status::FAILED;
    }
}

Status CardControl::requestPowerOn()
{
    if (!cardConnected_) {
        if (openSapConn() != Status::SUCCESS) {
            LOGE("Cannot power on card, card is not connected!\n");
            return Status::FAILED;
        }
    }

    if (sapCardMgr_->requestSimPowerOn(powerOnCb_) != Status::SUCCESS) {
        LOGE("Failed to send SIM power on request to the modem!\n");
        return Status::FAILED;
    }

    int timeoutSec = ASYNC_RESPONSE_TIMEOUT_SEC;
    powerOnPromise_ = std::promise<bool>();
    auto powerOnFuture = powerOnPromise_.get_future();

    if (powerOnFuture.wait_for(std::chrono::seconds(timeoutSec)) != std::future_status::ready) {
        LOGE("SIM didn't power on within %d seconds!\n", timeoutSec);
        return Status::FAILED;
    }

    if (powerOnFuture.get() == true) {
        return Status::SUCCESS;
    } else {
        return Status::FAILED;
    }
}
