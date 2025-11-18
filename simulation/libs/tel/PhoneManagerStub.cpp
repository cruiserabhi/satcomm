/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <bits/stdc++.h>
#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include "PhoneManagerStub.hpp"
#include "common/Logger.hpp"

using namespace telux::tel;

PhoneManagerStub::PhoneManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

telux::common::Status PhoneManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IPhoneListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    phoneStub_ = CommonUtils::getGrpcStub<PhoneService>();
    if(!phoneStub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate phone service");
        return telux::common::Status::FAILED;
    }
    cardStub_ = CommonUtils::getGrpcStub<CardService>();
    if(!cardStub_) {
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

void PhoneManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(phoneManagerMutex_);
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

void PhoneManagerStub::initSync() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    noOfSlots_ = 1;

    grpc::Status reqstatus = phoneStub_->InitService(&context, request, &response);
    if (reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " PhoneService init successfully");
        telux::common::ServiceStatus cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay_,
            " cbStatus::", static_cast<int>(cbStatus));
        if (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(INFO, __FUNCTION__, " Phone subsystem is ready");
            ClientContext context;
            grpc::Status reqstatus = cardStub_->InitService(&context, request, &response);
            if (reqstatus.ok()) {
                LOG(DEBUG, __FUNCTION__, " CardService init successfully");
                cbStatus =  static_cast<telux::common::ServiceStatus>(response.service_status());
                if (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                    LOG(INFO, __FUNCTION__, " Card Manager subsystem is ready");
                    if (telux::common::DeviceConfig::isMultiSimSupported()) {
                        noOfSlots_ = 2;
                    }
                    for (int id = 1; id <= noOfSlots_; id++) {
                        phoneSlotIdsMap_.emplace(id, id);
                        phoneIds_.emplace_back(id);
                        std::shared_ptr<PhoneStub> phone = std::make_shared<PhoneStub>(id);
                        if (phone) {
                            phoneMap_.emplace(id, phone);
                        }
                        phone->init();
                    }

                    telux::common::Status status = requestOperatingMode(nullptr);
                    if (status != telux::common::Status::SUCCESS) {
                        cbStatus = telux::common::ServiceStatus::SERVICE_FAILED;
                    } else {
                        cbStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
                    }

                    // Wait till radio state and service state are received
                    // for all phones asynchronously
                    for(auto it = phoneMap_.begin(); it != phoneMap_.end(); ++it) {
                        auto phone = it->second;
                        bool isReady = phone->isReady();
                        while(!isReady) {
                            // blocking infinite wait for service to be ready
                            std::future<bool> f = phone->onReady();
                            isReady = f.get();
                        }
                    }
                }
            }
        }

        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
        bool isSubsystemReady = (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
            true : false;
        setSubsystemReady(isSubsystemReady);
        setServiceStatus(cbStatus);
    }
}

PhoneManagerStub::~PhoneManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    setSubsystemReady(false);
    if (phoneStub_) {
        phoneStub_ = nullptr;
    }
    if (cardStub_) {
        cardStub_ = nullptr;
    }
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    phoneIds_.clear();
    phoneMap_.clear();
    phoneSlotIdsMap_.clear();
}

void PhoneManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(phoneManagerMutex_);
    ready_ = status;
    cv_.notify_all();
}

telux::common::ServiceStatus PhoneManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status PhoneManagerStub::registerListener(std::weak_ptr<IPhoneListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_PHONE_FILTER};
        std::vector<std::weak_ptr<IPhoneListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 1) {
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(shared_from_this(), filters);
        } else {
            LOG(DEBUG, __FUNCTION__, " Not registering to client event manager already registered");
        }
    }
    return status;
}

