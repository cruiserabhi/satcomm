/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SubscriptionManagerStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include "CardManagerStub.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using telStub::CardService;

using namespace telux::common;
#define FIRST_SIM_SLOT_ID 1

namespace telux {

namespace tel {

SubscriptionManagerStub::SubscriptionManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

void SubscriptionManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
        subSystemStatus_ = status;
    }
    if(initCb_) {
        auto f1 = std::async(std::launch::async,
        [this, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay_));
                initCb_(status);
        }).share();
        taskQ_->add(f1);
    } else {
        LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

telux::common::Status SubscriptionManagerStub::init(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ISubscriptionListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<SubscriptionService>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate subscription service");
        return telux::common::Status::FAILED;
    }
    cardstub_ = CommonUtils::getGrpcStub<CardService>();
    if(!cardstub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate card service");
        return telux::common::Status::FAILED;
    }
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if(!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    initCb_ = callback;
    auto f = std::async(std::launch::async,
        [this]() {
            this->initSync();
        }).share();
    auto status = taskQ_->add(f);
    return status;
}

SubscriptionManagerStub::~SubscriptionManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void SubscriptionManagerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
   std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);

   for(auto &it : subscriptionMap_) {
      auto subscription = it.second;
      subscription->cleanup();
   }

   if(!subscriptionMap_.empty()) {
      subscriptionMap_.clear();
   }
}

void SubscriptionManagerStub::initSync() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus servicestatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (reqStatus.ok()) {
        int numSlots = 1;
        cbDelay_ = static_cast<int>(response.delay());
        servicestatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        if(servicestatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            ClientContext context;
            cardstub_->InitService(&context, request, &response);
            telux::common::ServiceStatus cardMgrStatus =
                static_cast<telux::common::ServiceStatus>(response.service_status());
            if (cardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LOG(INFO, __FUNCTION__, " Card Manager subsystem is ready");
                if(telux::common::DeviceConfig::isMultiSimSupported()) {
                    numSlots = 2;
                }
                LOG(DEBUG, __FUNCTION__, " slot count from the card manager: ", numSlots);
                for(int id = FIRST_SIM_SLOT_ID; id < (FIRST_SIM_SLOT_ID + numSlots); id++) {
                    //check for card state and create the subscription object only
                    //if the card is available
                    telux::common::Status status = createSubscriptionAndNotify(id);
                    if(status != telux::common::Status::SUCCESS) {
                        LOG(ERROR, __FUNCTION__, " unable to update subscription",
                            " map on slot ", id);
                        servicestatus = telux::common::ServiceStatus::SERVICE_FAILED;
                        break;
                    }
                }
            } else {
                LOG(ERROR, __FUNCTION__,
                    " Card Manager subsystem is not ready,",
                    "failed to initialize Subscription Manager");
                servicestatus = telux::common::ServiceStatus::SERVICE_FAILED;
            }
        }
    }
    LOG(DEBUG, __FUNCTION__, " cbDelay:: ", cbDelay_, " cbStatus:: ",
        static_cast<int>(servicestatus));
    bool isSubsystemReady = (servicestatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
        true : false;
    setSubsystemReady(isSubsystemReady);
    setServiceStatus(servicestatus);
}

telux::common::Status SubscriptionManagerStub::getState(CardState &cardState, int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::GetCardStateRequest request;
    ::telStub::GetCardStateReply response;
    ClientContext context;
    ::telStub::CardState state;
    request.set_phone_id(phoneId);

    grpc::Status status = cardstub_->GetCardState(&context, request, &response);

    if (!status.ok()) {
       return telux::common::Status::FAILED;
    }
    state = response.card_state();
    cardState = static_cast<telux::tel::CardState>(state);
    LOG(DEBUG, __FUNCTION__, " Card state is ", static_cast<int>(cardState));
    return telux::common::Status::SUCCESS;
}

