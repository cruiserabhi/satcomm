/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "ImsServingSystemManagerStub.hpp"
#include "TelDefinesStub.hpp"

using namespace telux::common;
using namespace telux::tel;

ImsServingSystemManagerStub::ImsServingSystemManagerStub(SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    phoneId_ = static_cast<int>(slotId);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

void ImsServingSystemManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(mtx_);
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

telux::common::Status ImsServingSystemManagerStub::init(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IImsServingSystemListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<::telStub::ImsServingSystem>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ims serving system service");
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

void ImsServingSystemManagerStub::initSync() {
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " InitService request failed");
    } else {
        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
    }
    LOG(DEBUG, __FUNCTION__, " callback delay ", cbDelay_,
        " callback status ", static_cast<int>(cbStatus));
    setServiceStatus(cbStatus);
}

ImsServingSystemManagerStub::~ImsServingSystemManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void ImsServingSystemManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus ImsServingSystemManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status ImsServingSystemManagerStub::registerListener(
    std::weak_ptr<IImsServingSystemListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_IMS_SERVING_FILTER};
        std::vector<std::weak_ptr<IImsServingSystemListener>> applisteners;
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

telux::common::Status ImsServingSystemManagerStub::deregisterListener(
    std::weak_ptr<telux::tel::IImsServingSystemListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IImsServingSystemListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_IMS_SERVING_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status
    ImsServingSystemManagerStub::requestRegistrationInfo(ImsRegistrationInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims ServingSystem Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestRegistrationInfoRequest request;
    ::telStub::RequestRegistrationInfoReply response;
    ClientContext context;
    request.set_slot_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRegistrationInfo(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ImsRegistrationInfo info;
    info.imsRegStatus = static_cast<telux::tel::RegistrationStatus>(response.ims_reg_status());
    info.rat = static_cast<telux::tel::RadioTechnology>(response.rat());
    info.errorCode = response.error_code();
    info.errorString = (response.error_string());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, info, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(info, error);
            }
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ImsServingSystemManagerStub::requestServiceInfo(ImsServiceInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims ServingSystem Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestServiceInfoRequest request;
    ::telStub::RequestServiceInfoReply response;
    ClientContext context;
    request.set_slot_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestServiceInfo(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ImsServiceInfo info;
    info.sms = static_cast<telux::tel::CellularServiceStatus>(response.sms());
    info.voice = static_cast<telux::tel::CellularServiceStatus>(response.voice());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, info, error, callback, delay]() {
                if (callback) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(info, error);
                }
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ImsServingSystemManagerStub::requestPdpStatus(ImsPdpStatusInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims ServingSystem Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestPdpStatusRequest request;
    ::telStub::RequestPdpStatusReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestPdpStatus(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ImsPdpStatusInfo info;
    telux::common::DataCallEndReason failureReason;
    info.isPdpConnected = response.is_pdp_connected();
    info.apnName = response.apn_name();
    info.failureCode = static_cast<telux::tel::PdpFailureCode>(response.failure_code());
    failureReason.type = static_cast<telux::common::EndReasonType>(response.failure_reason());
    info.failureReason = failureReason;

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, info, error, callback, delay]() {
                if (callback) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(info, error);
                }
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

void ImsServingSystemManagerStub::handleImsRegStatusChanged
    (::telStub::ImsRegStatusChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    ImsRegistrationInfo info = {};
    info.imsRegStatus = static_cast<telux::tel::RegistrationStatus>(event.ims_reg_status());
    info.rat = static_cast<telux::tel::RadioTechnology>(event.rat());
    info.errorCode = event.error_code();
    info.errorString = event.error_string();
    std::vector<std::weak_ptr<IImsServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onImsRegStatusChange(info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ImsServingSystemManagerStub::handleImsServiceInfoChanged
    (::telStub::ImsServiceInfoChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    ImsServiceInfo info = {};
    info.sms = static_cast<telux::tel::CellularServiceStatus>(event.sms());
    info.voice = static_cast<telux::tel::CellularServiceStatus>(event.voice());
    std::vector<std::weak_ptr<IImsServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onImsServiceInfoChange(info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ImsServingSystemManagerStub::handleImsPdpStatusInfoChanged
    (::telStub::ImsPdpStatusInfoChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    ImsPdpStatusInfo info = {};
    telux::common::DataCallEndReason failureReason = {};
    info.isPdpConnected = event.is_pdp_connected();
    info.apnName = event.apn_name();
    info.failureCode = static_cast<telux::tel::PdpFailureCode>(event.failure_code());
    failureReason.type = static_cast<telux::common::EndReasonType>(event.failure_reason());
    info.failureReason = failureReason;
    std::vector<std::weak_ptr<IImsServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onImsPdpStatusInfoChange(info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ImsServingSystemManagerStub::onEventUpdate(google::protobuf::Any event) {
    if (event.Is<::telStub::ImsRegStatusChangeEvent>()) {
        ::telStub::ImsRegStatusChangeEvent imsRegStatusChangeEvent;
        event.UnpackTo(&imsRegStatusChangeEvent);
        handleImsRegStatusChanged(imsRegStatusChangeEvent);
    } else if(event.Is<::telStub::ImsServiceInfoChangeEvent>()) {
        ::telStub::ImsServiceInfoChangeEvent imsServiceInfoChangeEvent;
        event.UnpackTo(&imsServiceInfoChangeEvent);
        handleImsServiceInfoChanged(imsServiceInfoChangeEvent);
    } else if(event.Is<::telStub::ImsPdpStatusInfoChangeEvent>()) {
        ::telStub::ImsPdpStatusInfoChangeEvent imsPdpStatusInfoChangeEvent;
        event.UnpackTo(&imsPdpStatusInfoChangeEvent);
        handleImsPdpStatusInfoChanged(imsPdpStatusInfoChangeEvent);
    }
}