telux::common::Status PhoneManagerStub::removeListener(std::weak_ptr<IPhoneListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IPhoneListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_PHONE_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status PhoneManagerStub::getPhoneIds(std::vector<int> &phoneIds) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    {
        std::lock_guard<std::mutex> lock(phoneManagerMutex_);
        phoneIds = phoneIds_;
    }
    const ::google::protobuf::Empty request;
    telStub::GetPhoneIdsReply response;
    ClientContext context;
    grpc::Status reqstatus = phoneStub_->GetPhoneIds(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed");
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

int PhoneManagerStub::getPhoneIdFromSlotId(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    if (slotId <= 0 || slotId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid SlotId");
        return INVALID_PHONE_ID;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return INVALID_PHONE_ID;
    }
    auto phId = phoneSlotIdsMap_.find(slotId);
    if (phId != phoneSlotIdsMap_.end()) {
       LOG(DEBUG, __FUNCTION__, " Found phone Id");
       return phoneSlotIdsMap_[slotId];
    }
    LOG(DEBUG, __FUNCTION__, " Invalid SlotId");
    return INVALID_PHONE_ID;
}

int PhoneManagerStub::getSlotIdFromPhoneId(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return INVALID_SLOT_ID;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return INVALID_SLOT_ID;
    }
    int slotId = INVALID_SLOT_ID;
    for (auto &it : phoneSlotIdsMap_) {
        if (it.second == phoneId) {
            slotId = it.first;
            break;
        }
   }
   LOG(DEBUG, __FUNCTION__, " slot id: ", slotId);
   return slotId;
}

std::shared_ptr<IPhone> PhoneManagerStub::getPhone(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return nullptr;
    }
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " PhoneManager is not ready");
        return nullptr;
    }
    std::vector<int> phoneIds;
    getPhoneIds(phoneIds);
    auto iter = std::find_if(std::begin(phoneIds), std::end(phoneIds),
                            [=](int id) { return id == phoneId; });
    if (iter != std::end(phoneIds)) {
        LOG(DEBUG, __FUNCTION__, " Found given phoneId: ", phoneId);
    } else {
        LOG(INFO, __FUNCTION__, " Invalid phoneId provided: ", phoneId);
        return nullptr;
    }

    auto ph = phoneMap_.find(phoneId);
    if (ph != phoneMap_.end()) {
        LOG(DEBUG, __FUNCTION__, " Found phoneId (", phoneId, ") in the phoneMap");
        return phoneMap_[phoneId];
    } else {
        LOG(DEBUG, __FUNCTION__, " Updating phoneMap");
        auto ph = std::make_shared<PhoneStub>(phoneId);
        phoneMap_.emplace(phoneId, ph);
        return ph;
    }
}

