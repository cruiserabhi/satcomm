/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataControlManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DELAY 100
#define DEFAULT_DELIMITER " "
#define DATA_CONTROL_SSR_FILTER "data_control_ssr"

namespace telux {
namespace data {

using telux::common::Status;

DataControlManagerStub::DataControlManagerStub()
    : SimulationManagerStub<DataControlManager>(std::string("IDataControlManagerStub"))
    , clientEventMgr_(ClientEventManager::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

DataControlManagerStub::~DataControlManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void DataControlManagerStub::createListener() {
    LOG(DEBUG, __FUNCTION__);

    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IDataControlListener>>();
}

void DataControlManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void DataControlManagerStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t DataControlManagerStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
    return cbDelay_;
}

Status DataControlManagerStub::init() {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::SUCCESS;

    try {
        createListener();
    } catch (std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ": Invalid listener instance");
        return Status::FAILED;
    }
    status = registerDefaultIndications();
    return status;
}

Status DataControlManagerStub::registerDefaultIndications() {
    Status status = Status::FAILED;
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    status = clientEventMgr_.registerListener(shared_from_this(), DATA_CONTROL_SSR_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }
    return Status::SUCCESS;
}

void DataControlManagerStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        // Deregister optional indications on SSR (if any)
    }
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDataControlListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Data Control Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(srvcStatus);
            }
        }
    }
}

ServiceStatus DataControlManagerStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status DataControlManagerStub::initSyncComplete(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::FAILED;
    status = registerDefaultIndications();

    return status;
}

void DataControlManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    // Execute all events in separate thread
    auto f = std::async(std::launch::deferred, [this, event]() {
            if (event.Is<commonStub::GetServiceStatusReply>()) {
                handleSSREvent(event);
            } else {
                LOG(ERROR, __FUNCTION__, ":: Invalid event");
    }}).share();

    taskQ_.add(f);
}

void DataControlManagerStub::handleSSREvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    commonStub::GetServiceStatusReply ssrResp;
    event.UnpackTo(&ssrResp);

    ServiceStatus srvcStatus = ServiceStatus::SERVICE_FAILED;

    if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_AVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_AVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_UNAVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_FAILED) {
        srvcStatus = ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(ERROR, __FUNCTION__, ":: INVALID SSR event");
        return;
    }
    setServiceReady(srvcStatus);
    onServiceStatusChange(srvcStatus);
}

void DataControlManagerStub::onServiceStatusChange(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__, ":: Service Status: ", static_cast<int>(srvcStatus));

    if (srvcStatus == getServiceStatus()) {
        return;
    }
    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: Qms Service is UNAVAILABLE");
        setServiceStatus(srvcStatus);
    } else {
        LOG(INFO, __FUNCTION__, ":: Qms Service is AVAILABLE");
        auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
        taskQ_.add(f);
    }
}

telux::common::Status DataControlManagerStub::registerListener(
    std::weak_ptr<IDataControlListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status DataControlManagerStub::deregisterListener(
    std::weak_ptr<IDataControlListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

telux::common::ErrorCode DataControlManagerStub::setDataStallParams(
    const SlotId &slotId, const DataStallParams &params) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " DataControl manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::dataStub::SetDataStallParamsRequest request;
    ::dataStub::SetDataStallParamsReply response;
    ClientContext context;

    request.set_slot_id(static_cast<int>(slotId));

    grpc::Status reqStatus = stub_->SetDataStallParams(&context, request, &response);

    telux::common::ErrorCode error  =
        static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, ", errorcode: ", static_cast<int>(reqStatus.error_code()));
            LOG(ERROR, __FUNCTION__, " setDataStallParams request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }

    return error;
}

} // end of namespace data
} // end of namespace telux
