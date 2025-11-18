/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "PhoneFactoryImplStub.hpp"
#include <telux/common/DeviceConfig.hpp>

namespace telux {
namespace tel {

PhoneFactoryImplStub::PhoneFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
    cardMgrInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    subscriptionMgrInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    multiSimMgrInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    imssInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

PhoneFactoryImplStub::~PhoneFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
    // remove cardManager
    if (cardManager_) {
        (std::static_pointer_cast<CardManagerStub>(cardManager_))->cleanup();
    }
    // remove SubscriptionManagerStub
    if (subscriptionManager_) {
        (std::static_pointer_cast<SubscriptionManagerStub>(subscriptionManager_))->cleanup();
    }
    // remove smsManager
    for (const auto &sms : smsManagerMap_) {
        if (sms.second != nullptr) {
            (std::static_pointer_cast<SmsManagerStub>(sms.second))->cleanup();
        }
    }
    // remove multiSimManagerStub
    if (multiSimManager_) {
        (std::static_pointer_cast<MultiSimManagerStub>(multiSimManager_))->cleanup();
    }
    // remove imsServSysManager
    for (const auto &ims : imsServSysManagerMap_) {
        if (ims.second != nullptr) {
            (std::static_pointer_cast<ImsServingSystemManagerStub>(ims.second))->cleanup();
        }
    }
    // remove imsSettingsManager
    if (imsSettingsManager_) {
        (std::static_pointer_cast<ImsSettingsManagerStub>(imsSettingsManager_))->cleanup();
    }

