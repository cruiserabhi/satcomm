/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * @file       CardAppStub.hpp
 *
 * @brief      Implementation of ICardApp
 *
 */

#ifndef CARDAPP_STUB_HPP
#define CARDAPP_STUB_HPP

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/CardApp.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/CardManager.hpp>
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::CardService;


namespace telux {
namespace tel {

struct CardAppStatus {
   AppType appType;
   AppState appState;
   std::string aid;
};

class CardAppStub : public ICardApp {
public:
    AppType getAppType() override;

    AppState getAppState() override;

    std::string getAppId()  override;

    telux::common::Status changeCardPassword(CardLockType lockType, std::string oldPwd,
    std::string newPwd, PinOperationResponseCb callback)
    override;

    telux::common::Status unlockCardByPuk(CardLockType lockType, std::string puk,
    std::string newPin,
    PinOperationResponseCb callback)
    override;

    telux::common::Status unlockCardByPin(CardLockType lockType, std::string pin,
    PinOperationResponseCb callback)
    override;
    telux::common::Status queryPin1LockState(QueryPin1LockResponseCb callback) override;

    telux::common::Status queryFdnLockState(QueryFdnLockResponseCb callback) override;

    telux::common::Status setCardLock(CardLockType lockType, std::string password,
    bool isEnabled, PinOperationResponseCb callback) override;
    CardAppStub(int slotId, CardAppStatus cardAppStatus);
    void setlisteners(std::vector<std::weak_ptr<ICardListener>> listeners);
    std::vector<std::weak_ptr<ICardListener>> listeners_;
    bool match(CardAppStatus &cardAppStatus);
    telux::common::Status updateCardApp(CardAppStatus &cardAppStatus);
private:
    int slotId_;
    CardAppStatus cardAppStatus_;
    std::unique_ptr<::telStub::CardService::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    void invokelisteners(int slotId);
    void invokeCallback(PinOperationResponseCb callback,telux::common::ErrorCode error,
        int retryCount, int delay);
    void invokeCallback(QueryPin1LockResponseCb callback, telux::common::ErrorCode error,
        int delay, bool state);
    void invokeCallback( bool isavailable, bool isenabled, QueryFdnLockResponseCb callback,
        telux::common::ErrorCode error, int delay);
    void invokeCallback(PinOperationResponseCb callback, int retrycount,
        telux::common::ErrorCode error, int delay);
};

} // end of namespace tel

} // end of namespace telux

#endif // CARDAPP_STUB_HPP