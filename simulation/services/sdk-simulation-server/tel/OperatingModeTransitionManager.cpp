/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <telux/common/DeviceConfig.hpp>
#include "libs/common/CommonUtils.hpp"
#include "libs/common/Logger.hpp"
#include "libs/tel/TelDefinesStub.hpp"
#include "OperatingModeTransitionManager.hpp"
#include "TelUtil.hpp"
#include <thread>
#include <chrono>

#define TEL_PHONE_MANAGER                       "IPhoneManager"

static std::string phMgrJsonApiPaths[] = {
    "api/tel/IPhoneManagerSlot1.json",
    "api/tel/IPhoneManagerSlot2.json"
};

static std::string phMgrJsonSystemStatePaths[] = {
    "system-state/tel/IPhoneManagerStateSlot1.json",
    "system-state/tel/IPhoneManagerStateSlot2.json"
};

namespace telux {
namespace tel {

FactoryTestMode::FactoryTestMode(std::weak_ptr<BaseStateMachine> parent)
    : BaseState("FactoryTestMode",
    OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST, parent) {
}

bool FactoryTestMode::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, " FactoryTestMode: ", __FUNCTION__,
        " Received event: ", event->name_, " current state: ", name_);
    return true;
}

void FactoryTestMode::onEnter() {
    LOG(DEBUG, " FactoryTestMode: ", __FUNCTION__);
    if(parent_.lock()) {
        std::shared_ptr<OperatingModeTransitionManager> opTmMgr =
            std::static_pointer_cast<OperatingModeTransitionManager>(parent_.lock());
        if (opTmMgr) {
            if (opTmMgr->getPrevState() != nullptr) {
                int prevState = opTmMgr->getPrevState()->getCurrentState();
                if (prevState == OperatingModeTransitionManager::StateID::STATE_ONLINE) {
                    opTmMgr->notifyAll(telStub::OperatingMode::FACTORY_TEST);
                } else if(prevState ==
                    OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_AIRPLANE ||
                    prevState ==
                        OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER) {
                    opTmMgr->notifyOperatingMode(telStub::OperatingMode::FACTORY_TEST);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_OFFLINE) {
                    LOG(DEBUG, __FUNCTION__,
                        " No state change invalid transaction, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_RESETTING) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_SHUTDOWN) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                }
            } else {
                LOG(ERROR, " Prev State is null");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " OpTmMgr is null");
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " weakptr is null");
    }
}

void FactoryTestMode::onExit() {
    LOG(DEBUG, " FactoryTestMode: ", __FUNCTION__);
}

OnlineMode::OnlineMode(std::weak_ptr<BaseStateMachine> parent)
    : BaseState("OnlineMode",
    OperatingModeTransitionManager::StateID::STATE_ONLINE, parent) {
}

bool OnlineMode::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, " OnlineMode: ", __FUNCTION__,
        " Received event: ", event->name_, " current state: ", name_);
    return true;
}

void OnlineMode::onEnter() {
    LOG(DEBUG, " OnlineMode: ", __FUNCTION__);
    if(parent_.lock()) {
        std::shared_ptr<OperatingModeTransitionManager> opTmMgr =
            std::static_pointer_cast<OperatingModeTransitionManager>(parent_.lock());
        if (opTmMgr) {
            if (opTmMgr->getPrevState() != nullptr) {
                int prevState = opTmMgr->getPrevState()->getCurrentState();
                if (prevState == OperatingModeTransitionManager::StateID::STATE_ONLINE) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState ==
                    OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST || prevState ==
                        OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER) {
                    opTmMgr->notifyAll(telStub::OperatingMode::ONLINE);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_AIRPLANE) {
                    opTmMgr->notifyAll(telStub::OperatingMode::ONLINE);
                    //Add operator info
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_OFFLINE) {
                    LOG(DEBUG, __FUNCTION__,
                        " No state change invalid transaction, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_RESETTING) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_SHUTDOWN) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                }
            } else {
                LOG(ERROR, " Prev State is null");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " OpTmMgr is null");
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " weakptr is null");
    }
}

void OnlineMode::onExit() {
    LOG(DEBUG, " OnlineMode: ", __FUNCTION__);
}

OfflineMode::OfflineMode(std::weak_ptr<BaseStateMachine> parent)
    : BaseState("OfflineMode",
    OperatingModeTransitionManager::StateID::STATE_OFFLINE, parent) {
}

bool OfflineMode::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, " OfflineMode: ", __FUNCTION__,
        " Received event: ", event->name_, " current state: ", name_);
    return true;
}

