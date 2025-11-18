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

#include "CardFileHandlerStub.hpp"
#include "CardAppStub.hpp"
#include "CardManagerStub.hpp"

using namespace telux::common;

namespace telux {

namespace tel {

CardAppStub::CardAppStub(int slotId, CardAppStatus cardAppStatus) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<CardService>();
    slotId_ = slotId;
    cardAppStatus_ = cardAppStatus;
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

AppType CardAppStub::getAppType() {
    return cardAppStatus_.appType;
}

AppState CardAppStub::getAppState() {
    return cardAppStatus_.appState;
}

std::string CardAppStub::getAppId()  {
    return cardAppStatus_.aid;
}

telux::common::Status CardAppStub::changeCardPassword(CardLockType lockType, std::string oldPwd,
    std::string newPwd, PinOperationResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::ChangePinLockRequest request;
    ::telStub::ChangePinLockReply response;
    bool IsCardInfoChanged = false;
    telux::common::Status status = telux::common::Status::FAILED;
    ClientContext context;
    if(oldPwd.empty() || newPwd.empty()) {
        return telux::common::Status::FAILED;
    }
    if(lockType == CardLockType::PIN1 || lockType == CardLockType::PIN2) {
        LOG(DEBUG, "Send request to change pin");

        request.set_phone_id(slotId_);
        ::telStub::CardLockType type = static_cast<::telStub::CardLockType>(lockType);
        request.set_lock_type(type);
        request.set_old_pin(oldPwd);
        request.set_new_pin(newPwd);
        std::string appId = getAppId();
        request.set_aid(appId);
        grpc::Status reqstatus = stub_->ChangePinLock(&context, request, &response);

        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        int retrycount  = static_cast<int>(response.retry_count());
        IsCardInfoChanged  = static_cast<int>(response.iscardinfochanged());
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int delay = static_cast<int>(response.delay());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());

        if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
                [this, callback, error , retrycount, delay]() {
                    this->invokeCallback(callback, error , retrycount, delay);
                }).share();
            taskQ_->add(f1);
            if(IsCardInfoChanged) {
                auto f2 = std::async(std::launch::async,
                    [this]() {
                        this->invokelisteners(slotId_);
                    }).share();
                taskQ_->add(f2);
            }
        }
    } else {
        LOG(DEBUG, " Unsupported card lock: ", static_cast<int>(lockType));
        return telux::common::Status::NOTSUPPORTED;
    }
    return status;
}

void CardAppStub::invokeCallback(PinOperationResponseCb callback,telux::common::ErrorCode error,
    int retryCount, int delay) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    auto f1 = std::async(std::launch::async,
        [this, error , retryCount, callback]() {
            callback(retryCount, error);
        }).share();
    taskQ_->add(f1);

}
void CardAppStub::setlisteners(std::vector<std::weak_ptr<ICardListener>> listeners){
    listeners_ = listeners;
}

void CardAppStub::invokelisteners(int slotId) {
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onCardInfoChanged(slotId);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

telux::common::Status CardAppStub::unlockCardByPuk(CardLockType lockType, std::string puk,
    std::string newPin, PinOperationResponseCb callback) {
    if(newPin.empty()) {
        return telux::common::Status::FAILED;
    }
    std::string aid = getAppId();
    telux::common::Status status = telux::common::Status::FAILED;

    if(lockType == CardLockType::PUK1 || lockType == CardLockType::PUK2) {
        LOG(DEBUG, "Send request to unlock pin");
        ::telStub::UnlockByPukRequest request;
        ::telStub::UnlockByPukReply response;
        ClientContext context;

        request.set_phone_id(slotId_);
        ::telStub::CardLockType type = static_cast<::telStub::CardLockType>(lockType);
        request.set_lock_type(type);
        request.set_puk(puk);
        request.set_new_pin(newPin);
        std::string appId = getAppId();
        request.set_aid(appId);
        grpc::Status reqstatus = stub_->UnlockByPuk(&context, request, &response);

        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        int retrycount  = static_cast<int>(response.retry_count());
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int delay = static_cast<int>(response.delay());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());

        if ((status == telux::common::Status::SUCCESS)&&(isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
            [this,callback, error , retrycount, delay]() {
                this->invokeCallback(callback, error , retrycount, delay);
            }).share();
            taskQ_->add(f1);

            bool IsCardInfoChanged  = static_cast<int>(response.iscardinfochanged());
            if(IsCardInfoChanged) {
                auto f2 = std::async(std::launch::async,
                    [this]() {
                        this->invokelisteners(slotId_);
                    }).share();
                taskQ_->add(f2);
            }
        }
    } else {
        LOG(DEBUG, " Unsupported card lock: ", static_cast<int>(lockType));
        return telux::common::Status::NOTSUPPORTED;
    }
    return status;
}