    // remove servingSystemManager
    for (const auto &servingSysMgr : servingSystemManagerMap_) {
        if (servingSysMgr.second != nullptr) {
            (std::static_pointer_cast<ServingSystemManagerStub>(servingSysMgr.second))->cleanup();
        }
    }
    // remove networkSelectionManager
    for (const auto &networkSelMgr : networkSelectionManagerMap_) {
        if (networkSelMgr.second != nullptr) {
            (std::static_pointer_cast<NetworkSelectionManagerStub>
            (networkSelMgr.second))->cleanup();
        }
    }
    // cleanup suppServicesManager
    for (const auto &suppSvcMgr : suppSvcManagerMap_) {
        if (suppSvcMgr.second != nullptr) {
            (std::static_pointer_cast<SuppServicesManagerStub>(suppSvcMgr.second))->cleanup();
        }
    }
    networkSelectionManagerMap_.clear();
    networkSelMgrCallbacks_.clear();
    networkSelMgrInitStatus_.clear();
    servingSystemManagerMap_.clear();
    servingSysMgrCallbacks_.clear();
    servingSysMgrInitStatus_.clear();
    imsServSysManagerMap_.clear();
    imsServSysCallbacks_.clear();
    imsServingSystemMgrInitStatus_.clear();
    smsManagerMap_.clear();
    smsMgrInitStatus_.clear();
    smsMgrCallbacks_.clear();
    subscriptionMgrCallbacks_.clear();
    cardMgrCallbacks_.clear();
    multiSimMgrCallbacks_.clear();
    imssCallbacks_.clear();
    suppSvcManagerMap_.clear();
    suppSvcCallbacks_.clear();
    suppSvcInitStatus_.clear();
}

PhoneFactory::PhoneFactory() {
    LOG(DEBUG, __FUNCTION__);
}

PhoneFactory::~PhoneFactory() {
    LOG(DEBUG, __FUNCTION__);
}

PhoneFactory &PhoneFactoryImplStub::getInstance() {
    static PhoneFactoryImplStub instance;
    return instance;
}

PhoneFactory &PhoneFactory::getInstance() {
    return PhoneFactoryImplStub::getInstance();
}

std::shared_ptr<ISmsManager> PhoneFactoryImplStub::getSmsManager(int phoneId,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId: ", phoneId);
    if((phoneId < DEFAULT_SLOT_ID) || (phoneId > MAX_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " Invalid phoneId: ", phoneId);
        return nullptr;
    }
    if(!telux::common::DeviceConfig::isMultiSimSupported() && (phoneId != DEFAULT_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " MultiSim not supported, for phoneId: ", phoneId);
        return nullptr;
    }

    std::lock_guard<std::recursive_mutex> guard(mutex_);
    // Look for sms manager in the smsManagerMap_
    auto sms = smsManagerMap_.find(phoneId);
    if (sms == smsManagerMap_.end()) {
       LOG(DEBUG, __FUNCTION__, " Creating SMS Manager for phoneId ", phoneId);
       std::shared_ptr<SmsManagerStub> smsMgr = nullptr;
       try {
           smsMgr = std::make_shared<SmsManagerStub>(phoneId);
       } catch (std::bad_alloc & e) {
           LOG(ERROR, __FUNCTION__ , e.what());
           return nullptr;
       }
       auto initCb = [this, phoneId](telux::common::ServiceStatus status) {
           LOG(DEBUG, __FUNCTION__, " SMS Manager initialization callback");
           this->onSmsMgrInitResponse(phoneId, status);
       };
       auto status = smsMgr->init(initCb);
       if (status != telux::common::Status::SUCCESS) {
          LOG(ERROR, __FUNCTION__, " Failed to initialize the SMS manager");
          return nullptr;
       }
       smsManagerMap_.emplace(phoneId, smsMgr);
       smsMgrInitStatus_.emplace(phoneId, telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
       if (callback) {
           smsMgrCallbacks_[phoneId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (smsMgrInitStatus_[phoneId] == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " SMS Manager is not yet initialized");
       if (callback) {
           smsMgrCallbacks_[phoneId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " SMS Manager is initialized, invoking app callback");
       std::thread appCallback(callback, smsMgrInitStatus_[phoneId]);
       appCallback.detach();
    } else {
       LOG(DEBUG, __FUNCTION__, " SMS Manager is initialized, app callback is NULL");
    }
    return smsManagerMap_[phoneId];
}

void PhoneFactoryImplStub::onSmsMgrInitResponse(int phoneId, telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> smsMgrCallbacks;
    LOG(INFO, __FUNCTION__, " SMS Manager initalization status: ", static_cast<int>(status),
        " on phone: ", phoneId);
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       smsMgrInitStatus_[phoneId] = status;
       bool reportServiceStatus = false;
       switch(status) {
       case telux::common::ServiceStatus::SERVICE_FAILED:
          smsManagerMap_.erase(phoneId);
          smsMgrInitStatus_.erase(phoneId);
          reportServiceStatus = true;
          break;
       case telux::common::ServiceStatus::SERVICE_AVAILABLE:
          reportServiceStatus = true;
          break;
       default:
          break;
       }
       if(!reportServiceStatus) {
           return;
       }
       auto cbIter = smsMgrCallbacks_.find(phoneId);
       if(cbIter != smsMgrCallbacks_.end()) {
          smsMgrCallbacks = smsMgrCallbacks_[phoneId];
          smsMgrCallbacks_.erase(phoneId);
       }
    }
    for (auto &callback : smsMgrCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(ERROR, __FUNCTION__, " Callback is NULL");
        }
    }
}

std::shared_ptr<IPhoneManager> PhoneFactoryImplStub::getPhoneManager(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (phoneManager_ == nullptr) {
       std::shared_ptr<PhoneManagerStub> phoneManager = nullptr;
       try {
           phoneManager = std::make_shared<PhoneManagerStub>();
       } catch (std::bad_alloc & e) {
           LOG(ERROR, __FUNCTION__ , e.what());
           return nullptr;
       }
       auto initCb = [this](telux::common::ServiceStatus status) {
           LOG(DEBUG, __FUNCTION__, " Phone Manager initialization callback");
           this->onPhoneManagerResponse(status);
       };
       auto status = phoneManager->init(initCb);
       if (status != telux::common::Status::SUCCESS) {
          LOG(ERROR, __FUNCTION__, " Failed to initialize the Phone manager");
          phoneManager_ = nullptr;
          return nullptr;
       }
       if (callback) {
          phoneMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
       phoneManager_ = phoneManager;
    } else if (phoneMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " Phone manager is not yet initialized");
       if (callback) {
          phoneMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " Phone manager is initialized, invoking app callback");
       std::thread appCallback(callback, phoneMgrInitStatus_);
       appCallback.detach();
    } else {
       LOG(ERROR, __FUNCTION__, " Phone manager is initialized, app Callback is NULL");
    }
    return phoneManager_;
}


void PhoneFactoryImplStub::onPhoneManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> phmgrCallbacks;
    LOG(INFO, __FUNCTION__, " Phone Manager initialization status: " ,
      static_cast<int>(status));
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       phoneMgrInitStatus_ = status;
       bool reportServiceStatus = false;
       switch(status) {
          case telux::common::ServiceStatus::SERVICE_FAILED:
             phoneManager_ = NULL;
             reportServiceStatus = true;
             break;
          case telux::common::ServiceStatus::SERVICE_AVAILABLE:
             reportServiceStatus = true;
             break;
          default:
             break;
       }
       if (!reportServiceStatus) {
          return;
       }
       phmgrCallbacks = phoneMgrCallbacks_;
       phoneMgrCallbacks_.clear();
    }
    for (auto &callback : phmgrCallbacks) {
       if (callback) {
          callback(status);
       } else {
          LOG(INFO, __FUNCTION__, " Callback is NULL");
       }
    }
}

void PhoneFactoryImplStub::onCallMgrInitResponse(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " status: ", static_cast<int>(status));
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        callMgrInitStatus_ = status;
        if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
            //it is probably the manager's initSync task context, discard object in a separate
            //thread, to avoid join itself in the manager's destruction issue.
            std::thread cleanup([this]() {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                callManager_ = nullptr;
            });
            cleanup.detach();
        } else if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            return;
        }
        cbs = callMgrInitCallbacks_;
        callMgrInitCallbacks_.clear();
    }
    for (auto &cb : cbs) {
        if(cb) {
            cb(status);
        }
    }
}