void OfflineMode::onEnter() {
    LOG(DEBUG, " OfflineMode: ", __FUNCTION__);
    if(parent_.lock()) {
        std::shared_ptr<OperatingModeTransitionManager> opTmMgr =
            std::static_pointer_cast<OperatingModeTransitionManager>(parent_.lock());
        if (opTmMgr) {
            if (opTmMgr->getPrevState() != nullptr) {
                int prevState = opTmMgr->getPrevState()->getCurrentState();
                if (prevState == OperatingModeTransitionManager::StateID::STATE_ONLINE) {
                    opTmMgr->notifyAll(telStub::OperatingMode::OFFLINE);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST
                    || prevState == OperatingModeTransitionManager::StateID::STATE_AIRPLANE ||
                    prevState ==
                    OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER) {
                    opTmMgr->notifyOperatingMode(telStub::OperatingMode::OFFLINE);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_OFFLINE) {
                    LOG(DEBUG, __FUNCTION__,
                        " No state change , hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_RESETTING) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_SHUTDOWN) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                }
            } else {
                LOG(ERROR, " Prev State is null");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " OpTmMgr is null");
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " weakptr is null");
    }
}

void OfflineMode::onExit() {
    LOG(DEBUG, " OfflineMode: ", __FUNCTION__);
}

PersistentLowPowerMode::PersistentLowPowerMode(std::weak_ptr<BaseStateMachine> parent)
    : BaseState("PersistentLowPowerMode",
    OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER, parent) {
}

bool PersistentLowPowerMode::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, " PersistentLowPowerMode: ", __FUNCTION__,
        " Received event: ", event->name_, " current state: ", name_);
    return true;
}

void PersistentLowPowerMode::onEnter() {
    LOG(DEBUG, " PersistentLowPowerMode: ", __FUNCTION__);
    if(parent_.lock()) {
        std::shared_ptr<OperatingModeTransitionManager> opTmMgr =
            std::static_pointer_cast<OperatingModeTransitionManager>(parent_.lock());
        if (opTmMgr) {
            if (opTmMgr->getPrevState() != nullptr) {
                int prevState = opTmMgr->getPrevState()->getCurrentState();
                if (prevState == OperatingModeTransitionManager::StateID::STATE_ONLINE) {
                    opTmMgr->notifyAll(telStub::OperatingMode::PERSISTENT_LOW_POWER);
                } else if(prevState ==
                    OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST || prevState ==
                    OperatingModeTransitionManager::StateID::STATE_AIRPLANE) {
                    opTmMgr->notifyOperatingMode(telStub::OperatingMode::PERSISTENT_LOW_POWER);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_OFFLINE) {
                    LOG(DEBUG, __FUNCTION__,
                        " No state change invalid transaction, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_RESETTING) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_SHUTDOWN) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState ==
                    OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                }
            } else {
                LOG(ERROR, " Prev State is null");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " OpTmMgr is null");
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " weakptr is null");
    }
}

void PersistentLowPowerMode::onExit() {
    LOG(DEBUG, " PersistentLowPowerMode: ", __FUNCTION__);
}

AirplaneMode::AirplaneMode(std::weak_ptr<BaseStateMachine> parent)
    : BaseState("AirplaneMode",
    OperatingModeTransitionManager::StateID::STATE_AIRPLANE, parent) {
}

bool AirplaneMode::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, " AirplaneMode: ", __FUNCTION__,
        " Received event: ", event->name_, " current state: ", name_);
    return true;
}

