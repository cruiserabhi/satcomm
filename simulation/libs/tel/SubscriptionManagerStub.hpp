/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       SubscriptionManagerStub.hpp
 *
 * @brief      Implementation of ISubscriptionManager
 *
 */

#ifndef SUBSCRIPTION_MANAGER_STUB_HPP
#define SUBSCRIPTION_MANAGER_STUB_HPP

#include "SubscriptionStub.hpp"
#include <telux/tel/CardManager.hpp>
#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include <telux/tel/SubscriptionManager.hpp>
#include "common/ListenerManager.hpp"
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "CardAppStub.hpp"

using telStub::SubscriptionService;

namespace telux {
namespace tel {

class SubscriptionManagerStub : public ISubscriptionManager,
                                public IEventListener,
                                public ICardListener,
                                public ISubscriptionListener,
                                public std::enable_shared_from_this<SubscriptionManagerStub>  {
public:
    SubscriptionManagerStub();
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~SubscriptionManagerStub();
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<ISubscriptionListener> listener) override;
    telux::common::Status removeListener(std::weak_ptr<ISubscriptionListener> listener) override;
    std::shared_ptr<ISubscription> getSubscription(int slotId = DEFAULT_SLOT_ID,
        telux::common::Status *status = nullptr) override;
    std::vector<std::shared_ptr<ISubscription>>
        getAllSubscriptions(telux::common::Status *status = nullptr) override;
    void onEventUpdate(google::protobuf::Any event);
    void cleanup();
private:
    SlotId slotId_;
    std::mutex subscriptionManagerMutex_;
    telux::common::InitResponseCb initCb_;
    std::condition_variable subMgrInitCV_;
    int cbDelay_;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::telStub::SubscriptionService::Stub> stub_;
    std::unique_ptr<::telStub::CardService::Stub> cardstub_;
    std::map<int, std::shared_ptr<SubscriptionStub>> subscriptionMap_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_ = nullptr;
    std::shared_ptr<telux::common::ListenerManager<ISubscriptionListener>> listenerMgr_ = nullptr;
    bool ready_ = false;
    bool waitForInitialization();
    void setSubsystemReady(bool status);
    void initSync();
    void setServiceStatus(telux::common::ServiceStatus status);
    void notifyNumberOfSubscriptions(int count);
    void notifySubscriptionListener(std::shared_ptr<ISubscription> subscription);
    telux::common::Status createSubscriptionAndNotify(int slotId);
    telux::common::Status addNewOrUpdateSubscription(int slotId);
    void handleEvent(std::string token, std::string event);
    void handleSubscriptionInfoChanged(::telStub::SubscriptionEvent event);
    void handleCardInfoChanged(::telStub::cardInfoChange event);
    void onCardInfoChanged(int slotId);
    telux::common::Status getState(CardState &cardState, int phoneId);
    telux::common::Status getAppInfo(std::vector<CardAppStatus> &apps, int phoneId);
    telux::common::Status fetchSubscription(int slotId, std::string *carrierName,
        std::string *iccId, int* mcc, int* mnc, std::string *number, std::string *imsi,
        std::string *gid1, std::string *gid2 );
    void onEventUpdate(std::string event);
};

} // end of namespace tel

} // end of namespace telux

#endif // SUBSCRIPTION_MANAGER_STUB_HPP
