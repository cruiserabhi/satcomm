/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "CardStub.hpp"
#include "common/CommonUtils.hpp"
#include <thread>
namespace telux {

namespace tel {

CardStub::CardStub(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<CardService>();
    slotId_ = slotId;
    cardFileHandler_ = std::make_shared<CardFileHandlerStub>(static_cast<SlotId>(slotId_));
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

CardStub::~CardStub() {
    LOG(DEBUG, __FUNCTION__);
}

void CardStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
   cardApps_.clear();
   applications_.clear();
   if (cardFileHandler_) {
      cardFileHandler_->cleanup();
      cardFileHandler_ = nullptr;
   }
}

telux::common::Status CardStub::getState(CardState &cardState) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::GetCardStateRequest request;
    ::telStub::GetCardStateReply response;
    telux::common::Status error = telux::common::Status::SUCCESS;
    ClientContext context;
    ::telStub::CardState state;
    request.set_phone_id(slotId_);

    grpc::Status status = stub_->GetCardState(&context, request, &response);

    if (!status.ok()) {
       return telux::common::Status::FAILED;
    }
    state = response.card_state();
    cardState = static_cast<telux::tel::CardState>(state);
    return error;
}

std::vector<std::shared_ptr<ICardApp>> CardStub::getApplications(
    telux::common::Status *status) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<ICardApp>> applications;
    std::lock_guard<std::mutex> lock(cardMutex_);
    if(cardApps_.size() <= 0) {
        LOG(ERROR, "No card apps");
        if(status) {
            *status = telux::common::Status::NOTREADY;
        }
        return applications;
    }
    updateSimStatus();  //To get the latest card apps from json
    for(auto cardApp : cardApps_) {
        applications.emplace_back(cardApp);
    }
    if(status) {
        *status = telux::common::Status::SUCCESS;
    }
    return applications;
}

void CardStub::setlisteners(std::vector<std::weak_ptr<ICardListener>> listeners){
    for(auto cardApp : cardApps_) {
        cardApp->setlisteners(listeners);
    }
}

telux::common::Status CardStub::openLogicalChannel(
    std::string applicationId, std::shared_ptr<ICardChannelCallback> callback) {
    if (validateAppId(applicationId) == true) {
        LOG(DEBUG, __FUNCTION__,
            "Send request to open the channel for application: ", applicationId);

        ::telStub::OpenLogicalChannelRequest request;
        ::telStub::OpenLogicalChannelReply response;
        ClientContext context;

        request.set_phone_id(slotId_);
        request.set_app_id(applicationId);
        grpc::Status reqstatus = stub_->OpenLogicalChannel(&context, request, &response);

        if (!reqstatus.ok()) {
           return telux::common::Status::FAILED;
        }
        telux::tel::IccResult iccresult;
        iccresult.sw1  = static_cast<int>((response.result()).sw1());
        iccresult.sw2  = static_cast<int>((response.result()).sw2());
        iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
        int channel  = static_cast<int>(response.channel_id());
        telux::common::Status status = static_cast<telux::common::Status>(response.status());
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        std::vector<int> store;
        for (auto &r : (response.result()).data()) {
            int tmp =  static_cast<uint8_t>(r);
            LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
            store.emplace_back(tmp);
        }
        (iccresult.data).assign(store.begin(), store.end());

        LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1, "sw2 " ,iccresult.sw2,
            "payload " ,iccresult.payload );

        if((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
            auto f = std::async(std::launch::async,
             [this, delay, callback, channel, iccresult, error]() {
                   this->invokeCallback(callback, channel, iccresult, error, delay);
             }).share();
            taskQ_->add(f);
        }
        return status;
    } else {

       LOG(ERROR, __FUNCTION__, " Not a Valid application Id:", applicationId);
       return telux::common::Status::INVALIDPARAM;
    }
}

void CardStub::invokeCallback(std::shared_ptr<ICardChannelCallback> callback, int channel,
    IccResult result, telux::common::ErrorCode error, int delay) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    if(callback) {
        callback->onChannelResponse(channel, result, error);
    }
}