void AirplaneMode::onEnter() {
    LOG(DEBUG, " AirplaneMode: ", __FUNCTION__);
    if(parent_.lock()) {
        std::shared_ptr<OperatingModeTransitionManager> opTmMgr =
            std::static_pointer_cast<OperatingModeTransitionManager>(parent_.lock());
        if (opTmMgr) {
            if (opTmMgr->getPrevState() != nullptr) {
                int prevState = opTmMgr->getPrevState()->getCurrentState();
                if (prevState == OperatingModeTransitionManager::StateID::STATE_ONLINE) {
                    opTmMgr->notifyAll(telStub::OperatingMode::AIRPLANE);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST)
                    {
                    opTmMgr->notifyOperatingMode(telStub::OperatingMode::AIRPLANE);
                    telStub::VoiceServiceStateEvent voiceServiceStateEvent =
                        TelUtil::createVoiceServiceStateEvent(
                        SLOT_ID_1, telStub::VoiceServiceState::NOT_REG_AND_SEARCHING,
                        telStub::VoiceServiceDenialCause::GENERAL,
                        telStub::RadioTechnology::RADIO_TECH_UNKNOWN);
                    opTmMgr->getBuilder()->addVoiceServiceStateChangeEvent(SLOT_ID_1,
                        voiceServiceStateEvent);
                    std::shared_ptr<Notification> notification = opTmMgr->getBuilder()->build();
                    notification->notify();
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_AIRPLANE) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_OFFLINE) {
                    LOG(DEBUG, __FUNCTION__,
                        " No state change invalid transaction, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_RESETTING) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_SHUTDOWN) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState ==
                    OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER) {
                    opTmMgr->notifyOperatingMode(telStub::OperatingMode::AIRPLANE);
                }
            } else {
                LOG(ERROR, " Prev State is null");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " OpTmMgr is null");
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " weakptr is null");
    }
}

void AirplaneMode::onExit() {
    LOG(DEBUG, " AirplaneMode: ", __FUNCTION__);
    LOG(DEBUG, __FUNCTION__);
}

ResettingMode::ResettingMode(std::weak_ptr<BaseStateMachine> parent)
    : BaseState("ResettingMode",
    OperatingModeTransitionManager::StateID::STATE_RESETTING, parent) {
}

bool ResettingMode::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, " ResettingMode: ", __FUNCTION__,
        " Received event: ", event->name_, " current state: ", name_);
    return true;
}

void ResettingMode::onEnter() {
    LOG(DEBUG, " ResettingMode: ", __FUNCTION__);
    if(parent_.lock()) {
        std::shared_ptr<OperatingModeTransitionManager> opTmMgr =
            std::static_pointer_cast<OperatingModeTransitionManager>(parent_.lock());
        if (opTmMgr) {
            if (opTmMgr->getPrevState() != nullptr) {
                int prevState = opTmMgr->getPrevState()->getCurrentState();
                if (prevState == OperatingModeTransitionManager::StateID::STATE_ONLINE ||
                    prevState == OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST ||
                    prevState == OperatingModeTransitionManager::StateID::STATE_AIRPLANE ||
                    prevState == OperatingModeTransitionManager::StateID::STATE_OFFLINE ||
                    prevState ==
                        OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER) {
                    opTmMgr->notifyOperatingMode(telStub::OperatingMode::RESETTING);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_RESETTING) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_SHUTDOWN) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                }
            } else {
                LOG(ERROR, " Prev State is null");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " OpTmMgr is null");
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " weakptr is null");
    }
}

void ResettingMode::onExit() {
    LOG(DEBUG, " ResettingMode: ", __FUNCTION__);
}

ShutdownMode::ShutdownMode(std::weak_ptr<BaseStateMachine> parent)
    : BaseState("ShutdownMode",
    OperatingModeTransitionManager::StateID::STATE_SHUTDOWN, parent) {
}

bool ShutdownMode::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, " ShutdownMode: ", __FUNCTION__,
        " Received event: ", event->name_, " current state: ", name_);
    return true;
}

void ShutdownMode::onEnter() {
    LOG(DEBUG, " ShutdownMode: ", __FUNCTION__);
    if(parent_.lock()) {
        std::shared_ptr<OperatingModeTransitionManager> opTmMgr =
            std::static_pointer_cast<OperatingModeTransitionManager>(parent_.lock());
        if (opTmMgr) {
            if (opTmMgr->getPrevState() != nullptr) {
                int prevState = opTmMgr->getPrevState()->getCurrentState();
                if (prevState == OperatingModeTransitionManager::StateID::STATE_ONLINE ||
                    prevState == OperatingModeTransitionManager::StateID::STATE_FACTORY_TEST ||
                    prevState == OperatingModeTransitionManager::StateID::STATE_AIRPLANE ||
                    prevState ==
                        OperatingModeTransitionManager::StateID::STATE_PERSISTENT_LOW_POWER) {
                    opTmMgr->notifyOperatingMode(telStub::OperatingMode::SHUTTING_DOWN);
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_OFFLINE) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_RESETTING) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                } else if(prevState == OperatingModeTransitionManager::StateID::STATE_SHUTDOWN) {
                    LOG(DEBUG, __FUNCTION__, " No state change, hence no notification");
                }
            } else {
                LOG(ERROR, " Prev State is null");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " OpTmMgr is null");
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " weakptr is null");
    }
}

