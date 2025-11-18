/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "SuppServicesManagerStub.hpp"
#include "TelDefinesStub.hpp"

using namespace telux::tel;

SuppServicesManagerStub::SuppServicesManagerStub(SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    slotId_ = static_cast<int>(slotId);
}

telux::common::Status SuppServicesManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ISuppServicesListener>>();
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    suppServiceStub_ = CommonUtils::getGrpcStub<::telStub::SuppServicesService>();
    if (!suppServiceStub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate supplementary service");
        return telux::common::Status::FAILED;
    }
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if (!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    auto status = taskQ_->add(f);
    return status;
}

void SuppServicesManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(slotId_);

    grpc::Status reqstatus = suppServiceStub_->InitService(&context, request, &response);
    if (reqstatus.ok()) {
        telux::common::ServiceStatus cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        int cbDelay = static_cast<int>(response.delay());
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
        this->onServiceStatusChange(cbStatus);
        if (callback) {
            auto f1 = std::async(std::launch::async,
                [this, cbDelay, cbStatus, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(cbStatus);
            }).share();
            taskQ_->add(f1);
        }
    }
}

SuppServicesManagerStub::~SuppServicesManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void SuppServicesManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    suppServiceStub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus SuppServicesManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(slotId_);

    grpc::Status status = suppServiceStub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus serviceStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    return serviceStatus;
}

telux::common::Status SuppServicesManagerStub::registerListener(
    std::weak_ptr<ISuppServicesListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
    }
    return status;
}

telux::common::Status SuppServicesManagerStub::removeListener(
    std::weak_ptr<telux::tel::ISuppServicesListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->deRegisterListener(listener);
    }
    return status;
}

void SuppServicesManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ISuppServicesListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "SuppServices Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status SuppServicesManagerStub::setCallWaitingPref(
    SuppServicesStatus suppSvcStatus, SetSuppSvcPrefCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " SuppServices Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetCallWaitingPrefRequest request;
    ::telStub::SetCallWaitingPrefReply response;
    ClientContext context;
    request.set_slot_id(slotId_);
    request.set_supp_services_status(
        static_cast<telStub::SuppServicesStatus_Status>(suppSvcStatus));

    grpc::Status reqstatus = suppServiceStub_->SetCallWaitingPref(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    FailureCause failCause = FailureCause::UNAVAILABLE;
    failCause = static_cast<FailureCause>(response.failure_cause());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, failCause, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(error, failCause);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status SuppServicesManagerStub::requestCallWaitingPref(
    GetCallWaitingPrefExCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " SuppServices Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestCallWaitingPrefRequest request;
    ::telStub::RequestCallWaitingPrefReply response;
    ClientContext context;
    request.set_slot_id(slotId_);

    grpc::Status reqstatus = suppServiceStub_->RequestCallWaitingPref(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    FailureCause failureCause = FailureCause::UNAVAILABLE;
    SuppServicesStatus suppSvcStatus = SuppServicesStatus::UNKNOWN;
    suppSvcStatus = static_cast<SuppServicesStatus>(response.supp_services_status());
    failureCause = static_cast<FailureCause>(response.failure_cause());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, suppSvcStatus, failureCause, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(suppSvcStatus, failureCause, error);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status SuppServicesManagerStub::setForwardingPref(ForwardReq forwardReq,
    SetSuppSvcPrefCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " SuppServices Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetForwardingPrefRequest request;
    ::telStub::SetForwardingPrefReply response;
    ClientContext context;
    request.set_slot_id(slotId_);
    request.mutable_forward_req()->set_operation
        (static_cast<telStub::ForwardOperation_Operation>(forwardReq.operation));
    request.mutable_forward_req()->set_reason(static_cast<telStub::ForwardReason>
        (forwardReq.reason));
    int size = forwardReq.serviceClass.size();
    for (int j = 0; j < size ; j++)
    {
        if (forwardReq.serviceClass.test(j)) {
            request.mutable_forward_req()->add_service_class(
                static_cast<telStub::ServiceClassType_Type>(j));
        }
    }
    request.mutable_forward_req()->set_number(forwardReq.number);
    request.mutable_forward_req()->set_no_reply_timer(forwardReq.noReplyTimer);
    grpc::Status reqstatus = suppServiceStub_->SetForwardingPref(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    FailureCause failureCause = FailureCause::UNAVAILABLE;
    failureCause = static_cast<FailureCause>(response.failure_cause());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, failureCause, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(error, failureCause);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status SuppServicesManagerStub::requestForwardingPref(ServiceClass serviceClass,
    ForwardReason reason, GetForwardingPrefExCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " SuppServices Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestForwardingPrefRequest request;
    ::telStub::RequestForwardingPrefReply response;
    ClientContext context;
    request.set_slot_id(slotId_);
    int size = serviceClass.size();
    for (int j = 0; j < size ; j++)
    {
        if (serviceClass.test(j)) {
            request.add_service_class(
                static_cast<telStub::ServiceClassType_Type>(j));
        }
    }
    request.set_forward_reason(static_cast<telStub::ForwardReason>(reason));

    grpc::Status reqstatus = suppServiceStub_->RequestForwardingPref(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    std::vector<ForwardInfo> forwardInfoList = {};
    for (int i = 0; i < response.forward_info_size(); i++) {
        ForwardInfo info;
        info.status = static_cast<SuppServicesStatus>(response.mutable_forward_info(i)->status());
        ServiceClass serviceClassList;
        for (auto &sc : response.mutable_forward_info(i)->service_class()) {
            serviceClassList.set(static_cast<int>(sc));
        }
        info.serviceClass = serviceClassList;
        info.number = response.mutable_forward_info(i)->number();
        info.noReplyTimer = response.mutable_forward_info(i)->no_reply_timer();
        forwardInfoList.emplace_back(info);
    }

    FailureCause failureCause = FailureCause::UNAVAILABLE;
    failureCause = static_cast<FailureCause>(response.failure_cause());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, forwardInfoList, failureCause, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(forwardInfoList, failureCause, error);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status SuppServicesManagerStub::setOirPref(ServiceClass serviceClass,
    SuppServicesStatus suppSvcStatus, SetSuppSvcPrefCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " SuppServices Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetOirPrefRequest request;
    ::telStub::SetOirPrefReply response;
    ClientContext context;
    request.set_slot_id(slotId_);
    int size = serviceClass.size();
    for (int j = 0; j < size ; j++)
    {
        if (serviceClass.test(j)) {
            request.add_service_class(
                static_cast<telStub::ServiceClassType_Type>(j));
        }
    }
    request.set_supp_services_status(static_cast<telStub::SuppServicesStatus_Status>
        (suppSvcStatus));
    grpc::Status reqstatus = suppServiceStub_->SetOirPref(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    FailureCause failureCause = FailureCause::UNAVAILABLE;
    failureCause = static_cast<FailureCause>(response.failure_cause());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, failureCause, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(error, failureCause);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status SuppServicesManagerStub::requestOirPref(ServiceClass serviceClass,
    GetOirPrefCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " SuppServices Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestOirPrefRequest request;
    ::telStub::RequestOirPrefReply response;
    ClientContext context;
    request.set_slot_id(slotId_);
    int size = serviceClass.size();
    for (int j = 0; j < size ; j++) {
        if (serviceClass.test(j)) {
            request.add_service_class(static_cast<telStub::ServiceClassType_Type>(j));
        }
    }

    grpc::Status reqstatus = suppServiceStub_->RequestOirPref(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    FailureCause failureCause = FailureCause::UNAVAILABLE;
    SuppServicesStatus suppSvcStatus = SuppServicesStatus::UNKNOWN;
    SuppSvcProvisionStatus provisionStatus = SuppSvcProvisionStatus::UNKNOWN;
    suppSvcStatus = static_cast<SuppServicesStatus>(response.supp_services_status());
    provisionStatus = static_cast<SuppSvcProvisionStatus>(response.provision_status());
    failureCause = static_cast<FailureCause>(response.failure_cause());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, suppSvcStatus, provisionStatus, failureCause, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(suppSvcStatus, provisionStatus, failureCause, error);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

// deprecated API
telux::common::Status SuppServicesManagerStub::requestCallWaitingPref(
    GetCallWaitingPrefCb callback) {
    return telux::common::Status::NOTSUPPORTED;
}

// deprecated API
telux::common::Status SuppServicesManagerStub::requestForwardingPref(ServiceClass serviceClass,
    ForwardReason reason, GetForwardingPrefCb callback) {
    return telux::common::Status::NOTSUPPORTED;
}
