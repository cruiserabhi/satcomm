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
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file    CardControl.hpp
 * @brief   This file contains declarations for the CardControl class, as well
 *          as various callbacks. This class is used by the Sap Card Provider
 *          to interact with the SIM card using CardManager and SapCardManager
 *          APIs.
 */

#ifndef CARDCONTROL_HPP
#define CARDCONTROL_HPP

#include <unordered_map>

#include <telux/tel/PhoneFactory.hpp>

class CardListener : public telux::tel::ICardListener {
public:
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onCardInfoChanged(int slotId) override;
};

class ApduResponseCallback : public telux::tel::ISapCardCommandCallback {
public:
    ApduResponseCallback(uint8_t apduId);

    void onResponse(telux::tel::IccResult result, telux::common::ErrorCode error) override;

private:
    uint8_t apduId_;
};

class OpenConnectionCallback : public telux::common::ICommandResponseCallback {
public:
    void commandResponse(telux::common::ErrorCode errorCode) override;
};

class CloseConnectionCallback : public telux::common::ICommandResponseCallback {
public:
    void commandResponse(telux::common::ErrorCode errorCode) override;
};

class PowerOnCallback : public telux::common::ICommandResponseCallback {
public:
    void commandResponse(telux::common::ErrorCode errorCode) override;
};

class PowerOffCallback : public telux::common::ICommandResponseCallback {
public:
    void commandResponse(telux::common::ErrorCode errorCode) override;
};

class ResetCallback : public telux::common::ICommandResponseCallback {
public:
    void commandResponse(telux::common::ErrorCode errorCode) override;
};

class AtrResponseCallback : public telux::tel::IAtrResponseCallback {
public:
    AtrResponseCallback(uint8_t msgType);

    void atrResponse(std::vector<int> responseAtr, telux::common::ErrorCode error) override;

private:
    uint8_t msgType_;
};

class CardControl {
public:
    static CardControl &getInstance();

    telux::common::Status init();

    void sendApduToSim(uint8_t *buf, int bytes);

    void connectToCard();

    void disconnectFromCard();

    void powerUpCard();

    void powerDownCard();

    void resetCard();

    void setCardPresent(bool isPresent);

    void setCardConnected(bool isConnected);

    void connectAfterCardInsertion();

    void requestAtrAfterReset();

    void eraseFromApduRespCbMap(uint8_t apduId);

    void setOpenConnPromiseValue(bool isSuccess);

    void setPowerOnPromiseValue(bool isSuccess);

    std::shared_ptr<telux::tel::ICard> getSimCard(int slotId, telux::common::Status *status);

private:
    telux::common::Status openSapConn();

    telux::common::Status requestPowerOn();

    std::shared_ptr<telux::tel::ISapCardManager> sapCardMgr_;
    std::shared_ptr<telux::tel::ICardManager> cardMgr_;
    std::shared_ptr<telux::tel::ICardListener> listener_;
    bool cardPresent_ = false;
    bool cardConnected_ = false;
    std::unordered_map<uint8_t, std::shared_ptr<ApduResponseCallback>> apduRespCbMap_;
    std::promise<bool> openConnPromise_;
    std::promise<bool> powerOnPromise_;
    std::shared_ptr<OpenConnectionCallback> openConnCb_;
    std::shared_ptr<CloseConnectionCallback> closeConnCb_;
    std::shared_ptr<PowerOnCallback> powerOnCb_;
    std::shared_ptr<PowerOffCallback> powerOffCb_;
    std::shared_ptr<ResetCallback> resetCb_;
    std::shared_ptr<AtrResponseCallback> atrResetRespCb_;
    std::shared_ptr<AtrResponseCallback> atrInsertedRespCb_;
};

#endif // CARDCONTROL_HPP
