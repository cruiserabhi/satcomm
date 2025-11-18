/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "common/ListenerManager.hpp"
#include "TimeManagerStub.hpp"
#include "loc/LocationFactoryStub.hpp"

namespace telux {
namespace platform {

using namespace telux::common;
using namespace telux::loc;

TimeManagerStub::TimeManagerStub() {
    LOG(DEBUG, __FUNCTION__);

    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

TimeManagerStub::~TimeManagerStub() {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(mutex_);
    exiting_ = true;
    cv_.notify_all();
}

void TimeManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = nullptr;
}

ServiceStatus TimeManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);

    return serviceStatus_;
}

Status TimeManagerStub::init(InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    Status status = telux::common::Status::SUCCESS;

    std::lock_guard<std::mutex> lock(mutex_);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ITimeListener, TimeTypeMask>>();
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " FAILED to create ListenerManager instance");
        status = Status::FAILED;
    }

    if (status != telux::common::Status::SUCCESS) {
        cleanup();
        return status;
    }

    initCb_ = callback;
    auto f  = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_->add(f);

    return status;
}

void TimeManagerStub::initSync() {
    LOG(DEBUG, __FUNCTION__);

    serviceStatus_ = ServiceStatus::SERVICE_AVAILABLE;

    if (initCb_) {
        initCb_(serviceStatus_);
    } else {
        LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

// Overridden from ILocationListener
void TimeManagerStub::onCapabilitiesInfo(const telux::loc::LocCapability capabilityMask) {
    LOG(DEBUG, __FUNCTION__);

    if (capabilityMask & telux::loc::TIME_BASED_TRACKING) {
        std::unique_lock<std::mutex> lck(mutex_);
        if (!timeCap_) {
            LOG(DEBUG, __FUNCTION__, " Time based tracking capability is supported");
            timeCap_ = true;
            cv_.notify_all();
        }
    }
}

// Overridden from ILocationListener
void TimeManagerStub::onBasicLocationUpdate(
    const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) {
    LOG(DEBUG, __FUNCTION__);

    uint64_t utc = 0;
    if (locationInfo) {
        utc = locationInfo->getTimeStamp();
    }

    std::vector<std::weak_ptr<ITimeListener>> listeners;
    {
        std::lock_guard<std::mutex> lock(listenerMtx_);
        listenerMgr_->getAvailableListeners(
            static_cast<uint32_t>(SupportedTimeType::GNSS_UTC_TIME), listeners);
    }
    for (auto &wp : listeners) {
        if (auto sp = std::dynamic_pointer_cast<ITimeListener>(wp.lock())) {
            sp->onGnssUtcTimeUpdate(utc);
        }
    }
}

Status TimeManagerStub::startGnssUtcReport() {
    LOG(DEBUG, __FUNCTION__);

    if (not locMgr_) {
        // avoid re-creating loc manager
        bool statusUpdated = false;
        auto serviceStatus = ServiceStatus::SERVICE_UNAVAILABLE;
        auto statusCb      = [&](telux::common::ServiceStatus status) {
            std::lock_guard<std::mutex> lock(mutex_);
            statusUpdated = true;
            serviceStatus = status;
            cv_.notify_all();
        };

        locMgr_ = telux::loc::LocationFactory::getInstance().getLocationManager(statusCb);
        if (locMgr_) {
            std::unique_lock<std::mutex> lck(mutex_);
            // blocking wait for loc mgr to be ready
            LOG(DEBUG, __FUNCTION__, " Wait for location service available");
            cv_.wait(lck, [&] { return (statusUpdated || exiting_); });
            if (telux::common::ServiceStatus::SERVICE_AVAILABLE != serviceStatus) {
                LOG(ERROR, __FUNCTION__, " Location manager service unavailable");
                locMgr_ = nullptr;
                return Status::FAILED;
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Get location client failed");
            return Status::FAILED;
        }
    }

    // register for loc listener
    if (Status::SUCCESS != locMgr_->registerListenerEx(shared_from_this())) {
        LOG(ERROR, __FUNCTION__, " Failed to register location listener");
        return Status::FAILED;
    }

    auto capabilities = locMgr_->getCapabilities();
    if (capabilities & telux::loc::TIME_BASED_TRACKING) {
        std::unique_lock<std::mutex> lck(mutex_);
        timeCap_ = true;
    } else {
        // blocking wait for time based tracking capability
        LOG(DEBUG, __FUNCTION__, " Wait for time based tracking capability");
        std::unique_lock<std::mutex> lck(mutex_);
        cv_.wait(lck, [&] { return (timeCap_ || exiting_); });
        if (not timeCap_) {
            LOG(ERROR, __FUNCTION__, " Time based tracking capability not support");
            return Status::FAILED;
        }
    }

    bool respRecved       = false;
    auto err              = ErrorCode::INTERNAL_ERR;
    auto responseCallback = [&](ErrorCode error) {
        std::lock_guard<std::mutex> lock(mutex_);
        respRecved = true;
        err        = error;
        cv_.notify_all();
    };

    // start basic utc report with 100ms interval
    if (Status::SUCCESS == locMgr_->startBasicReports(0, 100, responseCallback)) {
        std::unique_lock<std::mutex> lck(mutex_);
        // blocking wait for response
        LOG(DEBUG, __FUNCTION__, " Wait for basic utc report start response");
        cv_.wait(lck, [&] { return (respRecved || exiting_); });
        if (ErrorCode::SUCCESS == err) {
            LOG(INFO, __FUNCTION__, " Basic utc report start success");
            return Status::SUCCESS;
        }
    }

    LOG(ERROR, __FUNCTION__, " Basic utc report start failed");
    return Status::FAILED;
}

Status TimeManagerStub::stopGnssUtcReport() {
    LOG(DEBUG, __FUNCTION__);

    auto ret = Status::SUCCESS;
    if (locMgr_) {
        // stop basic utc report
        std::promise<ErrorCode> p;
        ret = locMgr_->stopReports([&p](ErrorCode error) { p.set_value(error); });
        if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
            LOG(ERROR, __FUNCTION__, " Basic utc report stop failed");
            ret = Status::FAILED;
        }

        // deregister loc listener
        if (Status::SUCCESS != locMgr_->deRegisterListenerEx(shared_from_this())) {
            ret = Status::FAILED;
        }
    }

    return ret;
}

Status TimeManagerStub::registerListener(std::weak_ptr<ITimeListener> listener, TimeTypeMask mask) {
    LOG(DEBUG, __FUNCTION__, " mask:", mask);

    auto ret = Status::SUCCESS;
    {
        std::lock_guard<std::mutex> lock(listenerMtx_);
        TimeTypeMask firstReg;
        ret = listenerMgr_->registerListener(listener, mask, firstReg);
        if (ret != Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Listener registration failed");
            return ret;
        }

        // start GNSS or CV2X time report if it's the first client for that type
        if (firstReg.test(SupportedTimeType::GNSS_UTC_TIME)) {
            ret = (Status::SUCCESS == startGnssUtcReport()) ? ret : Status::FAILED;
        }
    }

    // reverse previous action if any type start fail
    if (Status::FAILED == ret) {
        deregisterListener(listener, mask);
    }

    return ret;
}

Status TimeManagerStub::deregisterListener(
    std::weak_ptr<ITimeListener> listener, TimeTypeMask mask) {
    LOG(DEBUG, __FUNCTION__, " mask:", mask);

    auto ret = Status::SUCCESS;
    {
        std::lock_guard<std::mutex> lock(listenerMtx_);
        TimeTypeMask lastDereg;
        ret = listenerMgr_->deRegisterListener(listener, mask, lastDereg);
        if (ret != Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Listener deregistration failed");
            return ret;
        }

        // stop GNSS or CV2X time report if it's the last client for that type
        if (lastDereg.any()) {
            if (lastDereg.test(SupportedTimeType::GNSS_UTC_TIME)) {
                ret = (Status::SUCCESS == stopGnssUtcReport()) ? ret : Status::FAILED;
            }
        }
    }

    return ret;
}

}  // end of namespace platform
}  // end of namespace telux