void ShutdownMode::onExit() {
    LOG(DEBUG, " ShutdownMode: ", __FUNCTION__);
}

OperatingModeTransitionManager::OperatingModeTransitionManager() :
    BaseStateMachine("OperatingModeTransitionManager") {
    LOG(DEBUG, __FUNCTION__);
}

/**
 * Overridden start method, would move the state machine to Online
 */
void OperatingModeTransitionManager::start() {
    BaseStateMachine::start();
    LOG(DEBUG, __FUNCTION__);
}

std::shared_ptr<TelephonyNotificationBuilder> OperatingModeTransitionManager::getBuilder() {
    return notificationBuilder_;
}

/**
 * Overridden stop method, clears all internal states and variables
 */
void OperatingModeTransitionManager::stop() {
    LOG(DEBUG, __FUNCTION__);
    BaseStateMachine::stop();
}

/**
 * Top-level event handler for the state-machine
 * Handles the incoming events, identifies the event and passes on further
 * to the current state for further handling
 * @param [in] event - The TelEvent that needs to be handled
 * @returns true if the event was handled
 */
bool OperatingModeTransitionManager::onEvent(std::shared_ptr<telux::common::Event> event) {

    bool eventStatus = false;
    if(event->id_ ==
        static_cast<int>(OperatingModeTransitionManager::EventID::UPDATE_OPERATING_MODE)) {
        std::shared_ptr<PhoneEvent> phoneEvent
                    = std::dynamic_pointer_cast<PhoneEvent>(event);
        telStub::OperatingMode operatingMode = phoneEvent->getOperatingMode();
        if (prevState_ != nullptr) {
            int prevStateId = prevState_->getCurrentState();
            int currentStateId = static_cast<int>(operatingMode);
            LOG(DEBUG, __FUNCTION__, " currentStateId:", currentStateId, " prevStateId:",
                prevStateId);
            if (prevStateId == STATE_OFFLINE && ((currentStateId == STATE_ONLINE) ||
                (currentStateId == STATE_PERSISTENT_LOW_POWER) || (currentStateId == STATE_AIRPLANE)
                || (currentStateId == STATE_FACTORY_TEST))) {
                LOG(DEBUG, __FUNCTION__, " INVALID_TRANSITION");
                return false;
            }
        }
        switch(operatingMode) {
            case telStub::OperatingMode::ONLINE:
                changeState(std::make_shared<telux::tel::OnlineMode>(shared_from_this()));
                break;
            case telStub::OperatingMode::AIRPLANE:
                changeState(std::make_shared<telux::tel::AirplaneMode>(shared_from_this()));
                break;
            case telStub::OperatingMode::FACTORY_TEST:
                changeState(std::make_shared<telux::tel::FactoryTestMode>(shared_from_this()));
                break;
            case telStub::OperatingMode::OFFLINE:
                changeState(std::make_shared<telux::tel::OfflineMode>(shared_from_this()));
                break;
            case telStub::OperatingMode::RESETTING:
                changeState(std::make_shared<telux::tel::ResettingMode>(shared_from_this()));
                break;
            case telStub::OperatingMode::SHUTTING_DOWN:
                changeState(std::make_shared<telux::tel::ShutdownMode>(shared_from_this()));
                break;
            case telStub::OperatingMode::PERSISTENT_LOW_POWER:
                changeState(std::make_shared<telux::tel::PersistentLowPowerMode>(
                    shared_from_this()));
                break;
            default:
                LOG(ERROR, __FUNCTION__, " Invalid operating mode");
                return false;
        }

        if (prevState_ != nullptr) {
            LOG(DEBUG, __FUNCTION__, " PrevState:", prevState_->getCurrentState());
        } else {
            LOG(DEBUG, __FUNCTION__, " Previous State is null on init");
        }

        if (currentState_ != nullptr) {
            LOG(DEBUG, __FUNCTION__, " CurrentState:", currentState_->getCurrentState());
        }
        prevState_ = currentState_;
        eventStatus = true;
    }
    return eventStatus;
}