std::shared_ptr<ICallManager> PhoneFactoryImplStub::getCallManager(
    telux::common::InitResponseCb callback) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    std::shared_ptr<CallManagerStub> callManager = nullptr;
    if (callManager_ == nullptr) {
        try {
            callManager = std::make_shared<CallManagerStub>();
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        auto initCb = [this](telux::common::ServiceStatus status) {
           LOG(DEBUG, __FUNCTION__, " Call Manager initialization callback");
           this->onCallMgrInitResponse(status);
        };
        auto status = callManager->init(initCb);
        if(status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to initialize CallManager");
            callManager_ = nullptr;
            return nullptr;
        }
        callMgrInitCallbacks_.push_back(callback);
        callManager_ = callManager;
    } else if (callMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " Call Manager is not yet initialized");
       if (callback) {
           callMgrInitCallbacks_.push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " Call Manager is initialized, invoking app callback");
       std::thread appCallback(callback, callMgrInitStatus_);
       appCallback.detach();
    }
    return callManager_;
}

std::shared_ptr<ICardManager> PhoneFactoryImplStub::getCardManager(
    telux::common::InitResponseCb callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (cardManager_ == nullptr) {
        std::shared_ptr<CardManagerStub> cardManager = nullptr;
        try {
            cardManager = std::make_shared<CardManagerStub>();
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        auto initCb = [this](telux::common::ServiceStatus status) {
            LOG(DEBUG, __FUNCTION__, " CardManager initialization callback");
            this->onCardManagerResponse(status);
        };
        auto status = cardManager->init(initCb);
        if(status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to initialize cardManager");
            cardManager_ = nullptr;
            return nullptr;
        }
        if (callback) {
            cardMgrCallbacks_.push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
        cardManager_ = cardManager;
    } else if (cardMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(DEBUG, __FUNCTION__, " Card manager is not yet initialized");
        if (callback) {
           cardMgrCallbacks_.push_back(callback);
        } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
    } else if (callback) {
        LOG(DEBUG, __FUNCTION__, " card manager is initialized, invoking app callback");
        std::thread appCallback(callback, cardMgrInitStatus_);
        appCallback.detach();
    } else {
        LOG(ERROR, __FUNCTION__, " Card manager is initialized, app Callback is NULL");
    }
    return cardManager_;
}

void PhoneFactoryImplStub::onCardManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cardMgrCallbacks;
    LOG(INFO, __FUNCTION__, " Card Manager initialization status: " ,
        static_cast<int>(status));
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        cardMgrInitStatus_ = status;
        bool reportServiceStatus = false;
        switch(status) {
           case telux::common::ServiceStatus::SERVICE_FAILED:
              cardManager_ = NULL;
              reportServiceStatus = true;
              break;
           case telux::common::ServiceStatus::SERVICE_AVAILABLE:
              reportServiceStatus = true;
              break;
           default:
              break;
       }
       if (!reportServiceStatus) {
           return;
       }
       cardMgrCallbacks = cardMgrCallbacks_;
       cardMgrCallbacks_.clear();
    }
    for (auto &callback : cardMgrCallbacks) {
        if (callback) {
           callback(status);
        } else {
           LOG(INFO, __FUNCTION__, " Callback is NULL");
        }
    }
}

std::shared_ptr<ISapCardManager> PhoneFactoryImplStub::getSapCardManager(int slotId,
    telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<ISubscriptionManager> PhoneFactoryImplStub::getSubscriptionManager(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (subscriptionManager_ == nullptr) {
       std::shared_ptr<SubscriptionManagerStub> subscriptionMgr = nullptr;
       try {
          subscriptionMgr = std::make_shared<SubscriptionManagerStub>();
       } catch (std::bad_alloc & e) {
          LOG(ERROR, __FUNCTION__ , e.what());
          return nullptr;
       }
       auto initCb = [this](telux::common::ServiceStatus status) {
          LOG(DEBUG, __FUNCTION__, " Subscription initialization callback");
          this->onSubscriptionManagerResponse(status);
       };
       auto status = subscriptionMgr->init(initCb);
       if (status != telux::common::Status::SUCCESS) {
          LOG(ERROR, __FUNCTION__, " Failed to initialize the Subscription manager");
          subscriptionManager_ = nullptr;
          return nullptr;
       }
       if (callback) {
          subscriptionMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
       subscriptionManager_ = subscriptionMgr;
    } else if (subscriptionMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " Subscription manager is not yet initialized");
       if (callback) {
          subscriptionMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " Subscription manager is initialized, invoking app callback");
       std::thread appCallback(callback, subscriptionMgrInitStatus_);
       appCallback.detach();
    } else {
       LOG(ERROR, __FUNCTION__, " Subscription manager is initialized, app Callback is NULL");
    }
    return subscriptionManager_;
}

void PhoneFactoryImplStub::onSubscriptionManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> subscriptionCallbacks;
    LOG(INFO, __FUNCTION__, " Subscription Manager initialization status: " ,
      static_cast<int>(status));
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       subscriptionMgrInitStatus_ = status;
       bool reportServiceStatus = false;
       switch(status) {
          case telux::common::ServiceStatus::SERVICE_FAILED:
             subscriptionManager_ = NULL;
             reportServiceStatus = true;
             break;
          case telux::common::ServiceStatus::SERVICE_AVAILABLE:
             reportServiceStatus = true;
             break;
          default:
             break;
       }
       if (!reportServiceStatus) {
          return;
       }
       subscriptionCallbacks = subscriptionMgrCallbacks_;
       subscriptionMgrCallbacks_.clear();
    }
    for (auto &callback : subscriptionCallbacks) {
       if (callback) {
          callback(status);
       } else {
          LOG(INFO, __FUNCTION__, " Callback is NULL");
       }
    }
}