telux::common::Status SubscriptionManagerStub::getAppInfo(std::vector<CardAppStatus> &apps,
    int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::updateSimStatusRequest request;
    ::telStub::updateSimStatusReply response;
    ::telStub::AppType apptype;
    ::telStub::AppState appstate;
    request.set_phone_id(phoneId);
    ClientContext context;

    grpc::Status reqstatus = cardstub_->updateSimStatus(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    if (reqstatus.ok()) {
        for (int i = 0; i < response.card_apps_size(); i++) {
            CardAppStatus appstatus;
            apptype = response.mutable_card_apps(i)->app_type();
            appstatus.appType = static_cast<telux::tel::AppType>(apptype);
            LOG(DEBUG, __FUNCTION__," appType " , static_cast<int>(appstatus.appType));

            appstate = response.mutable_card_apps(i)->app_state();
            appstatus.appState = static_cast<telux::tel::AppState>(appstate);
            LOG(DEBUG, __FUNCTION__," appState " , static_cast<int>(appstatus.appState));
            apps.emplace_back(appstatus);
        }
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status SubscriptionManagerStub::createSubscriptionAndNotify(int slotId) {

    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    telux::common::Status status = telux::common::Status::FAILED;
    int mapSize = -1;
    telux::tel::CardState cardState;
    status = getState(cardState, slotId);
    if(status == telux::common::Status::SUCCESS) {
        switch(cardState) {
            case CardState::CARDSTATE_ABSENT:
            case CardState::CARDSTATE_ERROR: {
                LOG(DEBUG, __FUNCTION__, " card is absent or error ");
                {
                std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
                auto it = subscriptionMap_.find(slotId);
                if(it != subscriptionMap_.end()) {
                    subscriptionMap_.erase(it);
                    LOG(DEBUG, __FUNCTION__, " removed slot id ", slotId,
                        " from map and mapSize is ", mapSize);
                }
                mapSize = subscriptionMap_.size();
                }
                notifyNumberOfSubscriptions(mapSize);
                notifySubscriptionListener(nullptr);
            } break;
            case CardState::CARDSTATE_PRESENT: {
                LOG(DEBUG, __FUNCTION__, " card state is present ");
                std::vector<CardAppStatus> apps;
                status = getAppInfo(apps, slotId);
                if(status == telux::common::Status::SUCCESS) {
                    // wait for app state to be READY
                    bool isAppReady = false;
                    for(auto &it : apps) {
                        if(it.appType != AppType::APPTYPE_UNKNOWN
                            && it.appType != AppType::APPTYPE_CSIM) {
                            if(it.appState == AppState::APPSTATE_READY) {
                                LOG(DEBUG, __FUNCTION__, " App State is ready");
                                isAppReady = true;
                            } else {
                                isAppReady = false;
                                LOG(DEBUG, __FUNCTION__, " Apps were not ready, appState: ",
                                    static_cast<int>(it.appState));
                                break;
                            }
                        }
                    }
                    if(isAppReady) {
                        addNewOrUpdateSubscription(slotId);
                        {
                            std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
                            mapSize = subscriptionMap_.size();
                        }
                        notifyNumberOfSubscriptions(mapSize);
                    }
                }
            } break;
            case CardState::CARDSTATE_UNKNOWN:
            default: {
                LOG(DEBUG, __FUNCTION__, " card state is unknown or invalid: ",
                    static_cast<int>(cardState));
            }
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " unable to get card state ");
    }
    return status;
}

void SubscriptionManagerStub::onCardInfoChanged(int slotId) {
   LOG(DEBUG, __FUNCTION__, " SlotId: ", slotId);
   createSubscriptionAndNotify(slotId);
}

void SubscriptionManagerStub::notifyNumberOfSubscriptions(int count) {
   LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ISubscriptionListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onNumberOfSubscriptionsChanged(count);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void SubscriptionManagerStub::notifySubscriptionListener
    (std::shared_ptr<ISubscription> subscription) {
    std::vector<std::weak_ptr<ISubscriptionListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onSubscriptionInfoChanged(subscription);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

telux::common::Status SubscriptionManagerStub::addNewOrUpdateSubscription(int slotId) {
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    std::string carrierName;
    std::string iccId;
    int mcc;
    int mnc;
    std::string number;
    std::string imsi;
    std::string gid1;
    std::string gid2;

    std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
    telux::common::Status status =
        fetchSubscription(slotId, &carrierName, &iccId, &mcc, &mnc, &number, &imsi, &gid1, &gid2);
    if(status == telux::common::Status::SUCCESS) {
        if (subscriptionMap_.find(slotId) != subscriptionMap_.end()) {
            std::shared_ptr<SubscriptionStub> iSub = subscriptionMap_[slotId];
            iSub->updateSubscription(slotId, carrierName, iccId, mcc, mnc, number, imsi, gid1, gid2);
            notifySubscriptionListener(subscriptionMap_[slotId]);
        } else {
            std::shared_ptr<SubscriptionStub> iSub = subscriptionMap_[slotId];
            iSub = std::make_shared<SubscriptionStub>(
            slotId, carrierName, iccId, mcc, mnc, number, imsi, gid1, gid2);
            subscriptionMap_[slotId] = iSub;
        }
    }
    return status;
}

telux::common::Status SubscriptionManagerStub::fetchSubscription(int slotId,
    std::string *carrierName, std::string *iccId, int* mcc, int* mnc, std::string *number,
    std::string *imsi, std::string *gid1, std::string *gid2 ) {
    LOG(DEBUG, __FUNCTION__, slotId);
    ::telStub::GetSubscriptionRequest request;
    ::telStub::Subscription response;
    ClientContext context;

    request.set_phone_id(slotId);

    grpc::Status status = stub_->GetSubscription(&context, request, &response);

    if (!status.ok()) {
        return telux::common::Status::FAILED;
    }
    *carrierName  = static_cast<std::string>(response.carrier_name());
    *iccId  = static_cast<std::string>(response.icc_id());
    *mcc  = static_cast<int>(response.mcc());
    *mnc  = static_cast<int>(response.mnc());
    *number  = static_cast<std::string>(response.phone_number());
    *imsi  = static_cast<std::string>(response.imsi());
    *gid1  = static_cast<std::string>(response.gid_1());
    *gid2  = static_cast<std::string>(response.gid_2());

    LOG(DEBUG, __FUNCTION__, " Carrier name is ",carrierName
    ," Phone number is ",number, " iccid is ", iccId , " mcc is ",
    mcc , " mnc is ",mnc , " imsi is ",imsi, " gid1 is ",gid1 , " gid2 is ",gid2);

    return telux::common::Status::SUCCESS;
}

void SubscriptionManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(subscriptionManagerMutex_);
    ready_ = status;
    subMgrInitCV_.notify_all();
}

bool SubscriptionManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

bool SubscriptionManagerStub::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> lock(subscriptionManagerMutex_);
    while (!isSubsystemReady()) {
        LOG(DEBUG, __FUNCTION__, " Waiting for Subscription Manager to get ready ");
        subMgrInitCV_.wait(lock);
    }
    return isSubsystemReady();
}

std::future<bool> SubscriptionManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return waitForInitialization(); });
    return future;
}