telux::common::Status PhoneManagerStub::requestCellularCapabilityInfo(
    std::shared_ptr<ICellularCapabilityCallback> callback) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    const ::google::protobuf::Empty request;
    telStub::CellularCapabilityInfoReply response;
    ClientContext context;
    telux::tel::CellularCapabilityInfo cellularCapabilityInfo;
    grpc::Status reqstatus = phoneStub_->GetCellularCapabilities(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(DEBUG, __FUNCTION__, " failed");
        return telux::common::Status::FAILED;
    }

    for (auto &tech : response.capability_info().voice_service_techs()) {
        VoiceServiceTechnology voiceServiceTechnology = static_cast<VoiceServiceTechnology>(tech);
        switch(voiceServiceTechnology) {
            case VoiceServiceTechnology::VOICE_TECH_GW_CSFB:
                cellularCapabilityInfo.voiceServiceTechs.set(static_cast<int>
                    (VoiceServiceTechnology::VOICE_TECH_GW_CSFB));
                break;
            case VoiceServiceTechnology::VOICE_TECH_1x_CSFB:
                cellularCapabilityInfo.voiceServiceTechs.set(static_cast<int>
                    (VoiceServiceTechnology::VOICE_TECH_1x_CSFB));
                break;
            case VoiceServiceTechnology::VOICE_TECH_VOLTE:
                cellularCapabilityInfo.voiceServiceTechs.set(static_cast<int>
                    (VoiceServiceTechnology::VOICE_TECH_VOLTE));
                break;
            default:
                LOG(ERROR, " Invalid voice technology");
                break;
        }
    }

    cellularCapabilityInfo.simCount = response.capability_info().sim_count();
    cellularCapabilityInfo.maxActiveSims = response.capability_info().max_active_sims();
    std::vector<SimRatCapability> simRatCapList;
    LOG(DEBUG, __FUNCTION__, " SIM RAT capabilities : ",
        response.capability_info().sim_rat_capabilities_size());
    for (int i = 0; i < response.capability_info().sim_rat_capabilities_size(); i++) {
        SimRatCapability simRatCap;
        simRatCap.slotId = response.capability_info().sim_rat_capabilities(i).phone_id();
        for (auto &rat : response.capability_info().sim_rat_capabilities(i).capabilities()) {
            RATCapability ratCap = static_cast<RATCapability>(rat);
            LOG(DEBUG, __FUNCTION__, " RAT Capability : ", static_cast<int>(ratCap));
            switch (ratCap) {
                case RATCapability::AMPS:
                     simRatCap.capabilities.set(static_cast<int>(RATCapability::AMPS));
                    break;
                case RATCapability::CDMA:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::CDMA));
                    break;
                case RATCapability::HDR:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::HDR));
                    break;
                case RATCapability::GSM:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::GSM));
                    break;
                case RATCapability::WCDMA:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::WCDMA));
                    break;
                case RATCapability::LTE:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::LTE));
                    break;
                case RATCapability::TDS:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::TDS));
                    break;
                case RATCapability::NR5G:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::NR5G));
                    break;
                case RATCapability::NR5GSA:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::NR5GSA));
                    break;
                case RATCapability::NB1_NTN:
                    simRatCap.capabilities.set(static_cast<int>(RATCapability::NB1_NTN));
                    break;
                default:
                    LOG(ERROR, __FUNCTION__, " Invalid radio capability");
                    break;
            }
        }
        simRatCapList.emplace_back(simRatCap);
        cellularCapabilityInfo.simRatCapabilities = simRatCapList;
    }

    std::vector<SimRatCapability> deviceRatCapList;
    LOG(DEBUG, __FUNCTION__, " Device RAT Capabilities : ",
        response.capability_info().device_rat_capability_size());
    for (int i = 0; i < response.capability_info().device_rat_capability_size(); i++) {
        SimRatCapability deviceRatCap;
        deviceRatCap.slotId =
            response.capability_info().device_rat_capability(i).phone_id();
        LOG(DEBUG, __FUNCTION__, " capabilities size:",
            response.capability_info().device_rat_capability(i).capabilities_size());
        for (auto &rat : response.capability_info().device_rat_capability(i).capabilities()) {
            RATCapability ratCap = static_cast<RATCapability>(rat);
            LOG(DEBUG, __FUNCTION__, " RAT Capability : ", static_cast<int>(ratCap));
            switch (ratCap) {
                case RATCapability::AMPS:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::AMPS));
                    break;
                case RATCapability::CDMA:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::CDMA));
                    break;
                case RATCapability::HDR:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::HDR));
                    break;
                case RATCapability::GSM:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::GSM));
                    break;
                case RATCapability::WCDMA:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::WCDMA));
                    break;
                case RATCapability::LTE:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::LTE));
                    break;
                case RATCapability::TDS:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::TDS));
                    break;
                case RATCapability::NR5G:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::NR5G));
                    break;
                case RATCapability::NR5GSA:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::NR5GSA));
                    break;
                case RATCapability::NB1_NTN:
                    deviceRatCap.capabilities.set(static_cast<int>(RATCapability::NB1_NTN));
                    break;
                default:
                    LOG(ERROR, " Invalid radio capability");
                    break;
            }
        }
        deviceRatCapList.emplace_back(deviceRatCap);
        cellularCapabilityInfo.deviceRatCapability = deviceRatCapList;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    LOG(DEBUG, __FUNCTION__, " Status is ", static_cast<int>(status));
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback, cellularCapabilityInfo]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback->cellularCapabilityResponse(cellularCapabilityInfo, error);
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneManagerStub::setOperatingMode(telux::tel::OperatingMode operatingMode,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telStub::SetOperatingModeRequest request;
    telStub::SetOperatingModeReply response;
    ClientContext context;

    request.set_operating_mode(static_cast<telStub::OperatingMode>(operatingMode));
    grpc::Status reqstatus = phoneStub_->SetOperatingMode(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    LOG(DEBUG, __FUNCTION__, " Status: ", static_cast<int>(status), " Errorcode: ",
        static_cast<int>(error));
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    if (error == telux::common::ErrorCode::SUCCESS) {
        updateRadioState(operatingMode);
    }
    if ((status == telux::common::Status::SUCCESS)&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneManagerStub::requestOperatingMode(
    std::shared_ptr<IOperatingModeCallback> callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    telStub::GetOperatingModeReply response;
    ClientContext context;
    grpc::Status reqstatus = phoneStub_->GetOperatingMode(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::tel::OperatingMode operatingMode = static_cast<telux::tel::OperatingMode>(
        response.operating_mode());
    // update radio state
    updateRadioState(operatingMode);
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, operatingMode, callback, error]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback->operatingModeResponse(operatingMode, error);
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

telux::common::Status PhoneManagerStub::resetWwan(telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    const ::google::protobuf::Empty request;
    ::telStub::ResetWwanReply response;
    ClientContext context;

    grpc::Status reqstatus = phoneStub_->ResetWwan(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto fut = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                }
            }).share();
        taskQ_->add(fut);
    }
    return status;
}

bool PhoneManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

std::future<bool> PhoneManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return PhoneManagerStub::waitForInitialization(); });
    return future;
}