grpc::Status OperatingModeTransitionManager::init() {
    LOG(DEBUG, __FUNCTION__);
    notificationBuilder_ = std::make_shared<telux::tel::TelephonyNotificationBuilder>();
    notificationBuilder_->init();
    std::string errorString = "";
    telux::common::ErrorCode error;
    do {
        error = initOperatingMode();
        if (error != telux::common::ErrorCode::SUCCESS) {
            errorString = " Get Operating mode during init failed";
            break;
        }
        error = initSignalStrength();
        if (error != telux::common::ErrorCode::SUCCESS) {
            errorString = " Get signal strength during init failed";
            break;
        }
        error = initServingRat();
        if (error != telux::common::ErrorCode::SUCCESS) {
            errorString = " Get Serving RAT during init failed";
            break;
        }
    } while(0);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, errorString);
        return grpc::Status(grpc::StatusCode::INTERNAL, errorString);
    }
    return grpc::Status::OK;
}

telux::common::ErrorCode OperatingModeTransitionManager::initSignalStrength() {
    LOG(DEBUG, __FUNCTION__);
    telStub::SignalStrength signalStrength {};
    telux::common::ErrorCode error =
        TelUtil::readSignalStrengthFromJsonFile(SLOT_ID_1, signalStrength);
    if (error == telux::common::ErrorCode::SUCCESS) {
        cachedSS_[SLOT_ID_1] = signalStrength;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
            signalStrength = {};
            error = TelUtil::readSignalStrengthFromJsonFile(SLOT_ID_2, signalStrength);
            if (error == telux::common::ErrorCode::SUCCESS) {
                cachedSS_[SLOT_ID_2] = signalStrength;
            } else {
                LOG(ERROR, __FUNCTION__, " Unable to read signal strength from slot2");
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read signal strength from slot1");
    }
    return error;
}

telux::common::ErrorCode OperatingModeTransitionManager::initServingRat() {
    LOG(DEBUG, __FUNCTION__);
    telStub::RadioTechnology rat {};
    telStub::ServiceDomainInfo_Domain domain {};
    telux::common::ErrorCode error =
        TelUtil::readSystemInfoFromJsonFile(SLOT_ID_1, rat, domain);
    if (error == telux::common::ErrorCode::SUCCESS) {
        cachedServingRat_[SLOT_ID_1] = rat;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
            error = TelUtil::readSystemInfoFromJsonFile(SLOT_ID_2, rat, domain);
            if (error == telux::common::ErrorCode::SUCCESS) {
                cachedServingRat_[SLOT_ID_2] = rat;
            } else {
                LOG(ERROR, __FUNCTION__, " Unable to read RAT from slot2");
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read RAT from slot1");
    }
    return error;
}

telux::common::ErrorCode OperatingModeTransitionManager::updateCachedSignalStrength(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    telStub::SignalStrength signalStrength {};
    telux::common::ErrorCode error =
        TelUtil::readSignalStrengthFromJsonFile(slotId, signalStrength);
    if (error == telux::common::ErrorCode::SUCCESS) {
        cachedSS_[slotId] = signalStrength;
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read signal strength for ", slotId);
    }
    return error;
}

telux::common::ErrorCode OperatingModeTransitionManager::updateCachedServingRat(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    telStub::RadioTechnology rat {};
    telStub::ServiceDomainInfo_Domain domain {};
    telux::common::ErrorCode error =
        TelUtil::readSystemInfoFromJsonFile(slotId, rat, domain);
    if (error == telux::common::ErrorCode::SUCCESS) {
        cachedServingRat_[slotId] = rat;
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read RAT for ", slotId);
    }
    return error;
}

telux::common::ErrorCode OperatingModeTransitionManager::initOperatingMode() {
    LOG(DEBUG, __FUNCTION__);
    telStub::OperatingMode operatingMode;
    telux::common::ErrorCode error = getOperatingMode(operatingMode);
    if (error == telux::common::ErrorCode::SUCCESS) {
        updateOperatingMode(operatingMode);
    }
    switch(operatingMode) {
        case telStub::OperatingMode::ONLINE:
            prevState_ = std::make_shared<telux::tel::OnlineMode>(shared_from_this());
            break;
        case telStub::OperatingMode::AIRPLANE:
            prevState_ = std::make_shared<telux::tel::AirplaneMode>(shared_from_this());
            break;
        case telStub::OperatingMode::FACTORY_TEST:
            prevState_ = std::make_shared<telux::tel::FactoryTestMode>(shared_from_this());
            break;
        case telStub::OperatingMode::OFFLINE:
            prevState_ = std::make_shared<telux::tel::OfflineMode>(shared_from_this());
            break;
        case telStub::OperatingMode::RESETTING:
            prevState_ = std::make_shared<telux::tel::ResettingMode>(shared_from_this());
            break;
        case telStub::OperatingMode::SHUTTING_DOWN:
            prevState_ = std::make_shared<telux::tel::ShutdownMode>(shared_from_this());
            break;
        case telStub::OperatingMode::PERSISTENT_LOW_POWER:
            prevState_ = std::make_shared<telux::tel::PersistentLowPowerMode>(shared_from_this());
            break;
        default:
            LOG(ERROR, __FUNCTION__, " Invalid operating mode");
            break;
    }
    return error;
}

telux::common::ErrorCode OperatingModeTransitionManager::getOperatingMode(
    telStub::OperatingMode &operatingMode) {
    LOG(DEBUG, __FUNCTION__);
    telStub::OperatingModeEvent event;
    telux::common::ErrorCode error = TelUtil::readOperatingModeEventFromJsonFile(event);
    operatingMode = event.operating_mode();
    return error;
}

telux::common::ErrorCode OperatingModeTransitionManager::updateOperatingMode(
    telStub::OperatingMode operatingMode) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<telux::common::Event> event = createEvent(UPDATE_OPERATING_MODE,
        "updateOperatingMode", DEFAULT_SLOT_ID);
    std::shared_ptr<PhoneEvent> phoneEvent = std::dynamic_pointer_cast<PhoneEvent>(event);
    phoneEvent->setOperatingMode(operatingMode);
    if (!onEvent(event)) {
        return telux::common::ErrorCode::INTERNAL_ERR;
    }
    return telux::common::ErrorCode::SUCCESS;
}

::telStub::SignalStrength& OperatingModeTransitionManager::getCachedSS(int slotId) {
    if (cachedSS_.find(slotId) != cachedSS_.end()) {
        LOG(DEBUG, __FUNCTION__, " Key exist: ", slotId);
    } else {
        LOG(DEBUG, __FUNCTION__, " Key does not exist: ", slotId);
        telStub::SignalStrength strength {};
        cachedSS_[slotId] = strength;
    }
    return cachedSS_[slotId];
}

::telStub::RadioTechnology& OperatingModeTransitionManager::getCachedServingRat(int slotId) {
    if (cachedServingRat_.find(slotId) != cachedServingRat_.end()) {
        LOG(DEBUG, __FUNCTION__, " Key exist: ", slotId);
    } else {
        LOG(DEBUG, __FUNCTION__, " Key does not exist: ", slotId);
        telStub::RadioTechnology rat {};
        cachedServingRat_[slotId] = rat;
    }
    return cachedServingRat_[slotId];
}

telux::common::ServiceStatus OperatingModeTransitionManager::readSubsystemStatus(int slotId) {
    int cbDelay = 0;
    return readSubsystemStatus(slotId, cbDelay);
}

telux::common::ServiceStatus OperatingModeTransitionManager::readSubsystemStatus(int slotId,
    int &cbDelay) {
    Json::Value rootObj;
    std::string filePath = (slotId == SLOT_ID_1)? phMgrJsonApiPaths[0] : phMgrJsonApiPaths[1];
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }

    cbDelay = rootObj[TEL_PHONE_MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj[TEL_PHONE_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus, " slotId::", slotId);
    return status;
}

telux::common::ErrorCode OperatingModeTransitionManager::readJsonData(int slotId,
    std::string method, JsonData &data) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (slotId == SLOT_ID_1)? phMgrJsonApiPaths[0] : phMgrJsonApiPaths[1];
    std::string subsystem = TEL_PHONE_MANAGER;
    std::string stateJsonPath = (slotId == SLOT_ID_1)?
        phMgrJsonSystemStatePaths[0] : phMgrJsonSystemStatePaths[1];
    return CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
}