telux::common::Status CardAppStub::unlockCardByPin(CardLockType lockType, std::string pin,
    PinOperationResponseCb callback) {
    if(pin.empty()) {
        return telux::common::Status::FAILED;
    }
    std::string aid = getAppId();
    telux::common::Status status = telux::common::Status::FAILED;

    if(lockType == CardLockType::PIN1 || lockType == CardLockType::PIN2) {
        LOG(DEBUG, "Send request to unlock pin");
        ::telStub::UnlockByPinRequest request;
        ::telStub::UnlockByPinReply response;
        ClientContext context;

        request.set_phone_id(slotId_);
        ::telStub::CardLockType type = static_cast<::telStub::CardLockType>(lockType);
        request.set_lock_type(type);
        request.set_pin(pin);
        std::string appId = getAppId();
        request.set_aid(appId);
        grpc::Status reqstatus = stub_->UnlockByPin(&context, request, &response);

        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        int retrycount  = static_cast<int>(response.retry_count());
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int delay = static_cast<int>(response.delay());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());

        if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
            bool IsCardInfoChanged  = static_cast<int>(response.iscardinfochanged());
            auto f1 = std::async(std::launch::async,
                [this,callback, error , retrycount, delay]() {
                    this->invokeCallback(callback, error , retrycount, delay);
                }).share();
            taskQ_->add(f1);
            if(IsCardInfoChanged) {
                auto f2 = std::async(std::launch::async,
                    [this]() {
                        this->invokelisteners(slotId_);
                    }).share();
                taskQ_->add(f2);
            }
        }
    } else {
        LOG(DEBUG, " Unsupported card lock: ", static_cast<int>(lockType));
        return telux::common::Status::NOTSUPPORTED;
    }
    return status;
}

void CardAppStub::invokeCallback(QueryPin1LockResponseCb callback,
    telux::common::ErrorCode error, int delay, bool state) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    auto f1 = std::async(std::launch::async,
        [this, error, state, callback]() {
            callback(state, error);
        }).share();
    taskQ_->add(f1);
}

