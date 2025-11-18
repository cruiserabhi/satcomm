/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Cv2xFactoryStub.hpp"
#include "Cv2xConfigStub.hpp"
#include "Cv2xRadioManagerStub.hpp"
#include "Cv2xThrottleManagerStub.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/Logger.hpp"

#include <telux/cv2x/Cv2xConfig.hpp>
#include <telux/cv2x/Cv2xThrottleManager.hpp>

using std::shared_ptr;

namespace telux {

namespace cv2x {

Cv2xFactory::Cv2xFactory() {
    LOG(DEBUG, __FUNCTION__);
}

Cv2xFactory::~Cv2xFactory() {
    LOG(DEBUG, __FUNCTION__);
}

Cv2xFactory &Cv2xFactory::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    return Cv2xFactoryStub::getInstance();
}

std::shared_ptr<ICv2xRadioManager> Cv2xFactory::getCv2xRadioManager(
    telux::common::InitResponseCb cb) {
    LOG(DEBUG, __FUNCTION__);
    return Cv2xFactoryStub::getInstance().getCv2xRadioManager(cb);
}
std::shared_ptr<ICv2xConfig> Cv2xFactory::getCv2xConfig(telux::common::InitResponseCb cb) {
    LOG(DEBUG, __FUNCTION__);
    return Cv2xFactoryStub::getInstance().getCv2xConfig(cb);
}
std::shared_ptr<ICv2xThrottleManager> Cv2xFactory::getCv2xThrottleManager(
    telux::common::InitResponseCb cb) {
    LOG(DEBUG, __FUNCTION__);
    return Cv2xFactoryStub::getInstance().getCv2xThrottleManager(cb);
}

Cv2xFactoryStub::Cv2xFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

Cv2xFactoryStub::~Cv2xFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
    /*make sure asyn tasks complete before going forward & destructing other
     * members*/
    if (taskQ_) {
        taskQ_->shutdown();
    }
}

Cv2xFactory &Cv2xFactoryStub::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    return getCv2xFactoryStub();
}

Cv2xFactoryStub &Cv2xFactoryStub::getCv2xFactoryStub() {
    static Cv2xFactoryStub instance;
    return instance;
}

void Cv2xFactoryStub::onGetCv2xRadioManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cv2xManagerInitStatus_ = status;
        if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(ERROR, "Fail to initialize Cv2xRadioManager");
        }
        cbs = cv2xManagerInitCallbacks_;
        cv2xManagerInitCallbacks_.clear();
    }
    for (auto &cb : cbs) {
        if (cb) {
            cb(status);
        }
    }
}

shared_ptr<ICv2xRadioManager> Cv2xFactoryStub::getCv2xRadioManager(
    telux::common::InitResponseCb cb) {
    return getCv2xRadioManager(false, cb);
}

void Cv2xFactoryStub::onGetCv2xConfigResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cv2xConfigInitStatus_ = status;
        if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(ERROR, "Fail to initialize Cv2xConfig");
        }

        cbs = cv2xConfigInitCallbacks_;

        cv2xConfigInitCallbacks_.clear();
    }
    for (auto &cb : cbs) {
        if (cb) {
            cb(status);
        }
    }
}

shared_ptr<ICv2xConfig> Cv2xFactoryStub::getCv2xConfig(telux::common::InitResponseCb cb) {
    std::shared_ptr<ICv2xConfig> config = config_.lock();
    std::lock_guard<std::mutex> lock(mutex_);
    if (not config) {
        std::shared_ptr<Cv2xConfigStub> cv2xConfig = nullptr;
        try {
            cv2xConfig = std::make_shared<Cv2xConfigStub>();
        } catch (std::bad_alloc &e) {
            LOG(ERROR, "Fail to get Cv2xConfig due to ", e.what());
            return nullptr;
        }
        cv2xConfig->init(
            [this](telux::common::ServiceStatus status) { onGetCv2xConfigResponse(status); });
        config_ = cv2xConfig;
        config  = cv2xConfig;
        cv2xConfigInitCallbacks_.push_back(cb);
    } else if (cv2xConfigInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        cv2xConfigInitCallbacks_.push_back(cb);
    } else {  // AVAILABLE or FAIL
        if (cb) {
            std::thread appCallback(cb, cv2xConfigInitStatus_);
            appCallback.detach();
        } else {
            LOG(INFO, __FUNCTION__, "Callback is NULL");
        }
    }

    return config;
}

void Cv2xFactoryStub::onGetCv2xThrottleManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cv2xThrottleMgrInitStatus_ = status;
        if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(ERROR, "Fail to initialize Cv2xThrottleManager");
        }

        cbs = cv2xThrottleMgrInitCallbacks_;

        cv2xThrottleMgrInitCallbacks_.clear();
    }
    for (auto &cb : cbs) {
        if (cb) {
            cb(status);
        }
    }
}

shared_ptr<ICv2xThrottleManager> Cv2xFactoryStub::getCv2xThrottleManager(
    telux::common::InitResponseCb cb) {
    std::shared_ptr<ICv2xThrottleManager> throttleMgr = throttleManager_.lock();
    std::lock_guard<std::mutex> lock(mutex_);
    if (not throttleMgr) {
        std::shared_ptr<Cv2xThrottleManagerStub> throttleManager = nullptr;
        try {
            throttleManager = std::make_shared<Cv2xThrottleManagerStub>();
        } catch (std::bad_alloc &e) {
            LOG(ERROR, "Fail to get ThrottleManager due to ", e.what());
            return nullptr;
        }
        throttleManager->init([this](telux::common::ServiceStatus status) {
            onGetCv2xThrottleManagerResponse(status);
        });
        throttleManager_ = throttleManager;
        throttleMgr      = throttleManager;
        cv2xThrottleMgrInitCallbacks_.push_back(cb);
    } else if (cv2xThrottleMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        cv2xThrottleMgrInitCallbacks_.push_back(cb);
    } else {
        if (cb) {
            std::thread appCallback(cb, cv2xThrottleMgrInitStatus_);
            appCallback.detach();
        } else {
            LOG(INFO, __FUNCTION__, "Callback is NULL");
        }
    }
    return throttleMgr;
}

shared_ptr<ICv2xRadioManager> Cv2xFactoryStub::getCv2xRadioManager(
    bool ipcServerMode, telux::common::InitResponseCb cb) {
    std::shared_ptr<ICv2xRadioManager> radioMgr = radioManager_.lock();
    std::lock_guard<std::mutex> lock(mutex_);
    if (not radioMgr) {
        std::shared_ptr<Cv2xRadioManagerStub> cv2xRadioMgr = nullptr;
        try {
            cv2xRadioMgr = std::make_shared<Cv2xRadioManagerStub>(ipcServerMode);
        } catch (std::bad_alloc &e) {
            LOG(ERROR, "Fail to get Cv2xRadioManager due to ", e.what());
            return nullptr;
        }
        cv2xRadioMgr->init(
            [this](telux::common::ServiceStatus status) { onGetCv2xRadioManagerResponse(status); });
        radioManager_ = cv2xRadioMgr;
        radioMgr      = cv2xRadioMgr;
        cv2xManagerInitCallbacks_.push_back(cb);
    } else if (cv2xManagerInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        cv2xManagerInitCallbacks_.push_back(cb);
    } else {  // AVAILABLE or FAIL
        if (cb) {
            std::thread appCallback(cb, cv2xManagerInitStatus_);
            appCallback.detach();
        } else {
            LOG(INFO, __FUNCTION__, "Callback is NULL");
        }
    }

    return radioMgr;
}

}  // namespace cv2x
}  // namespace telux