telux::common::ErrorCode OperatingModeTransitionManager::readJsonData(int slotId,
    std::string method, JsonData &data, std::string &stateJsonPath) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (slotId == SLOT_ID_1)? phMgrJsonApiPaths[0] : phMgrJsonApiPaths[1];
    std::string subsystem = TEL_PHONE_MANAGER;
    stateJsonPath = (slotId == SLOT_ID_1)? phMgrJsonSystemStatePaths[0] : phMgrJsonSystemStatePaths[1];
    return CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
}

void OperatingModeTransitionManager::notifyAll(telStub::OperatingMode mode) {
    int noOfSlots = 1;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        noOfSlots = 2;
    }
    std::shared_ptr<TelephonyNotificationBuilder> notificationBuilder = getBuilder();
    if (mode == telStub::OperatingMode::FACTORY_TEST ||
        mode == telStub::OperatingMode::OFFLINE||
        mode == telStub::OperatingMode::PERSISTENT_LOW_POWER ||
        mode == telStub::OperatingMode::AIRPLANE ||
        mode == telStub::OperatingMode::ONLINE) {
        telStub::OperatingModeEvent opModeEvent = TelUtil::createOperatingModeEvent(mode);
        notificationBuilder->addOperatingModeChangeEvent(opModeEvent);
    }

    for (int slotId = 1 ; slotId <= noOfSlots; slotId++){
        if (mode == telStub::OperatingMode::FACTORY_TEST ||
            mode == telStub::OperatingMode::OFFLINE||
            mode == telStub::OperatingMode::PERSISTENT_LOW_POWER ||
            mode == telStub::OperatingMode::AIRPLANE) {

            telStub::ServiceStateChangeEvent serviceStateChangeEvent =
                TelUtil::createServiceStateEvent(slotId, telStub::ServiceState::OUT_OF_SERVICE);
            notificationBuilder->addServiceStateChangeEvent(slotId,serviceStateChangeEvent);

            telStub::VoiceServiceStateEvent voiceServiceStateEvent =
                TelUtil::createVoiceServiceStateEvent(slotId,
                telStub::VoiceServiceState::NOT_REG_AND_SEARCHING,
                telStub::VoiceServiceDenialCause::GENERAL,
                telStub::RadioTechnology::RADIO_TECH_UNKNOWN);
            notificationBuilder->addVoiceServiceStateChangeEvent(slotId, voiceServiceStateEvent);

            telStub::SignalStrengthChangeEvent signalStrengthChangeEvent =
                TelUtil::createSignalStrengthWithDefaultValues(slotId);
            notificationBuilder->addSignalStrengthChangeEvent(slotId, signalStrengthChangeEvent);

            telStub::VoiceRadioTechnologyChangeEvent voiceRadioTechnologyChangeEvent =
                TelUtil::createVoiceRadioTechnologyChangeEvent(
                slotId, telStub::RadioTechnology::RADIO_TECH_IS95A);
            notificationBuilder->addVoiceRadioTechnologyChangeEvent(slotId,
                voiceRadioTechnologyChangeEvent);
            std::shared_ptr<Notification> notification = notificationBuilder->build();
            notification->notify();
        } else if (mode == telStub::OperatingMode::ONLINE) {

            ::telStub::SignalStrength cachedSignalStrength = getCachedSS(slotId);
            telStub::SignalStrengthChangeEvent signalStrengthChangeEvent =
                TelUtil::createSignalStrengthEvent(slotId, cachedSignalStrength);
            notificationBuilder->addSignalStrengthChangeEvent(slotId, signalStrengthChangeEvent);

            ::telStub::RadioTechnology cachedServingRat = getCachedServingRat(slotId);
            telStub::VoiceRadioTechnologyChangeEvent voiceRadioTechnologyChangeEvent =
                TelUtil::createVoiceRadioTechnologyChangeEvent(
                slotId, cachedServingRat);
            notificationBuilder->addVoiceRadioTechnologyChangeEvent(slotId,
                voiceRadioTechnologyChangeEvent);

            telStub::ServiceStateChangeEvent serviceStateChangeEvent =
                TelUtil::createServiceStateEvent(slotId, telStub::ServiceState::IN_SERVICE);
            notificationBuilder->addServiceStateChangeEvent(slotId,serviceStateChangeEvent);

            telStub::VoiceServiceStateEvent voiceServiceStateEvent =
                TelUtil::createVoiceServiceStateEvent(slotId,
                telStub::VoiceServiceState::REG_HOME,
                telStub::VoiceServiceDenialCause::GENERAL,
                cachedServingRat);
            notificationBuilder->addVoiceServiceStateChangeEvent(slotId, voiceServiceStateEvent);
            std::shared_ptr<Notification> notification = notificationBuilder->build();
            notification->notify();
        }
    }
}