telux::common::Status CardAppStub::queryPin1LockState(QueryPin1LockResponseCb callback) {
    ::telStub::QueryPin1LockRequest request;
    ::telStub::QueryPin1LockReply response;
    ClientContext context;

    request.set_phone_id(slotId_);
    grpc::Status reqstatus = stub_->QueryPin1Lock(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    bool state  = static_cast<bool>(response.state());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, callback, error, delay, state]() {
                this->invokeCallback(callback, error , delay, state );
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

void CardAppStub::invokeCallback( bool isavailable, bool isenabled,
    QueryFdnLockResponseCb callback,telux::common::ErrorCode error,
    int delay) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    auto f = std::async(std::launch::async,
        [this, isavailable, isenabled, error , callback]() {
            callback(isavailable, isenabled, error);
        }).share();
    taskQ_->add(f);
}

telux::common::Status CardAppStub::queryFdnLockState(QueryFdnLockResponseCb callback) {
    ::telStub::QueryFdnLockRequest request;
    ::telStub::QueryFdnLockReply response;
    ClientContext context;

    request.set_phone_id(slotId_);
    grpc::Status reqstatus = stub_->QueryFdnLock(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    bool isenabled  = static_cast<bool>(response.state());
    bool isavailable  = static_cast<bool>(response.is_available());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, isavailable, isenabled, callback, error, delay]() {
                this->invokeCallback(isavailable, isenabled, callback, error, delay);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status CardAppStub::setCardLock(CardLockType lockType, std::string password,
    bool isEnabled, PinOperationResponseCb callback) {
    if(password.empty()) {
        return telux::common::Status::FAILED;
    }
    std::string aid = getAppId();
    if(lockType == CardLockType::PIN1) {
        if(isEnabled) {
            LOG(DEBUG, "Send request to set PIN lock enabled");
        } else {
            LOG(DEBUG, "Send request to set PIN lock disabled");
        }
    } else if(lockType == CardLockType::FDN) {
        if(isEnabled) {
            LOG(DEBUG, "Send request to set FDN lock enabled");
        } else {
            LOG(DEBUG, "Send request to set FDN lock disabled");
        }
    } else {
        LOG(DEBUG, " Unsupported card lock: ", static_cast<int>(lockType));
        return telux::common::Status::FAILED;
    }

    ::telStub::SetCardLockRequest request;
    ::telStub::SetCardLockReply response;
    ClientContext context;
    request.set_phone_id(slotId_);
    ::telStub::CardLockType type = static_cast<::telStub::CardLockType>(lockType);
    request.set_lock_type(type);
    request.set_pwd(password);
    request.set_enable(isEnabled);
    std::string appId = getAppId();
    request.set_aid(appId);

    grpc::Status reqstatus = stub_->SetCardLock(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    int retrycount  = static_cast<int>(response.retry_count());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        bool IsCardInfoChanged  = static_cast<int>(response.iscardinfochanged());
        auto f = std::async(std::launch::async,
            [this, error , retrycount, callback, delay]() {
                this->invokeCallback(callback, retrycount, error, delay);
            }).share();
        taskQ_->add(f);
        if(IsCardInfoChanged) {
            auto f2 = std::async(std::launch::async,
                [this]() {
                    this->invokelisteners(slotId_);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}

void CardAppStub::invokeCallback(PinOperationResponseCb callback, int retrycount,
    telux::common::ErrorCode error, int delay) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    auto f = std::async(std::launch::async,
        [this, error , retrycount, callback]() {
            callback(retrycount, error);
        }).share();
    taskQ_->add(f);
}

bool CardAppStub::match(CardAppStatus &cardAppStatus) {
    LOG(DEBUG, "Card App Status: appType = ", static_cast<int>(cardAppStatus.appType),
        ", appState = ", static_cast<int>(cardAppStatus.appState),
        ", aid = ", (cardAppStatus.aid));
    if(cardAppStatus.appType == cardAppStatus_.appType
        && cardAppStatus.appState == cardAppStatus_.appState
        && cardAppStatus.aid == cardAppStatus_.aid) {
        return true;
    }
    return false;
}

telux::common::Status CardAppStub::updateCardApp(CardAppStatus &cardAppStatus) {
    LOG(DEBUG, __FUNCTION__);
    // Do nothing if states haven't changed.
    bool isCardAppChanged = !(match(cardAppStatus));
    if(!isCardAppChanged) {
        LOG(DEBUG, "No changes in card app for app type: ", cardAppStatus_.appType);
        return telux::common::Status::SUCCESS;
    } else {
        LOG(DEBUG, "Previous Card State: ", cardAppStatus_.appState, " Current Card State: ",
            cardAppStatus.appState);
        cardAppStatus_.appType = cardAppStatus.appType;
        cardAppStatus_.appState = cardAppStatus.appState;
        cardAppStatus_.aid = cardAppStatus.aid;
    }
    return telux::common::Status::SUCCESS;
}

} // end of namespace tel

} // end of namespace telux