std::shared_ptr<telux::tel::IServingSystemManager> PhoneFactoryImplStub::getServingSystemManager(
    int slotId, telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    if((slotId < DEFAULT_SLOT_ID) || (slotId > MAX_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " Invalid slotId: ", slotId);
        return nullptr;
    }
    if(!telux::common::DeviceConfig::isMultiSimSupported() && (slotId != DEFAULT_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " MultiSim not supported, for slotId: ", slotId);
        return nullptr;
    }

    std::lock_guard<std::recursive_mutex> guard(mutex_);
    // Look for servingSystemManager in the servingSystemManagerMap_
    auto mgr = servingSystemManagerMap_.find(slotId);
    if (mgr == servingSystemManagerMap_.end()) {
       LOG(DEBUG, __FUNCTION__, " Creating ServingSystem Manager for slotId ", slotId);
       std::shared_ptr<ServingSystemManagerStub> servingSystemMgr = nullptr;
       try {
           servingSystemMgr = std::make_shared<ServingSystemManagerStub>(slotId);
       } catch (std::bad_alloc & e) {
           LOG(ERROR, __FUNCTION__ , e.what());
           return nullptr;
       }
       auto initCb = [this, slotId](telux::common::ServiceStatus status) {
           LOG(DEBUG, __FUNCTION__, " ServingSystem initialization callback");
           this->onServingSystemMgrInitResponse(slotId, status);
       };
       auto status = servingSystemMgr->init(initCb);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to initialize the Serving System Manager");
            return nullptr;
        }
       servingSystemManagerMap_.emplace(slotId, servingSystemMgr);
       servingSysMgrInitStatus_.emplace(slotId,
       telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
       if (callback) {
           servingSysMgrCallbacks_[slotId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (servingSysMgrInitStatus_[slotId] ==
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " ServingSystem is not yet initialized");
       if (callback) {
           servingSysMgrCallbacks_[slotId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " ServingSystem is initialized, invoking app callback");
       std::thread appCallback(callback, servingSysMgrInitStatus_[slotId]);
       appCallback.detach();
    } else {
       LOG(DEBUG, __FUNCTION__, " ServingSystem is initialized, app callback is NULL");
    }
    return servingSystemManagerMap_[slotId];
}

std::shared_ptr<telux::tel::INetworkSelectionManager>
    PhoneFactoryImplStub::getNetworkSelectionManager(int slotId,
    telux::common::InitResponseCb  callback) {
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    if((slotId < DEFAULT_SLOT_ID) || (slotId > MAX_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " Invalid slotId: ", slotId);
        return nullptr;
    }
    if(!telux::common::DeviceConfig::isMultiSimSupported() && (slotId != DEFAULT_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " MultiSim not supported, for slotId: ", slotId);
        return nullptr;
    }

    std::lock_guard<std::recursive_mutex> guard(mutex_);
    // Look for networkSelectionManager in the networkSelectionManagerMap_
    auto mgr = networkSelectionManagerMap_.find(slotId);
    if (mgr == networkSelectionManagerMap_.end()) {
       LOG(DEBUG, __FUNCTION__, " Creating networkSelection Manager for slotId ", slotId);
       std::shared_ptr<NetworkSelectionManagerStub> networkSelectionMgr = nullptr;
       try {
           networkSelectionMgr = std::make_shared<NetworkSelectionManagerStub>(slotId);
       } catch (std::bad_alloc & e) {
           LOG(ERROR, __FUNCTION__ , e.what());
           return nullptr;
       }
       auto initCb = [this, slotId](telux::common::ServiceStatus status) {
           LOG(DEBUG, __FUNCTION__, " networkSelection manager initialization callback");
           this->onNetworkSelectionMgrInitResponse(slotId, status);
       };
       telux::common::Status status = networkSelectionMgr->init(initCb);
       if(status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to initialize networkSelection manager");
            return nullptr;
       }
       networkSelectionManagerMap_.emplace(slotId, networkSelectionMgr);
       networkSelMgrInitStatus_.emplace(slotId,
       telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
       if (callback) {
           networkSelMgrCallbacks_[slotId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (networkSelMgrInitStatus_[slotId] ==
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " NetworkSelectionManager is not yet initialized");
       if (callback) {
           networkSelMgrCallbacks_[slotId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " NetworkSelectionManager is initialized, invoking app callback");
       std::thread appCallback(callback, networkSelMgrInitStatus_[slotId]);
       appCallback.detach();
    } else {
       LOG(DEBUG, __FUNCTION__, " NetworkSelectionManager is initialized, app callback is NULL");
    }
    return networkSelectionManagerMap_[slotId];
}

void PhoneFactoryImplStub::onNetworkSelectionMgrInitResponse(int slotId,
    telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> networkSelMgrCallbacks;
    LOG(INFO, __FUNCTION__, " NetworkSelection Manager initalization status: ",
        static_cast<int>(status), " on phone: ", slotId);
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       networkSelMgrInitStatus_[slotId] = status;
       bool reportServiceStatus = false;
       switch(status) {
       case telux::common::ServiceStatus::SERVICE_FAILED:
          networkSelectionManagerMap_.erase(slotId);
          networkSelMgrInitStatus_.erase(slotId);
          reportServiceStatus = true;
          break;
       case telux::common::ServiceStatus::SERVICE_AVAILABLE:
          reportServiceStatus = true;
          break;
       default:
          break;
       }
       if(!reportServiceStatus) {
           return;
       }
       auto cbIter = networkSelMgrCallbacks_.find(slotId);
       if(cbIter != networkSelMgrCallbacks_.end()) {
          networkSelMgrCallbacks = networkSelMgrCallbacks_[slotId];
          networkSelMgrCallbacks_.erase(slotId);
       }
    }
    for (auto &callback : networkSelMgrCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(ERROR, __FUNCTION__, " Callback is NULL");
        }
    }
}

std::shared_ptr<IRemoteSimManager> PhoneFactoryImplStub::getRemoteSimManager(int slotId,
    telux::common::InitResponseCb  callback) {
    return nullptr;
}

std::shared_ptr<IMultiSimManager> PhoneFactoryImplStub::getMultiSimManager(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (multiSimManager_ == nullptr) {
       std::shared_ptr<MultiSimManagerStub> multiSimMgr = nullptr;
       try {
          multiSimMgr = std::make_shared<MultiSimManagerStub>();
       } catch (std::bad_alloc & e) {
          LOG(ERROR, __FUNCTION__ , e.what());
          return nullptr;
       }
       auto initCb = [this](telux::common::ServiceStatus status) {
          LOG(DEBUG, __FUNCTION__, " MultiSim Manager initialization callback");
          this->onMultiSimManagerResponse(status);
       };
       auto status = multiSimMgr->init(initCb);
       if (status != telux::common::Status::SUCCESS) {
          LOG(ERROR, __FUNCTION__, " Failed to initialize the Muti Sim manager");
          multiSimManager_ = nullptr;
          return nullptr;
       }
       if (callback) {
          multiSimMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
       multiSimManager_ = multiSimMgr;
    } else if (multiSimMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " MultiSim Manager is not yet initialized");
       if (callback) {
          multiSimMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " MultiSim Manager is initialized, invoking app callback");
       std::thread appCallback(callback, multiSimMgrInitStatus_);
       appCallback.detach();
    } else {
       LOG(ERROR, __FUNCTION__, " MultiSim Manager is initialized, app Callback is NULL");
    }
    return multiSimManager_;
}

void PhoneFactoryImplStub::onMultiSimManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> multiSimCallbacks;
    LOG(INFO, __FUNCTION__, " MultiSim Manager initialization status: " ,
      static_cast<int>(status));
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       multiSimMgrInitStatus_ = status;
       bool reportServiceStatus = false;
       switch(status) {
          case telux::common::ServiceStatus::SERVICE_FAILED:
             multiSimManager_ = NULL;
             reportServiceStatus = true;
             break;
          case telux::common::ServiceStatus::SERVICE_AVAILABLE:
             reportServiceStatus = true;
             break;
          default:
             break;
       }
       if (!reportServiceStatus) {
          return;
       }
       multiSimCallbacks = multiSimMgrCallbacks_;
       multiSimMgrCallbacks_.clear();
    }
    for (auto &callback : multiSimCallbacks) {
       if (callback) {
          callback(status);
       } else {
          LOG(INFO, __FUNCTION__, " Callback is NULL");
       }
    }
}

std::shared_ptr<ICellBroadcastManager> PhoneFactoryImplStub::getCellBroadcastManager(SlotId slotId,
    telux::common::InitResponseCb callback) {
    if ((slotId < DEFAULT_SLOT_ID) || (slotId > MAX_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " Invalid slotId: ", slotId);
        return nullptr;
    }
    if (!telux::common::DeviceConfig::isMultiSimSupported() && (slotId != DEFAULT_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " MultiSim not supported, slotId: ", slotId);
        return nullptr;
    }
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    auto cbMgr = cbMap_.find(slotId);
    if (cbMgr == cbMap_.end()) {
        LOG(DEBUG, __FUNCTION__, " Creating CellBroadcastManager for slot id: ", slotId);
        std::shared_ptr<CellBroadcastManagerStub> cellbroadcastMgr = nullptr;
        try {
            cellbroadcastMgr = std::make_shared<CellBroadcastManagerStub>(slotId);
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        auto initCb = [this, slotId](telux::common::ServiceStatus status) {
            LOG(DEBUG, __FUNCTION__, " CellBroadcast Manager init callback");
            this->onCellBroadcastManagerResponse(slotId, status);
        };
        auto status = cellbroadcastMgr->init(initCb);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to initialize CellBroadcast Manager");
            return nullptr;
        }
        cbMap_.emplace(slotId, cellbroadcastMgr);
        cbMgrInitStatus_.emplace(slotId, telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
        if (callback) {
            cbMgrCallbacks_[slotId].push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
    } else if (cbMgrInitStatus_[slotId] == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(DEBUG, __FUNCTION__, " CellBroadcast Manager is not yet initialized");
        if (callback) {
            cbMgrCallbacks_[slotId].push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
    } else if (callback) {
        LOG(DEBUG, __FUNCTION__, " CellBroadcast Manager is initialized, invoking app callback");
        std::thread appCallback(callback, cbMgrInitStatus_[slotId]);
        appCallback.detach();
    } else {
        LOG(DEBUG, __FUNCTION__, " CellBroadcast Manager is initialized, callback is NULL");
    }
    return cbMap_[slotId];
}

void PhoneFactoryImplStub::onCellBroadcastManagerResponse(SlotId slotId,
    telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cbMgrCallbacks;
    LOG(INFO, __FUNCTION__, " CellBroadcast Manager initialization status: " ,
      static_cast<int>(status), " on slot: ", slotId);
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       cbMgrInitStatus_[slotId] = status;
       bool reportServiceStatus = false;
       switch(status) {
          case telux::common::ServiceStatus::SERVICE_FAILED:
             cbMap_.erase(slotId);
             cbMgrInitStatus_.erase(slotId);
             reportServiceStatus = true;
             break;
          case telux::common::ServiceStatus::SERVICE_AVAILABLE:
             reportServiceStatus = true;
             break;
          default:
             break;
       }
       if (!reportServiceStatus) {
          return;
       }
       auto cbIter = cbMgrCallbacks_.find(slotId);
       if (cbIter != cbMgrCallbacks_.end()) {
          cbMgrCallbacks = cbMgrCallbacks_[slotId];
          cbMgrCallbacks_.erase(slotId);
       }
    }
    for (auto &callback : cbMgrCallbacks) {
       if (callback) {
          callback(status);
       } else {
          LOG(INFO, __FUNCTION__, " Callback is NULL");
       }
    }
}

std::shared_ptr<ISimProfileManager> PhoneFactoryImplStub::getSimProfileManager(
    telux::common::InitResponseCb  callback) {
    return nullptr;
}

std::shared_ptr<IImsSettingsManager> PhoneFactoryImplStub::getImsSettingsManager(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (imsSettingsManager_ == nullptr) {
        std::shared_ptr<ImsSettingsManagerStub> imssManagerImpl = nullptr;
        try {
            imssManagerImpl = std::make_shared<ImsSettingsManagerStub>();
        } catch (std::bad_alloc &e) {
            LOG(ERROR, __FUNCTION__, e.what());
            return nullptr;
        }
        auto initCb = std::bind(&PhoneFactoryImplStub::onImsSettingsManagerResponse, this,
            std::placeholders::_1);
        auto status = imssManagerImpl->init(initCb);
        if (status != telux::common::Status::SUCCESS) {
          LOG(ERROR, __FUNCTION__, " Failed to initialize the IMS settings manager");
          imsSettingsManager_ = nullptr;
          return nullptr;
        }
        LOG(DEBUG, __FUNCTION__, " initialize the IMS settings manager");
        if (callback) {
            imssCallbacks_.push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
        imsSettingsManager_ = imssManagerImpl;
    } else if (imssInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(DEBUG, __FUNCTION__, " IMS settings manager is not yet initialized");
        if (callback) {
            imssCallbacks_.push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
    } else if (callback) {
        LOG(DEBUG, __FUNCTION__, " IMS settings manager is initialized, invoking app callback");
        std::thread appCallback(callback, imssInitStatus_);
        appCallback.detach();
    } else {
        LOG(ERROR, __FUNCTION__, " IMS settings manager is initialized, app Callback is NULL");
    }
    return imsSettingsManager_;
}

void PhoneFactoryImplStub::onImsSettingsManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> imssCallbacks;
    LOG(INFO, __FUNCTION__,
        " Ims Settings Manager initialization status: ", static_cast<int>(status));
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        imssInitStatus_          = status;
        bool reportServiceStatus = false;
        switch (status) {
            case telux::common::ServiceStatus::SERVICE_FAILED:
                imsSettingsManager_ = NULL;
                reportServiceStatus = true;
                break;
            case telux::common::ServiceStatus::SERVICE_AVAILABLE:
                reportServiceStatus = true;
                break;
            default:
                break;
        }
        if (!reportServiceStatus) {
            return;
        }
        imssCallbacks = imssCallbacks_;
        imssCallbacks_.clear();
    }
    for (auto &callback : imssCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(INFO, __FUNCTION__, " Callback is NULL");
        }
    }
}

std::shared_ptr<IEcallManager> PhoneFactoryImplStub::getEcallManager(
    telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<IHttpTransactionManager> PhoneFactoryImplStub::getHttpTransactionManager(
    telux::common::InitResponseCb  callback) {
    return nullptr;
}
std::shared_ptr<IImsServingSystemManager> PhoneFactoryImplStub::getImsServingSystemManager(
    SlotId slotId, telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    if((slotId < DEFAULT_SLOT_ID) || (slotId > MAX_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " Invalid slotId: ", slotId);
        return nullptr;
    }
    if(!telux::common::DeviceConfig::isMultiSimSupported() && (slotId != DEFAULT_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " MultiSim not supported, for slotId: ", slotId);
        return nullptr;
    }

    std::lock_guard<std::recursive_mutex> guard(mutex_);
    // Look for imsServingSystemManager in the imsServSysManagerMap_
    auto ims = imsServSysManagerMap_.find(slotId);
    if (ims == imsServSysManagerMap_.end()) {
       LOG(DEBUG, __FUNCTION__, " Creating IMSServingSystem Manager for slotId ", slotId);
       std::shared_ptr<ImsServingSystemManagerStub> imsMgr = nullptr;
       try {
           imsMgr = std::make_shared<ImsServingSystemManagerStub>(slotId);
       } catch (std::bad_alloc & e) {
           LOG(ERROR, __FUNCTION__ , e.what());
           return nullptr;
       }
       auto initCb = [this, slotId](telux::common::ServiceStatus status) {
           LOG(DEBUG, __FUNCTION__, " IMSServingSystem initialization callback");
           this->onImsServingSystemMgrInitResponse(slotId, status);
       };
       auto status = imsMgr->init(initCb);
       if (status != telux::common::Status::SUCCESS) {
          LOG(ERROR, __FUNCTION__, " Failed to initialize the IMSServingSystem manager");
          return nullptr;
       }
       LOG(DEBUG, __FUNCTION__, " initialize the IMSServingSystem manager");
       imsServSysManagerMap_.emplace(slotId, imsMgr);
       imsServingSystemMgrInitStatus_.emplace(slotId,
       telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
       if (callback) {
           imsServSysCallbacks_[slotId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (imsServingSystemMgrInitStatus_[slotId] ==
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " IMSServingSystem is not yet initialized");
       if (callback) {
           imsServSysCallbacks_[slotId].push_back(callback);
       } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " IMSServingSystem is initialized, invoking app callback");
       std::thread appCallback(callback, imsServingSystemMgrInitStatus_[slotId]);
       appCallback.detach();
    } else {
       LOG(DEBUG, __FUNCTION__, " IMSServingSystem is initialized, app callback is NULL");
    }
    return imsServSysManagerMap_[slotId];
}

void PhoneFactoryImplStub::onServingSystemMgrInitResponse(int slotId,
    telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> servingSystemMgrCallbacks;
    LOG(INFO, __FUNCTION__, " ServingSystem Manager initalization status: ",
        static_cast<int>(status), " on phone: ", slotId);
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       servingSysMgrInitStatus_[slotId] = status;
       bool reportServiceStatus = false;
       switch(status) {
       case telux::common::ServiceStatus::SERVICE_FAILED:
          servingSystemManagerMap_.erase(slotId);
          servingSysMgrInitStatus_.erase(slotId);
          reportServiceStatus = true;
          break;
       case telux::common::ServiceStatus::SERVICE_AVAILABLE:
          reportServiceStatus = true;
          break;
       default:
          break;
       }
       if(!reportServiceStatus) {
           return;
       }
       auto cbIter = servingSysMgrCallbacks_.find(slotId);
       if(cbIter != servingSysMgrCallbacks_.end()) {
          servingSystemMgrCallbacks = servingSysMgrCallbacks_[slotId];
          servingSysMgrCallbacks_.erase(slotId);
       }
    }
    for (auto &callback : servingSystemMgrCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(ERROR, __FUNCTION__, " Callback is NULL");
        }
    }
}

void PhoneFactoryImplStub::onImsServingSystemMgrInitResponse(SlotId slotId,
    telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> imsServingSystemMgrCallbacks;
    LOG(INFO, __FUNCTION__, " IMSServingSystem Manager initalization status: ",
        static_cast<int>(status), " on phone: ", slotId);
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       imsServingSystemMgrInitStatus_[slotId] = status;
       bool reportServiceStatus = false;
       switch(status) {
       case telux::common::ServiceStatus::SERVICE_FAILED:
          imsServSysManagerMap_.erase(slotId);
          imsServingSystemMgrInitStatus_.erase(slotId);
          reportServiceStatus = true;
          break;
       case telux::common::ServiceStatus::SERVICE_AVAILABLE:
          reportServiceStatus = true;
          break;
       default:
          break;
       }
       if(!reportServiceStatus) {
           return;
       }
       auto cbIter = imsServSysCallbacks_.find(slotId);
       if(cbIter != imsServSysCallbacks_.end()) {
          imsServingSystemMgrCallbacks = imsServSysCallbacks_[slotId];
          imsServSysCallbacks_.erase(slotId);
       }
    }
    for (auto &callback : imsServingSystemMgrCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(ERROR, __FUNCTION__, " Callback is NULL");
        }
    }
}

void PhoneFactoryImplStub::onSuppSvcInitResponse(SlotId slotId, telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Supp Service Manager init status : ", static_cast<int>(status));
    std::vector<telux::common::InitResponseCb> suppSvcCallbacks;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        suppSvcInitStatus_[slotId] = status;
        bool reportedServiceStatus = false;
        switch (status) {
            case telux::common::ServiceStatus::SERVICE_FAILED:
                suppSvcManagerMap_.erase(slotId);
                suppSvcInitStatus_.erase(slotId);
                reportedServiceStatus = true;
                break;
            case telux::common::ServiceStatus::SERVICE_AVAILABLE:
                reportedServiceStatus = true;
                break;
            default:
                break;
        }
        if (!reportedServiceStatus) {
            return;
        }
        auto cbIter = suppSvcCallbacks_.find(slotId);
        if (cbIter != suppSvcCallbacks_.end()) {
            suppSvcCallbacks = suppSvcCallbacks_[slotId];
            suppSvcCallbacks_.erase(slotId);
        }
    }
    for (auto &callback : suppSvcCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(INFO, __FUNCTION__, " Callback is NULL");
        }
    }
}

std::shared_ptr<ISuppServicesManager> PhoneFactoryImplStub::getSuppServicesManager(
    SlotId slotId, telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if ((slotId < DEFAULT_SLOT_ID) || (slotId > MAX_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " Invalid slotId: ", slotId);
        return nullptr;
    }
    if (!telux::common::DeviceConfig::isMultiSimSupported() && (slotId != DEFAULT_SLOT_ID)) {
        LOG(ERROR, __FUNCTION__, " MultiSim not supported, slotId: ", slotId);
        return nullptr;
    }
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    auto suppServiceMgr = suppSvcManagerMap_.find(slotId);
    if (suppServiceMgr == suppSvcManagerMap_.end()) {
        std::shared_ptr<SuppServicesManagerStub> suppSvcManagerImpl = nullptr;
        try {
            suppSvcManagerImpl = std::make_shared<SuppServicesManagerStub>(slotId);
        } catch (std::bad_alloc &e) {
            LOG(ERROR, __FUNCTION__, e.what());
            return nullptr;
        }
        auto initCb = [this, slotId](telux::common::ServiceStatus status) {
            LOG(DEBUG, __FUNCTION__, " Supplementary service manager init callback");
            this->onSuppSvcInitResponse(slotId, status);
        };
        auto status = suppSvcManagerImpl->init(initCb);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to initialize the SuppServices manager");
            return nullptr;
        }
        suppSvcManagerMap_.emplace(slotId, suppSvcManagerImpl);
        suppSvcInitStatus_.emplace(slotId, telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
        LOG(DEBUG, __FUNCTION__, " SuppServicesManager initialized");
        if (callback) {
            suppSvcCallbacks_[slotId].push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
    } else if (suppSvcInitStatus_[slotId] == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        if (callback) {
            suppSvcCallbacks_[slotId].push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Call back is null");
        }
    } else if (callback) {
        std::thread appCallback(callback, suppSvcInitStatus_[slotId]);
        appCallback.detach();
    } else {
        LOG(ERROR, __FUNCTION__, " Supp Svc Manager is initialized, Callback is NULL");
    }
    return suppSvcManagerMap_[slotId];
}

std::shared_ptr<IApSimProfileManager> PhoneFactoryImplStub::getApSimProfileManager(
    telux::common::InitResponseCb callback) {
    return nullptr;
}

}  // namespace tel
}  // namespace telux
