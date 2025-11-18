/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"
#include "AntennaManagerStub.hpp"

#define DEFAULT_ANTENNA_INDEX 0
#define RPC_FAIL_SUFFIX " RPC Request failed - "
#define ANTENNA_MANAGER_FILTER "antenna_manager"

namespace telux {
namespace platform {
namespace hardware {

using namespace telux::common;

AntennaManagerStub::AntennaManagerStub()
   : SimulationManagerStub<AntennaManagerService>(std::string("IAntennaManager"))
   , isAntSwitchEnabled_(false)
   , antIndex_(DEFAULT_ANTENNA_INDEX)
   , clientEventMgr_(ClientEventManager::getInstance()) {
    LOG(INFO, __FUNCTION__);
}

AntennaManagerStub::~AntennaManagerStub() {
    LOG(INFO, __FUNCTION__);
}

void AntennaManagerStub::createListener() {
    LOG(DEBUG, __FUNCTION__);

    listenerMgr_ = std::make_shared<ListenerManager<IAntennaListener>>();
}

void AntennaManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void AntennaManagerStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t AntennaManagerStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);

    return cbDelay_;
}

Status AntennaManagerStub::init() {
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

void AntennaManagerStub::onEventUpdate(google::protobuf::Any event) {
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

void AntennaManagerStub::handleSSREvent(google::protobuf::Any event) {
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
    onAntennaManagerServiceStatusChange(srvcStatus);
}

void AntennaManagerStub::onAntennaManagerServiceStatusChange(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__, ":: Service Status: ", static_cast<int>(srvcStatus));

    if (srvcStatus == getServiceStatus()) {
        return;
    }

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: Antenna Manager Service is UNAVAILABLE/FAILED");
        setServiceStatus(srvcStatus);
        return;
    }

    LOG(INFO, __FUNCTION__, ":: Antenna Manager Service is AVAILABLE");
    auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_.add(f);
}

void AntennaManagerStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE) {
        // set antIndex back to init value
        std::lock_guard<std::mutex> lock(mutex_);
        antIndex_ = DEFAULT_ANTENNA_INDEX;
    }
    std::vector<std::weak_ptr<IAntennaListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    LOG(DEBUG, __FUNCTION__, ":: Notifying antenna manager service status: ",
            static_cast<int>(srvcStatus), " to listeners: ", applisteners.size());
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onServiceStatusChange(srvcStatus);
        }
    }
}

ServiceStatus AntennaManagerStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status AntennaManagerStub::registerDefaultIndications() {
    Status status = Status::FAILED;
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    status = clientEventMgr_.registerListener(shared_from_this(), ANTENNA_MANAGER_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }
    return status;
}

Status AntennaManagerStub::initSyncComplete(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    registerDefaultIndications();

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE)
    {
        return Status::FAILED;
    }

    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, ":: Invalid instance ");
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

Status AntennaManagerStub::registerListener(std::weak_ptr<IAntennaListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
    }

    return status;
}

Status AntennaManagerStub::deregisterListener(std::weak_ptr<IAntennaListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->deRegisterListener(listener);
    }

    return status;
}


void AntennaManagerStub::onActiveAntennaChange(int antIndex) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::weak_ptr<IAntennaListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onActiveAntennaChange(antIndex);
        }
    }
}

void AntennaManagerStub::onSetAntConfigResponse(int antIndex, ResponseCallback callback,
    ErrorCode errorCode){
    LOG(DEBUG, __FUNCTION__);

    do {
        // The antenna status before executing this set request
        bool isAntSwitchPreEnabled = false;
        isAntSwitchPreEnabled = isAntSwitchEnabled_;
        if (!isAntSwitchPreEnabled) {
            isAntSwitchEnabled_ = true;
        }

        if (isAntSwitchPreEnabled) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                antIndex_ = antIndex;
            }

            if (callback == nullptr) {
                LOG(ERROR, __FUNCTION__, " Callback is nullptr");
                break;
            }
            callback(errorCode);
            auto f = std::async(std::launch::async, [this, antIndex]() {
                onActiveAntennaChange(antIndex);
            }).share();
            taskQ_.add(f);
        } else {
            setActiveAntenna(antIndex, callback);
        }
    } while (0);
}

telux::common::Status AntennaManagerStub::setActiveAntenna(int antIndex,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, "AntennaManagerStub::", __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, " AntennaManagerStub is not ready");
        return telux::common::Status::NOTREADY;
    }

    Status status = Status::FAILED;
    ErrorCode errorCode = ErrorCode::GENERIC_FAILURE;
    ::platformStub::DefaultReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->SetActiveAntenna(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        return status;
    }

    status = static_cast<telux::common::Status>(response.status());
    errorCode = static_cast<telux::common::ErrorCode>(response.error());
    LOG(DEBUG, __FUNCTION__, " set ANT config req status: ", static_cast<int>(status));
    auto f = std::async(std::launch::async, [this, antIndex, callback, errorCode]() {
        onSetAntConfigResponse(antIndex, callback, errorCode);
    }).share();
    taskQ_.add(f);

    return status;
}

void AntennaManagerStub::onGetAntConfigResponse(GetActiveAntCb callback, ErrorCode errorCode) {
    LOG(DEBUG, __FUNCTION__);

    int antIndex = DEFAULT_ANTENNA_INDEX;
    if (callback == nullptr) {
        LOG(ERROR, __FUNCTION__, " Callback is nullptr");
        return;
    }

    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " could not get antIndex from response");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        antIndex = antIndex_;
    }

    callback(antIndex, errorCode);
}

telux::common::Status AntennaManagerStub::getActiveAntenna(GetActiveAntCb callback) {
    LOG(DEBUG, "AntennaManagerStub::", __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, " AntennaManagerStub is not ready");
        return telux::common::Status::NOTREADY;
    }

    if (callback == nullptr) {
        LOG(DEBUG, __FUNCTION__, " Callback is null");
    }

    Status status = Status::FAILED;
    ErrorCode errorCode = ErrorCode::GENERIC_FAILURE;
    ::platformStub::DefaultReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->GetActiveAntenna(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        return status;
    }

    status = static_cast<telux::common::Status>(response.status());
    errorCode = static_cast<telux::common::ErrorCode>(response.error());
    LOG(DEBUG, __FUNCTION__, " get ANT config req status: ", static_cast<int>(status));
    auto f = std::async(std::launch::async, [this, callback, errorCode]() {
        onGetAntConfigResponse(callback, errorCode);
    }).share();
    taskQ_.add(f);

    return status;
}

}  // end of namespace hardware
}  // end of namespace platform
}  // end of namespace telux
