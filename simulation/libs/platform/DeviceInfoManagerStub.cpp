/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "common/Logger.hpp"
#include "DeviceInfoManagerStub.hpp"
#include "libs/common/CommonUtils.hpp"

#define RPC_FAIL_SUFFIX " RPC Request failed - "
#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define DEVICEINFO_MANAGER_FILTER "deviceinfo_manager"

namespace telux {
namespace platform {

using namespace telux::common;

DeviceInfoManagerStub::DeviceInfoManagerStub()
   : SimulationManagerStub<DeviceInfoManagerService>(std::string("IDeviceInfoManager"))
   , clientEventMgr_(ClientEventManager::getInstance()) {
    LOG(INFO, __FUNCTION__);
}

DeviceInfoManagerStub::~DeviceInfoManagerStub() {
    LOG(INFO, __FUNCTION__);
}


void DeviceInfoManagerStub::createListener() {
    LOG(DEBUG, __FUNCTION__);

    listenerMgr_ = std::make_shared<ListenerManager<IDeviceInfoListener>>();
}

void DeviceInfoManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void DeviceInfoManagerStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t DeviceInfoManagerStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);

    return cbDelay_;
}

Status DeviceInfoManagerStub::init() {
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

void DeviceInfoManagerStub::onEventUpdate(google::protobuf::Any event) {
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

void DeviceInfoManagerStub::handleSSREvent(google::protobuf::Any event) {
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
    onDmsServiceStatusChange(srvcStatus);
}

void DeviceInfoManagerStub::onDmsServiceStatusChange(
    telux::common::ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    if (srvcStatus == getServiceStatus()) {
        return;
    }

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: DeviceInfo Manager Service is UNAVAILABLE/FAILED");
        setServiceStatus(srvcStatus);
        return;
    }

    LOG(INFO, __FUNCTION__, ":: DeviceInfo Manager Service is AVAILABLE");
    auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_.add(f);
}

void DeviceInfoManagerStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::weak_ptr<IDeviceInfoListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    LOG(DEBUG, __FUNCTION__, ":: Notifying DeviceInfo manager service status: ",
            static_cast<int>(srvcStatus), " to listeners: ", applisteners.size());
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onServiceStatusChange(srvcStatus);
        }
    }
}

ServiceStatus DeviceInfoManagerStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status DeviceInfoManagerStub::registerDefaultIndications() {
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    Status status = Status::FAILED;
    status = clientEventMgr_.registerListener(shared_from_this(), DEVICEINFO_MANAGER_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }
    return status;
}

Status DeviceInfoManagerStub::initSyncComplete(
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

Status DeviceInfoManagerStub::registerListener(std::weak_ptr<IDeviceInfoListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return listenerMgr_->registerListener(listener);
}

Status DeviceInfoManagerStub::deregisterListener(std::weak_ptr<IDeviceInfoListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return listenerMgr_->deRegisterListener(listener);
}

Status DeviceInfoManagerStub::getPlatformVersion(PlatformVersion &pv) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, " DeviceInfoManagerStub is not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    ::platformStub::PlatformVersionInfo response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->GetPlatformVersion(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        LOG(ERROR, __FUNCTION__, "Get platform version failed");
    }

    status = static_cast<telux::common::Status>(response.reply().status());
    if (status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, "Get platform version successful");
        pv.modem = response.modem_details();
        pv.integratedApp = response.integrated_app();
        pv.externalApp = response.external_app();
        pv.meta = response.meta_details();
    }

    return status;
}


Status DeviceInfoManagerStub::getIMEI(std::string &imei) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, " DeviceInfoManagerStub is not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    ::platformStub::PlatformImeiInfo response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->GetIMEI(&context, request, &response);

    status = static_cast<telux::common::Status>(response.reply().status());

    if (!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        LOG(ERROR, __FUNCTION__, "Unable to get IMEI");
    }

    status = static_cast<telux::common::Status>(response.reply().status());
    if (status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " IMEI is ", imei);
        imei = response.imei_info();
    }

    return status;
}

}  // end of namespace platform
}  // end of namespace telux
