/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <thread>

#include "LocationFactoryStub.hpp"
#include "common/Logger.hpp"

using namespace telux::common;

namespace telux {

namespace loc {

LocationFactory::LocationFactory() {
    LOG(DEBUG, __FUNCTION__);
}

LocationFactory::~LocationFactory() {
   LOG(DEBUG, __FUNCTION__);
}

LocationFactoryStub::LocationFactoryStub()
   : locationManager_(nullptr)
   , locConfigurator_(nullptr)
   , configuratorInitStatus_(ServiceStatus::SERVICE_UNAVAILABLE)
   , dgnssInitStatus_(ServiceStatus::SERVICE_UNAVAILABLE) {
   LOG(DEBUG, __FUNCTION__);
}

LocationFactoryStub::~LocationFactoryStub() {
   if(locConfigurator_) {
      locConfigurator_->cleanup();
   }
}

LocationFactory &LocationFactory::getInstance() {
   return LocationFactoryStub::getInstance();
}

LocationFactory &LocationFactoryStub::getInstance() {
   static LocationFactoryStub instance;
   return instance;
}

void LocationFactoryStub::onGetConfiguratorResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> configuratorCallbacks{};
    {
        // As configurator has synchronous initializaiton, we have to wait till
        // the completion of getLocationConfigurator() to avoid race conditions
        std::unique_lock<std::mutex> cvLock(locationFactoryMutex_);
        while (!configuratorCallbacks_.size()) {
            cv_.wait(cvLock);
        }
        configuratorInitStatus_ = status;
        if (status != ServiceStatus::SERVICE_AVAILABLE) {
            LOG(ERROR, "Failed to initalize location configurator");
            locConfigurator_ = nullptr;
        }
        configuratorCallbacks = configuratorCallbacks_;
        configuratorCallbacks_.clear();
    }
    for (auto &callback : configuratorCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(INFO, __FUNCTION__, "Callback is NULL");
        }
    }
}

void LocationFactoryStub::onGetDgnssManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> dgnssCallbacks{};
    {
        std::lock_guard<std::mutex> lock(locationFactoryMutex_);
        dgnssInitStatus_ = status;
        if (status != ServiceStatus::SERVICE_AVAILABLE) {
            LOG(ERROR, "Failed to initalize Dgnss Manager");
            dgnssManager_ = nullptr;
        }
        dgnssCallbacks = dgnssCallbacks_;
        dgnssCallbacks_.clear();
    }
    for (auto &callback : dgnssCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(INFO, __FUNCTION__, "Callback is NULL");
        }
    }
}

std::shared_ptr<ILocationManager> LocationFactoryStub::getLocationManager(
    telux::common::InitResponseCb callback) {
    std::shared_ptr<LocationManagerStub> locationManager = std::make_shared<LocationManagerStub>();
    if (locationManager) {
        telux::common::Status status = locationManager->init(callback);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, "Failed to initialize the manager");
            return nullptr;
        }
    }
   return locationManager;
}

std::shared_ptr<ILocationConfigurator> LocationFactoryStub::getLocationConfigurator(
    telux::common::InitResponseCb callback) {
    std::shared_ptr<ILocationConfigurator> locConfigurator = nullptr;
    std::lock_guard<std::mutex> lock(locationFactoryMutex_);
    if(locConfigurator_ == nullptr) {
        std::shared_ptr<LocationConfiguratorStub> locationConfigurator = nullptr;
        try {
            locationConfigurator = std::make_shared<LocationConfiguratorStub>();
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        auto initCb = std::bind(&LocationFactoryStub::onGetConfiguratorResponse, this,
            std::placeholders::_1);
        telux::common::Status status = locationConfigurator->init(initCb);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, "Failed to initialize the manager");
            locConfigurator_ = nullptr;
            return nullptr;
        }
        configuratorCallbacks_.push_back(callback);
        locConfigurator_ = locationConfigurator;
        /* This cv is to notify the onGetConfiguratorResponse thread to unblock and execute,
         * if at all it is waiting on this cv. This synchronisation ensures that there will not
         * be any missed callbacks for application in case of race conditions.
         */
        cv_.notify_one();
    } else if (configuratorInitStatus_ == ServiceStatus::SERVICE_UNAVAILABLE) {
        configuratorCallbacks_.push_back(callback);
    } else {
     if (callback) {
            std::thread appCallback(callback, configuratorInitStatus_);
            appCallback.detach();
        } else {
            LOG(INFO, __FUNCTION__, "Callback is NULL");
        }
    }
    return locConfigurator_;
}

std::shared_ptr<IDgnssManager> LocationFactoryStub::getDgnssManager(DgnssDataFormat dataFormat,
        telux::common::InitResponseCb callback) {
    std::shared_ptr<IDgnssManager> dgnssManager = nullptr;
    std::lock_guard<std::mutex> lock(locationFactoryMutex_);
    if(dgnssManager_ == nullptr) {
        std::shared_ptr<DgnssManagerStub> dgnssManager = nullptr;
        try {
            dgnssManager = std::make_shared<DgnssManagerStub>(dataFormat);
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        auto initCb = std::bind(&LocationFactoryStub::onGetDgnssManagerResponse, this,
            std::placeholders::_1);
        telux::common::Status status = dgnssManager->init(initCb);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, "Failed to initialize the manager");
            return nullptr;
        }
        dgnssCallbacks_.push_back(callback);
        dgnssManager_ = dgnssManager;
    } else if (dgnssInitStatus_ == ServiceStatus::SERVICE_UNAVAILABLE) {
        dgnssCallbacks_.push_back(callback);
    } else {
        if (callback) {
            std::thread appCallback(callback, dgnssInitStatus_);
            appCallback.detach();
        } else {
            LOG(INFO, __FUNCTION__, "Callback is NULL");
        }
    }
    return dgnssManager_;
}

}  // end namespace loc
}  // end namespace telux