telux::common::Status CardStub::closeLogicalChannel(
    int channelId, std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    ::telStub::CloseLogicalChannelRequest request;
    ::telStub::CloseLogicalChannelReply response;
    ClientContext context;

    request.set_phone_id(slotId_);
    request.set_channel_id(channelId);
    grpc::Status reqstatus = stub_->CloseLogicalChannel(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());

    if((isCallbackNeeded) && (status == telux::common::Status::SUCCESS )) {
        auto f = std::async(std::launch::async,
        [this, callback, delay, error]() {
            this->invokeCallback(callback, delay, error);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

void CardStub::invokeCallback(std::shared_ptr<telux::common::ICommandResponseCallback> callback,
    int delay, telux::common::ErrorCode error ) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    if (callback) {
        callback->commandResponse(error);
    }
}

telux::common::Status CardStub::transmitApduLogicalChannel(int channel, uint8_t cla,
    uint8_t instruction, uint8_t p1, uint8_t p2, uint8_t p3, std::vector<uint8_t> data,
    std::shared_ptr<ICardCommandCallback> callback) {
    ::telStub::TransmitAPDURequest request;
    ::telStub::TransmitAPDUReply response;
    ClientContext context;

    request.set_phone_id(slotId_);
    telux::tel::IccResult iccresult;
    int size = data.size();
    for (int j = 0; j < size ; j++)
    {
        int d = static_cast<int>(data[j]);
        request.add_data(d);
    }
    grpc::Status reqstatus = stub_->TransmitAPDU(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());
    std::vector<int> store;
    for(auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1,
        "sw2 " ,iccresult.sw2,"payload " ,iccresult.payload, "status", static_cast<int>(status));

    if((isCallbackNeeded) && (status == telux::common::Status::SUCCESS )) {
        auto f = std::async(std::launch::async,
            [this, delay, iccresult, error , callback]() {
                this->invokeCallback(callback, delay, iccresult, error);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void CardStub::invokeCallback(std::shared_ptr<ICardCommandCallback> callback, int delay,
    telux::tel::IccResult iccresult, telux::common::ErrorCode error ) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    if (callback) {
        callback->onResponse(iccresult, error);
    }
}

telux::common::Status CardStub::transmitApduBasicChannel(uint8_t cla, uint8_t instruction,
    uint8_t p1, uint8_t p2, uint8_t p3, std::vector<uint8_t> data,
    std::shared_ptr<ICardCommandCallback> callback) {
    ::telStub::TransmitBasicAPDURequest request;
    ::telStub::TransmitBasicAPDUReply response;
    ClientContext context;

    request.set_phone_id(slotId_);
    telux::tel::IccResult iccresult;
    int size = data.size();
    for (int j = 0; j < size ; j++)
    {
        int d = static_cast<int>(data[j]);
        request.add_data(d);
    }
    grpc::Status reqstatus = stub_->TransmitBasicAPDU(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());
    std::vector<int> store;
    for (auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1,
        "sw2 " ,iccresult.sw2,"payload " ,iccresult.payload);

    if((isCallbackNeeded) && (status == telux::common::Status::SUCCESS )) {
        auto f = std::async(std::launch::async,
            [this, iccresult, error, callback, delay]() {
                this->invokeCallback(callback, delay, iccresult, error);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status CardStub::exchangeSimIO(uint16_t fileId, uint8_t command, uint8_t p1,
    uint8_t p2, uint8_t p3, std::string filePath, std::vector<uint8_t> data, std::string pin2,
    std::string aid, std::shared_ptr<ICardCommandCallback> callback) {

    ::telStub::exchangeSimIORequest request;
    ::telStub::exchangeSimIOReply response;
    ClientContext context;

    request.set_phone_id(slotId_);
    //add data input
    telux::tel::IccResult iccresult;
    int size = data.size();
    for (int j = 0; j < size ; j++)
    {
        int d = static_cast<int>(data[j]);
        request.add_data(d);
    }
    grpc::Status reqstatus = stub_->exchangeSimIO(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    iccresult.sw1  = static_cast<int>((response.result()).sw1());
    iccresult.sw2  = static_cast<int>((response.result()).sw2());
    iccresult.payload  = static_cast<std::string>((response.result()).pay_load());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());
    std::vector<int> store;
    for (auto &r : (response.result()).data()) {
        int tmp =  static_cast<uint8_t>(r);
        LOG(DEBUG, __FUNCTION__,"data response is  " ,tmp );
        store.emplace_back(tmp);
    }
    (iccresult.data).assign(store.begin(), store.end());

    LOG(DEBUG, __FUNCTION__,"sw1 " ,iccresult.sw1,
        "sw2 " ,iccresult.sw2,"payload " ,iccresult.payload);

    if((isCallbackNeeded) && (status == telux::common::Status::SUCCESS )) {
        auto f = std::async(std::launch::async,
            [this, delay, iccresult, error , callback]() {
                this->invokeCallback(callback, delay, iccresult, error);
            }).share();
        taskQ_->add(f);
    }
    return status;
}


telux::common::Status CardStub::requestEid(EidResponseCallback callback) {
    ::telStub::requestEidReply response;
    ::telStub::requestEidRequest request;
    request.set_phone_id(slotId_);
    ClientContext context;

    grpc::Status reqstatus = stub_->requestEid(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    std::string eid = static_cast<std::string>(response.eid());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, eid, error , callback, delay]() {
                this->invokeCallback(callback, eid, delay, error);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

bool CardStub::isNtnProfileActive() {
    LOG(DEBUG, __FUNCTION__, " getSlotId() = ", getSlotId());
    bool ntnSupported = false;
    ::telStub::IsNtnProfileActiveRequest request;
    ::telStub::IsNtnProfileActiveReply response;
    ClientContext context;
    request.set_phone_id(getSlotId());

    grpc::Status status = stub_->IsNtnProfileActive(&context, request, &response);

    if (!status.ok()) {
        return ntnSupported;
    }
    ntnSupported = response.is_ntn_profile_active();
    LOG(DEBUG, __FUNCTION__, " ntnSupported : ", ntnSupported);
    return ntnSupported;
}

void CardStub::invokeCallback(EidResponseCallback callback, std::string eid, int delay,
    telux::common::ErrorCode error ) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    if (callback) {
        callback(eid, error);
    }
}

std::shared_ptr<ICardFileHandler> CardStub::getFileHandler() {
    return cardFileHandler_;
}

int CardStub::getSlotId() {
    return slotId_;
}

bool CardStub::validateAppId(std::string applicationId) {
    int count = 0;
    int appIdSize = applicationId.size();
    for (auto i = 0; i < appIdSize; i++) {
        if(isxdigit(applicationId[i])) {
            count++;
        }
    }
    if ((count == appIdSize) && (appIdSize % 2 == 0)) {
        return true;
    }
    return false;
}

void CardStub::updateSimStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::updateSimStatusRequest request;
    ::telStub::updateSimStatusReply response;
    request.set_phone_id(slotId_);
    ClientContext context;

    grpc::Status status = stub_->updateSimStatus(&context, request, &response);

    ::telStub::AppType apptype;
    ::telStub::AppState appstate;
    std::vector<std::shared_ptr<ICardApp>> applications;

    std::vector<CardAppStatus> latestApplications;
    for (int i = 0; i < response.card_apps_size(); i++) {
        CardAppStatus appstatus;
        apptype = response.mutable_card_apps(i)->app_type();
        appstatus.appType = static_cast<telux::tel::AppType>(apptype);
        LOG(DEBUG, __FUNCTION__,"appType " , static_cast<int>(appstatus.appType));

        appstate = response.mutable_card_apps(i)->app_state();
        appstatus.appState = static_cast<telux::tel::AppState>(appstate);
        LOG(DEBUG, __FUNCTION__,"appState " , static_cast<int>(appstatus.appState));

        appstatus.aid = response.mutable_card_apps(i)->app_id();
         LOG(DEBUG, __FUNCTION__,"aid " , appstatus.aid);
         latestApplications.emplace_back(appstatus);
    }
    LOG(DEBUG,__FUNCTION__, "Number of original cardApps : ", cardApps_.size(),
        "Number of latestApplications: ", latestApplications.size());

    // Compare the list of CardApp obtained from server with cached values
    for(auto matchingCardApp = std::begin(cardApps_); matchingCardApp != std::end(cardApps_);) {
        auto iter = std::find_if(
        std::begin(latestApplications), std::end(latestApplications),
        [=](CardAppStatus cardAppStatus) { return (*matchingCardApp)->match(cardAppStatus); });
        if(iter != std::end(latestApplications)) {  // matching cardApp found, update the cardApp
                                                    // with latest cardApp info received from
                                                    // server
            LOG(DEBUG, "Updating existing card App details");
            std::shared_ptr<CardAppStub> cardApp = *matchingCardApp;
            if(cardApp) {
                LOG(DEBUG, "Card App pointer address ", cardApp);
                cardApp->updateCardApp(*iter);
                latestApplications.erase(iter);
                matchingCardApp++;
            } else {
                LOG(DEBUG, " cardApp is NULL");
            }
        } else {  // Cached Card App is dropped as recent received CardApps does not have same
                  // CardApp
            LOG(DEBUG, "dropped Card App found, removing it");
            std::shared_ptr<CardAppStub> cardApp = *matchingCardApp;
            if(cardApp) {
                LOG(DEBUG, "Card App pointer address ", cardApp);
                cardApps_.erase(matchingCardApp);  // dropped Card App, remove it
            } else {
                LOG(DEBUG, " cardApp is NULL");
            }
        }
    }
      // Add new card app to the list of cached card Apps
    for(auto &newCardAppStatus : latestApplications) {
        LOG(DEBUG, "Number of original cardApps : ", cardApps_.size());
        auto cardApp = std::make_shared<CardAppStub>(slotId_, newCardAppStatus);
        cardApps_.emplace_back(cardApp);
    }

    if (cardFileHandler_) {
        cardFileHandler_->updateCardApps(cardApps_);
    } else {
        LOG(WARNING, __FUNCTION__, " cardFileHandler is null");
    }
}

} // end of namespace tel

} // end of namespace telux