bool PhoneManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(phoneManagerMutex_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

void PhoneManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::SignalStrengthChangeEvent>()) {
        ::telStub::SignalStrengthChangeEvent signalStrengthChangeEvent;
        event.UnpackTo(&signalStrengthChangeEvent);
        handleSignalStrengthChanged(signalStrengthChangeEvent);
    } else if(event.Is<::telStub::CellInfoListEvent>()) {
        ::telStub::CellInfoListEvent cellInfoListChangeEvent;
        event.UnpackTo(&cellInfoListChangeEvent);
        handleCellInfoListChanged(cellInfoListChangeEvent);
    } else if(event.Is<::telStub::VoiceServiceStateEvent>()) {
        ::telStub::VoiceServiceStateEvent voiceServiceStateChangeEvent;
        event.UnpackTo(&voiceServiceStateChangeEvent);
        handleVoiceServiceStateChanged(voiceServiceStateChangeEvent);
    } else if(event.Is<::telStub::OperatingModeEvent>()) {
        ::telStub::OperatingModeEvent operatingModeChangeEvent;
        event.UnpackTo(&operatingModeChangeEvent);
        handleOperatingModeChanged(operatingModeChangeEvent);
    } else if(event.Is<::telStub::ECallModeInfoChangeEvent>()) {
        ::telStub::ECallModeInfoChangeEvent ecallModeInfoChangeEvent;
        event.UnpackTo(&ecallModeInfoChangeEvent);
        handleECallOperatingModeChanged(ecallModeInfoChangeEvent);
    } else if(event.Is<::telStub::OperatorInfoEvent>()) {
        ::telStub::OperatorInfoEvent opertorInfoChangeEvent;
        event.UnpackTo(&opertorInfoChangeEvent);
        handleOperatorInfoChanged(opertorInfoChangeEvent);
    } else if(event.Is<::telStub::VoiceRadioTechnologyChangeEvent>()) {
        ::telStub::VoiceRadioTechnologyChangeEvent voiceRadioTechnologyChangeEvent;
        event.UnpackTo(&voiceRadioTechnologyChangeEvent);
        handleVoiceRadioTechChanged(voiceRadioTechnologyChangeEvent);
    } else if(event.Is<::telStub::ServiceStateChangeEvent>()) {
        ::telStub::ServiceStateChangeEvent serviceStateChangeEvent;
        event.UnpackTo(&serviceStateChangeEvent);
        handleServiceStateChanged(serviceStateChangeEvent);
    } else {
        LOG(DEBUG, __FUNCTION__, "No handling required for other events");
    }
}

