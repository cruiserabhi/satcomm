/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"

#include "SubsystemManagerStub.hpp"
#define DEVICEINFO_MANAGER_FILTER "deviceinfo_manager"
#define SUBSYSTEM_MANAGER_FILTER "subsystem_manager"

namespace telux {
namespace platform {

using namespace telux::common;

SubsystemManagerStub::SubsystemManagerStub()
   : SimulationManagerStub<DeviceInfoManagerService>(std::string("ISubsystemManager"))
   , clientEventMgr_(ClientEventManager::getInstance()) {
    LOG(INFO, __FUNCTION__);
}

SubsystemManagerStub::~SubsystemManagerStub() {
    LOG(INFO, __FUNCTION__);
}

void SubsystemManagerStub::createListener() {
    LOG(DEBUG, __FUNCTION__);

    q6ListenerMgr_ = std::make_shared<ListenerManager<ISubsystemListener>>();
    a7ListenerMgr_ = std::make_shared<ListenerManager<ISubsystemListener>>();
}

void SubsystemManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void SubsystemManagerStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t SubsystemManagerStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);

    return cbDelay_;
}

void SubsystemManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    // Execute all events in separate thread
    auto f = std::async(std::launch::deferred, [this, event]() {
            if (event.Is<commonStub::GetServiceStatusReply>()) {
                handleSSREvent(event);
            } else if (event.Is<::platformStub::SubsystemStatusreply>()) {
                handleSubsystemEvent(event);
            } else {
                LOG(ERROR, __FUNCTION__, ":: Invalid event");
    }}).share();

    taskQ_.add(f);
}

void SubsystemManagerStub::registerCombination(Subsystem subsystem, ProcType procType) {
    std::lock_guard<std::mutex> lock(mutex_);
    supportedCombinations_.insert({subsystem, procType});
}

bool SubsystemManagerStub::isSupported(Subsystem subsystem, ProcType procType) {
    std::lock_guard<std::mutex> lock(mutex_);
    return supportedCombinations_.find({subsystem, procType}) != supportedCombinations_.end();
}

void SubsystemManagerStub::resetCombination() {
    std::lock_guard<std::mutex> lock(mutex_);
    supportedCombinations_.clear();
}

void SubsystemManagerStub::handleSubsystemEvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    ::platformStub::SubsystemStatusreply subsystemResp;
    event.UnpackTo(&subsystemResp);
    OperationalStatus opStatus = OperationalStatus::UNAVAILABLE;

    if (subsystemResp.status() == commonStub::OperationalStatus::OPERATIONAL) {
        opStatus = OperationalStatus::OPERATIONAL;
    } else if (subsystemResp.status() == commonStub::OperationalStatus::NONOPERATIONAL) {
        opStatus = OperationalStatus::UNAVAILABLE;
    } else {
        LOG(ERROR, __FUNCTION__, ":: INVALID event");
        return;
    }

    telux::common::Subsystem subsystem = static_cast<Subsystem>(subsystemResp.subsystem());
    telux::common::ProcType procType = static_cast<ProcType>(subsystemResp.proc_type());

    if (!isSupported(subsystem, procType)) {
        LOG(DEBUG, __FUNCTION__, " ", static_cast<int>(subsystem), " and ",
        static_cast<int>(procType), " combination is not supported/registered. ");
        return;
    }

    sendNewStatusToClients(opStatus, subsystem, procType);
}

/*
 * Find all the registered clients and pass them the latest state.
 */
void SubsystemManagerStub::sendNewStatusToClients(telux::common::OperationalStatus newOpStatus,
    Subsystem subsystem, ProcType procType) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::SubsystemInfo subsystemInfo{};

    subsystemInfo.subsystems = subsystem;
    subsystemInfo.location = procType;

    std::vector<std::weak_ptr<ISubsystemListener>> applisteners;

    if (subsystem == telux::common::Subsystem::MPSS) {
        q6ListenerMgr_->getAvailableListeners(applisteners);
    } else {
        a7ListenerMgr_->getAvailableListeners(applisteners);
    }

    if (applisteners.empty()) {
        LOG(DEBUG, __FUNCTION__, " no registered listener");
        return;
    }

    for (auto &wp : applisteners) {
        if (auto app = wp.lock()) {
            app->onStateChange(subsystemInfo, newOpStatus);
        }
    }
}

void SubsystemManagerStub::handleSSREvent(google::protobuf::Any event) {
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

/*
 * Invoked when DMS service is no longer available.
 */
void SubsystemManagerStub::onDmsServiceStatusChange(
    telux::common::ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__, ":: Service Status: ", static_cast<int>(srvcStatus));

    if (srvcStatus == getServiceStatus()) {
        return;
    }

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: Deviceinfo Manager Service is UNAVAILABLE/FAILED");
        setServiceStatus(srvcStatus);
        return;
    }

    LOG(INFO, __FUNCTION__, ":: Deviceinfo Manager Service is AVAILABLE");
    /* Should be scheduled after sendNewStatusToClients since initSync may block */
    auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_.add(f);
}

void SubsystemManagerStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE) {
        // De-register all indications from server
        std::vector<std::string> filters{SUBSYSTEM_MANAGER_FILTER};
        clientEventMgr_.deregisterListener(shared_from_this(), filters);
    }
}

ServiceStatus SubsystemManagerStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status SubsystemManagerStub::registerDefaultIndications() {
    Status status = Status::FAILED;
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    status = clientEventMgr_.registerListener(shared_from_this(), DEVICEINFO_MANAGER_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }
    return status;
}

Status SubsystemManagerStub::initSyncComplete(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::FAILED;
    status = registerDefaultIndications();

    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }

    if (srvcStatus != ServiceStatus::SERVICE_AVAILABLE)
    {
        return Status::FAILED;
    }

    if (!q6ListenerMgr_ || !a7ListenerMgr_) {
        LOG(ERROR, __FUNCTION__, ":: Invalid listener manager instance ");
        return Status::FAILED;
    }

    status = clientEventMgr_.registerListener(shared_from_this(), SUBSYSTEM_MANAGER_FILTER);
    if ((status != Status::SUCCESS) &&
            (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering subsystem monitor event failed");
    }

    return status;
}

Status SubsystemManagerStub::init() {
    LOG(DEBUG, __FUNCTION__);

    try {
        createListener();
    } catch (std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ": Invalid listener instance");
        return Status::FAILED;
    }

    return registerDefaultIndications();
}

/*
 * Add client provided listener to the internal list.
 */
telux::common::ErrorCode SubsystemManagerStub::registerListener(
    std::weak_ptr<ISubsystemListener> listener,
    std::vector<telux::common::SubsystemInfo> subsystems) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ErrorCode ec;

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, " Subsystem Manager subsystem is not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (subsystems.empty() || (!listener.lock())) {
        LOG(ERROR, __FUNCTION__, " no subsystem or listener");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    for (telux::common::SubsystemInfo &subsystem : subsystems) {
        if ((subsystem.subsystems & telux::common::Subsystem::MPSS)
            == telux::common::Subsystem::MPSS) {
            ec = registerForMpss(listener, subsystem.location);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                return ec;
            } else {
                registerCombination(Subsystem::MPSS, subsystem.location);
            }
        }
        if ((subsystem.subsystems & telux::common::Subsystem::APSS)
            == telux::common::Subsystem::APSS) {
            ec = registerForApss(listener, subsystem.location);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                return ec;
            } else {
                registerCombination(Subsystem::APSS, subsystem.location);
            }
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Add client's listener for monitoring MPSS state change.
 */
telux::common::ErrorCode SubsystemManagerStub::registerForMpss(
    std::weak_ptr<ISubsystemListener> listener, telux::common::ProcType location) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status;

    if (location != telux::common::ProcType::LOCAL_PROC) {
        /* Running on MDM but trying to monitor Q6 on EAP */
        LOG(ERROR, __FUNCTION__, " can't monitor EAP from MDM");
        return telux::common::ErrorCode::INVALID_ARG;
    }

    status = q6ListenerMgr_->registerListener(listener);

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't register, err ", static_cast<int>(status));
        return telux::common::CommonUtils::toErrorCode(status);
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Add client's listener for monitoring APSS state change.
 */
telux::common::ErrorCode SubsystemManagerStub::registerForApss(
    std::weak_ptr<ISubsystemListener> listener, telux::common::ProcType location) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status;

    if (location == telux::common::ProcType::LOCAL_PROC) {
        /* Running on MDM and trying to monitor MDM itself  */
        LOG(ERROR, __FUNCTION__, " can't monitor mdm from mdm");
        return telux::common::ErrorCode::INVALID_ARG;
    }

    /* Running on MDM and trying to monitor EAP/APQ should be denied.
     * However, application may be running on NAD1 and trying to monitor
     * NAD2, therefore, allow the registration. */
    status = a7ListenerMgr_->registerListener(listener);

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't register, err ", static_cast<int>(status));
        return telux::common::CommonUtils::toErrorCode(status);
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Remove client's listener from internal list.
 */
telux::common::ErrorCode SubsystemManagerStub::deRegisterListener(
    std::weak_ptr<ISubsystemListener> listener) {

    telux::common::Status status;
    resetCombination();

    status = q6ListenerMgr_->deRegisterListener(listener);
    if ((status != telux::common::Status::SUCCESS)
        && (status != telux::common::Status::NOSUCH)) {
        LOG(ERROR, __FUNCTION__, " can't deregister q6, err ", static_cast<int>(status));
        return telux::common::CommonUtils::toErrorCode(status);
    }

    status = a7ListenerMgr_->deRegisterListener(listener);
    if ((status != telux::common::Status::SUCCESS)
        && (status != telux::common::Status::NOSUCH)) {
        LOG(ERROR, __FUNCTION__, " can't deregister a7, err ", static_cast<int>(status));
        return telux::common::CommonUtils::toErrorCode(status);
    }

    return telux::common::ErrorCode::SUCCESS;
}

}  // End of namespace platform
}  // End of namespace telux