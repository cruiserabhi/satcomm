/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ServingSystemManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include <thread>
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {

ServingSystemManagerStub::ServingSystemManagerStub (SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IServingSystemListener>>();
    slotId_ = slotId;
}

ServingSystemManagerStub::~ServingSystemManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status ServingSystemManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void ServingSystemManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::DataServingSystemManager>();

    ::dataStub::SlotInfo request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay = DEFAULT_DELAY;

    do {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " InitService request failed");
            break;
        }

        cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());

        this->onServiceStatusChange(cbStatus);
        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
    } while (0);

    setSubSystemStatus(cbStatus);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void ServingSystemManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void ServingSystemManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void ServingSystemManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus ServingSystemManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

DrbStatus ServingSystemManagerStub::getDrbStatus() {
    LOG(DEBUG, __FUNCTION__);

    DrbStatus status = DrbStatus::UNKNOWN;

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data ServingSystem manager not ready");
        return status;
    }

    ::dataStub::GetDrbStatusRequest request;
    ::dataStub::GetDrbStatusReply response;
    ClientContext context;

    request.mutable_drb_status()->set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->GetDrbStatus(&context, request, &response);

    status = static_cast<telux::data::DrbStatus>(response.mutable_drb_status()->drb_status());

    return status;
}

telux::common::Status ServingSystemManagerStub::requestServiceStatus(
    RequestServiceStatusResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    telux::data::ServiceStatus serviceStatus = {telux::data::DataServiceState::UNKNOWN,
        telux::data::NetworkRat::UNKNOWN};

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data ServingSystem manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::ServingStatusRequest request;
    ::dataStub::ServiceStatusReply response;
    ClientContext context;

    request.mutable_serving_status()->set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->RequestServiceStatus(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    serviceStatus.serviceState = static_cast<telux::data::DataServiceState>(
        response.data_service_state().data_service_state());
    serviceStatus.networkRat = static_cast<telux::data::NetworkRat>(
        response.network_rat().network_rat());;

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " requestServiceStatus failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, error, serviceStatus, delay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(serviceStatus, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status ServingSystemManagerStub::requestRoamingStatus(
    RequestRoamingStatusResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    telux::data::RoamingStatus roamingStatus = {false, telux::data::RoamingType::UNKNOWN};

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data ServingSystem manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RoamingStatusRequest request;
    ::dataStub::RomingStatusReply response;
    ClientContext context;

    request.mutable_roaming_status()->set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->RequestRoamingStatus(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    roamingStatus.isRoaming = response.is_roaming();
    roamingStatus.type = static_cast<telux::data::RoamingType>(
        response.roaming_type().roaming_type());;

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " requestRoamingStatus failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, error, roamingStatus, delay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(roamingStatus, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status ServingSystemManagerStub::makeDormant(
    telux::common::ResponseCallback callback = nullptr) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status ServingSystemManagerStub::requestNrIconType(
    RequestNrIconTypeResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data ServingSystem manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;
    NrIconType type = telux::data::NrIconType::NONE;

    ::dataStub::NrIconTypeRequest request;
    ::dataStub::NrIconTypeReply response;
    ClientContext context;

    request.mutable_nr_icon_status()->set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->RequestNrIconType(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    type = static_cast<telux::data::NrIconType>(
        response.nr_icon_type().nr_icon_type());;

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " requestRoamingStatus failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, error, type, delay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(type, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

SlotId ServingSystemManagerStub::getSlotId() {
    LOG(DEBUG, __FUNCTION__);
    return slotId_;
}

telux::common::Status ServingSystemManagerStub::registerListener(
    std::weak_ptr<IServingSystemListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status ServingSystemManagerStub::deregisterListener(
    std::weak_ptr<IServingSystemListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void ServingSystemManagerStub::onRoamingStatusChanged(RoamingStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IServingSystemListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Serving System Manager: invoking onRoamingStatusChanged");
                sp->onRoamingStatusChanged(status);
            }
        }
    }
}

void ServingSystemManagerStub::onNrIconTypeChanged(NrIconType type) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IServingSystemListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Serving System Manager: invoking onNrIconTypeChangeInd");
                sp->onNrIconTypeChanged(type);
            }
        }
    }
}

void ServingSystemManagerStub::onServiceStateChangeInd(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IServingSystemListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Serving System Manager: invoking onServiceStatusChange");
                sp->onServiceStateChanged(status);
            }
        }
    }
}

void ServingSystemManagerStub::onDrbStatusChanged(DrbStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IServingSystemListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Serving System Manager: invoking onDrbStatusChanged");
                sp->onDrbStatusChanged(status);
            }
        }
    }
}

} // end of namespace data
} // end of namespace telux