void OperatingModeTransitionManager::notifyOperatingMode(telStub::OperatingMode mode) {
    std::shared_ptr<TelephonyNotificationBuilder> notificationBuilder = getBuilder();
    telStub::OperatingModeEvent opModeEvent = TelUtil::createOperatingModeEvent(mode);
    notificationBuilder->addOperatingModeChangeEvent(opModeEvent);
    std::shared_ptr<Notification> notification = notificationBuilder->build();
    notification->notify();
}

std::shared_ptr<BaseState> OperatingModeTransitionManager::getPrevState() {
    return prevState_;
}

Notification::Notification() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();

}

Notification::~Notification() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

void Notification::addEvent(eventService::EventResponse eventResponse) {
    LOG(DEBUG, __FUNCTION__);
    {
        std::lock_guard<std::mutex> lck(notificationMutex_);
        events_.emplace_back(eventResponse);
    }
}

void Notification::notify() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [this]() {
        {
            std::lock_guard<std::mutex> lck(notificationMutex_);
            for(eventService::EventResponse event : events_) {
                this->triggerChangeEvent(event);
            }
            events_.clear();
        }
    }).share();
    taskQ_->add(f);
}

void Notification::triggerChangeEvent(eventService::EventResponse eventResponse) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(eventResponse);
}