void PhoneManagerStub::handleVoiceRadioTechChanged(
    ::telStub::VoiceRadioTechnologyChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    telux::tel::RadioTechnology rat =
        static_cast<telux::tel::RadioTechnology>(event.radio_technology());
    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onVoiceRadioTechnologyChanged(phoneId, rat);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void PhoneManagerStub::handleServiceStateChanged(::telStub::ServiceStateChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    telux::tel::ServiceState serviceState =
        static_cast<telux::tel::ServiceState>(event.service_state());
    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onServiceStateChanged(phoneId, serviceState);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}


void PhoneManagerStub::handleSignalStrengthChanged(::telStub::SignalStrengthChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    std::shared_ptr<SignalStrength> signalStrengthNotify = nullptr;
    std::shared_ptr<GsmSignalStrengthInfo> gsmSignalStrength =
         std::make_shared<GsmSignalStrengthInfo>(
             event.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
                gsm_signal_strength(),
             event.mutable_signal_strength()->mutable_gsm_signal_strength_info()->
                gsm_bit_error_rate(), INVALID_SIGNAL_STRENGTH_VALUE);
    std::shared_ptr<LteSignalStrengthInfo> lteSignalStrength
        = std::make_shared<LteSignalStrengthInfo>(
            event.mutable_signal_strength()->mutable_lte_signal_strength_info()->
                lte_signal_strength(),
            event.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_rsrp(),
            event.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_rsrq(),
            event.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_rssnr(),
            event.mutable_signal_strength()->mutable_lte_signal_strength_info()->lte_cqi(),
            event.mutable_signal_strength()->mutable_lte_signal_strength_info()->
                timing_advance());
    std::shared_ptr<WcdmaSignalStrengthInfo> wcdmaSignalStrength
        = std::make_shared<WcdmaSignalStrengthInfo>(
            event.mutable_signal_strength()->mutable_wcdma_signal_strength_info()->
                signal_strength(), event.mutable_signal_strength()->
                    mutable_wcdma_signal_strength_info()->bit_error_rate(),
                event.mutable_signal_strength()->
                    mutable_wcdma_signal_strength_info()->ecio(),
                event.mutable_signal_strength()->
                    mutable_wcdma_signal_strength_info()->rscp());
    std::shared_ptr<Nr5gSignalStrengthInfo> nr5gSignalStrength
        = std::make_shared<Nr5gSignalStrengthInfo>(
            event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->rsrp(),
            event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->rsrq(),
            event.mutable_signal_strength()->mutable_nr5g_signal_strength_info()->rssnr());
    std::shared_ptr<Nb1NtnSignalStrengthInfo> nb1NtnSignalStrength
        = std::make_shared<Nb1NtnSignalStrengthInfo>(
            event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->
                signal_strength(),
            event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->rsrp(),
            event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->rsrq(),
            event.mutable_signal_strength()->mutable_nb1_ntn_signal_strength_info()->rssnr());
    signalStrengthNotify
        = std::make_shared<SignalStrength>(lteSignalStrength, gsmSignalStrength,
            nullptr/*cdma deprecated*/, wcdmaSignalStrength, nullptr/*tdscdma deprecated*/,
            nr5gSignalStrength, nb1NtnSignalStrength);
    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onSignalStrengthChanged(phoneId, signalStrengthNotify);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void PhoneManagerStub::handleCellInfoListChanged(::telStub::CellInfoListEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    // update cellInfoList
    std::vector<std::shared_ptr<tel::CellInfo>> cellInfoList = {};
     for (int i = 0; i < event.cell_info_list_size(); i++) {
        CellType cellType = static_cast<CellType>
            (event.mutable_cell_info_list(i)->mutable_cell_type()->cell_type());
        int registered = event.mutable_cell_info_list(i)-> mutable_cell_type()->registered();
        LOG(DEBUG, __FUNCTION__, " Cell registered : ", registered);
        switch(cellType) {
            case CellType::GSM: {
                string gsmMcc = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->mcc();
                string gsmMnc = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->mnc();
                int gsmLac = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->lac();
                int gsmCid = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->cid();
                int gsmArfcn = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->arfcn();
                int gsmBsic = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_cell_identity()->bsic();
                int gsmSignalStrength = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_signal_strength_info()->gsm_signal_strength();
                int gsmBitErrorRate = event.mutable_cell_info_list(i)->mutable_gsm_cell_info()->
                    mutable_gsm_signal_strength_info()->gsm_bit_error_rate();
                GsmSignalStrengthInfo gsmCellSS(gsmSignalStrength, gsmBitErrorRate,
                                               INVALID_SIGNAL_STRENGTH_VALUE);
                GsmCellIdentity gsmCI(gsmMcc, gsmMnc, gsmLac, gsmCid, gsmArfcn, gsmBsic);
                auto gsmCellInfo = std::make_shared<GsmCellInfo>(registered, gsmCI, gsmCellSS);
                cellInfoList.emplace_back(gsmCellInfo);
                break;
            }
            case CellType::WCDMA: {
                string wcdmaMcc = event.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->mcc();
                string wcdmaMnc = event.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->mnc();
                int wcdmaLac = event.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->lac();
                int wcdmaCid = event.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->cid();
                int wcdmaArfcn = event.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->uarfcn();
                int wcdmaPsc = event.mutable_cell_info_list(i)->mutable_wcdma_cell_info()->
                    mutable_wcdma_cell_identity()->psc();
                int wcdmaSignalStrength = event.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()-> mutable_wcdma_signal_strength_info()->
                    signal_strength();
                int wcdmaBitErrorRate = event.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                    bit_error_rate();
                int wcdmaEcio = event.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                    ecio();
                int wcdmaRscp = event.mutable_cell_info_list(i)->
                    mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                    rscp();
                WcdmaSignalStrengthInfo wcdmaCellSS(wcdmaSignalStrength, wcdmaBitErrorRate,
                    wcdmaEcio, wcdmaRscp);
                WcdmaCellIdentity wcdmaCI(wcdmaMcc, wcdmaMnc, wcdmaLac, wcdmaCid, wcdmaPsc,
                    wcdmaArfcn);
                auto wcdmaCellInfo = std::make_shared<telux::tel::WcdmaCellInfo>
                    (registered, wcdmaCI, wcdmaCellSS);
                cellInfoList.emplace_back(wcdmaCellInfo);
                break;
            }
            case CellType::LTE: {
                string lteMcc = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->mcc();
                string lteMnc = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->mnc();
                int lteTac = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->tac();
                int lteCi = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->ci();
                int lteEarfcn = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->earfcn();
                int ltePci = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_cell_identity()->pci();
                int lteSignalStrength = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_signal_strength();
                int lteRsrp = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_rsrp();
                int lteRsrq = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_rsrq();
                int lteRssnr = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_rssnr();
                int lteCqi = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->lte_cqi();
                int lteTimingAdvance = event.mutable_cell_info_list(i)->mutable_lte_cell_info()->
                    mutable_lte_signal_strength_info()->timing_advance();
                LteSignalStrengthInfo lteCellSS(lteSignalStrength, lteRsrp, lteRsrq, lteRssnr,
                     lteCqi, lteTimingAdvance);
                LteCellIdentity lteCI(lteMcc, lteMnc, lteCi, ltePci, lteTac, lteEarfcn);
                auto lteCellInfo = std::make_shared<LteCellInfo>(registered, lteCI, lteCellSS);
                cellInfoList.emplace_back(lteCellInfo);
                break;
            }
            case CellType::NR5G: {
                string nr5gMcc = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->mcc();
                string nr5gMnc = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->mnc();
                int nr5gTac = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->tac();
                int nr5gCi = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->ci();
                int nr5gArfcn = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->arfcn();
                int nr5gPci = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_cell_identity()->pci();
                int nr5gRsrp = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_signal_strength_info()->rsrp();
                int nr5gRsrq = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_signal_strength_info()->rsrq();
                int nr5gRssnr = event.mutable_cell_info_list(i)->mutable_nr5g_cell_info()->
                    mutable_nr5g_signal_strength_info()->rssnr();
                Nr5gSignalStrengthInfo nr5gCellSS(nr5gRsrp, nr5gRsrq, nr5gRssnr);
                Nr5gCellIdentity nr5gCI(nr5gMcc, nr5gMnc, nr5gCi, nr5gPci, nr5gTac, nr5gArfcn);
                auto nr5gCellInfo = std::make_shared<Nr5gCellInfo>(registered, nr5gCI,
                    nr5gCellSS);
                cellInfoList.emplace_back(nr5gCellInfo);
                break;
            }
            case CellType::NB1_NTN: {
                string nb1NtnMcc = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->mcc();
                string nb1NtnMnc = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->mnc();
                int nb1NtnTac = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->tac();
                int nb1NtnCi = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->ci();
                int nb1NtnEarfcn = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_cell_identity()->earfcn();
                int nb1NtnSignalStrength = event.mutable_cell_info_list(i)->
                    mutable_nb1_ntn_cell_info()->mutable_nb1_ntn_signal_strength_info()->
                        signal_strength();
                int nb1NtnRsrp = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_signal_strength_info()->rsrp();
                int nb1NtnRsrq = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_signal_strength_info()->rsrq();
                int nb1NtnRssnr = event.mutable_cell_info_list(i)->mutable_nb1_ntn_cell_info()->
                    mutable_nb1_ntn_signal_strength_info()->rssnr();
                Nb1NtnSignalStrengthInfo nb1NtnCellSS(nb1NtnSignalStrength, nb1NtnRsrp, nb1NtnRsrq,
                     nb1NtnRssnr);
                Nb1NtnCellIdentity nb1NtnCI(nb1NtnMcc, nb1NtnMnc, nb1NtnCi, nb1NtnTac,
                     nb1NtnEarfcn);
                auto nb1NtnCellInfo = std::make_shared<Nb1NtnCellInfo>(registered, nb1NtnCI,
                    nb1NtnCellSS);
                cellInfoList.emplace_back(nb1NtnCellInfo);
                break;
            }
            case CellType::CDMA:
            case CellType::TDSCDMA:
            default:
                LOG(ERROR, __FUNCTION__, " Invalid or deprecated cell type");
                break;
        }
    }
    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onCellInfoListChanged(phoneId, cellInfoList);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void PhoneManagerStub::handleVoiceServiceStateChanged(::telStub::VoiceServiceStateEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    std::shared_ptr<VoiceServiceInfo> vocSrvInfo = nullptr;
    VoiceServiceState voiceServiceState =
        static_cast<telux::tel::VoiceServiceState>(
            event.voice_service_state_info().voice_service_state());
    VoiceServiceDenialCause denialCause =
        static_cast<telux::tel::VoiceServiceDenialCause>(
            event.voice_service_state_info().voice_service_denial_cause());
    RadioTechnology radioTech =
        static_cast<telux::tel::RadioTechnology>(
            event.voice_service_state_info().radio_technology());
    vocSrvInfo = std::make_shared<VoiceServiceInfo>(voiceServiceState, denialCause, radioTech);
    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onVoiceServiceStateChanged(phoneId, vocSrvInfo);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void PhoneManagerStub::handleOperatingModeChanged(::telStub::OperatingModeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    telux::tel::OperatingMode opMode =
        static_cast<telux::tel::OperatingMode>(event.operating_mode());
    updateRadioState(opMode);
    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onOperatingModeChanged(opMode);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void PhoneManagerStub::handleECallOperatingModeChanged(::telStub::ECallModeInfoChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    telux::tel::ECallMode ecallMode = static_cast<telux::tel::ECallMode>(event.ecall_mode());
    telux::tel::ECallModeReason ecallModeReason =
         static_cast<telux::tel::ECallModeReason>(event.ecall_mode_reason());
    telux::tel::ECallModeInfo info;
    info.mode = ecallMode;
    info.reason = ecallModeReason;
    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onECallOperatingModeChange(phoneId, info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void PhoneManagerStub::handleOperatorInfoChanged(::telStub::OperatorInfoEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    telux::common::BoolValue isHome = BoolValue::STATE_FALSE;
    telux::tel::PlmnInfo plmnInfo;
    plmnInfo.longName = event.mutable_plmn_info()->long_name();
    plmnInfo.shortName = event.mutable_plmn_info()->short_name();
    plmnInfo.plmn = event.mutable_plmn_info()->plmn();

    if (event.mutable_plmn_info()->ishome()) {
        isHome = BoolValue::STATE_TRUE;
    }
    plmnInfo.isHome = isHome;

    std::vector<std::weak_ptr<IPhoneListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onOperatorInfoChange(phoneId, plmnInfo);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void PhoneManagerStub::updateRadioState(OperatingMode optMode) {
   LOG(DEBUG, __FUNCTION__, " optMode: ", static_cast<int>(optMode));
   for (auto &it : phoneMap_) {
       auto phone = it.second;
       switch(optMode) {
           case OperatingMode::ONLINE:
               phone->updateRadioState(RadioState::RADIO_STATE_ON);
               break;
           case OperatingMode::AIRPLANE:
           case OperatingMode::RESETTING:
           case OperatingMode::SHUTTING_DOWN:
           case OperatingMode::PERSISTENT_LOW_POWER:
           case OperatingMode::OFFLINE:
               phone->updateRadioState(RadioState::RADIO_STATE_OFF);
               break;
           case OperatingMode::FACTORY_TEST:
           default:
               phone->updateRadioState(RadioState::RADIO_STATE_UNAVAILABLE);
               break;
       }
   }
}