telux::common::ServiceStatus SubscriptionManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status SubscriptionManagerStub::registerListener(
    std::weak_ptr<ISubscriptionListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_SUBSCRIPTION_FILTER, TEL_CARD_FILTER};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(shared_from_this(), filters);
    }
    return status;
}

telux::common::Status SubscriptionManagerStub::removeListener(
    std::weak_ptr<ISubscriptionListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ISubscriptionListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_SUBSCRIPTION_FILTER, TEL_CARD_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

std::shared_ptr<ISubscription> SubscriptionManagerStub::getSubscription(int slotId,
    telux::common::Status *status) {
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Subscription Manager not ready ");
        if(status) {
            *status = telux::common::Status::NOTREADY;
        }
        return nullptr;
    }
    telux::common::Status subStatus = telux::common::Status::FAILED;
    std::shared_ptr<ISubscription> subscription = nullptr;
    {
        std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
        auto it = subscriptionMap_.find(slotId);
        if(it != subscriptionMap_.end()) {
            subscription = it->second;
            subStatus = telux::common::Status::SUCCESS;
        }
    }
    if(status) {
        *status = subStatus;
    }
    return subscription;
}

std::vector<std::shared_ptr<ISubscription>>
    SubscriptionManagerStub::getAllSubscriptions(telux::common::Status *status) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<ISubscription>> subs;
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Subscription Manager not ready ");
        if(status) {
            *status = telux::common::Status::NOTREADY;
        }
        return subs;
    }
    {
        std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
        for(auto &it : subscriptionMap_) {
            subs.emplace_back(it.second);
        }
    }
    if(status) {
        *status = telux::common::Status::SUCCESS;
    }
    return subs;
}

void SubscriptionManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::cardInfoChange>()) {
        ::telStub::cardInfoChange cardEvent;
        event.UnpackTo(&cardEvent);
        handleCardInfoChanged(cardEvent);
    } else if (event.Is<::telStub::SubscriptionEvent>()) {
        ::telStub::SubscriptionEvent subscriptionEvent;
        event.UnpackTo(&subscriptionEvent);
        handleSubscriptionInfoChanged(subscriptionEvent);
    }
}

void SubscriptionManagerStub::handleCardInfoChanged(::telStub::cardInfoChange event) {
    int slotId = event.phone_id();
    onCardInfoChanged(slotId);
}

void SubscriptionManagerStub::handleSubscriptionInfoChanged(::telStub::SubscriptionEvent event) {
    int slotId = event.phone_id();
    LOG(DEBUG, __FUNCTION__, " The fetched slot id is: ", slotId);
    createSubscriptionAndNotify(slotId);
}

} // end of namespace tel

} // end of namespace telux