TelephonyNotificationBuilder::TelephonyNotificationBuilder() {
    LOG(DEBUG, __FUNCTION__);
}

TelephonyNotificationBuilder::~TelephonyNotificationBuilder() {
    LOG(DEBUG, __FUNCTION__);
}

void TelephonyNotificationBuilder::addVoiceServiceStateChangeEvent(
    int phoneId, telStub::VoiceServiceStateEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = TelUtil::writeVoiceServiceStateToJsonFile(phoneId, event);
    if (error == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(event);
        {
            std::lock_guard<std::mutex> lck(notificationBuilderMutex_);
            events_.emplace_back(anyResponse);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Writing event to JSON failed");
    }
}

void TelephonyNotificationBuilder::addSignalStrengthChangeEvent(
    int phoneId, telStub::SignalStrengthChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = TelUtil::writeSignalStrengthToJsonFile(phoneId, event);
    if (error == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(event);
        {
            std::lock_guard<std::mutex> lck(notificationBuilderMutex_);
            events_.emplace_back(anyResponse);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Writing event to JSON failed");
    }
}

void TelephonyNotificationBuilder::addOperatingModeChangeEvent(
    telStub::OperatingModeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = TelUtil::writeOperatingModeToJsonFile(event);
    if (error == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(event);
        {
            std::lock_guard<std::mutex> lck(notificationBuilderMutex_);
            events_.emplace_back(anyResponse);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Writing event to JSON failed");
    }
}

void TelephonyNotificationBuilder::addServiceStateChangeEvent(int phoneId,
    telStub::ServiceStateChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = TelUtil::writeServiceStateToJsonFile(phoneId, event);
    if (error == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(event);
        {
            std::lock_guard<std::mutex> lck(notificationBuilderMutex_);
            events_.emplace_back(anyResponse);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Writing event to JSON failed");
    }
}

void TelephonyNotificationBuilder::addVoiceRadioTechnologyChangeEvent(int phoneId,
    telStub::VoiceRadioTechnologyChangeEvent &event) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = TelUtil::writeVoiceRadioTechnologyToJsonFile(phoneId, event);
    if (error == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(event);
        {
            std::lock_guard<std::mutex> lck(notificationBuilderMutex_);
            events_.emplace_back(anyResponse);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Writing event to JSON failed");
    }
}

std::shared_ptr<Notification> TelephonyNotificationBuilder::build() {
    LOG(DEBUG, __FUNCTION__);

    {
        std::lock_guard<std::mutex> lck(notificationBuilderMutex_);
        for(eventService::EventResponse event : events_) {
            notification_->addEvent(event);
        }
        events_.clear();
    }
    return notification_;
}

}  // End of namespace tel
}  // End of namespace telux
