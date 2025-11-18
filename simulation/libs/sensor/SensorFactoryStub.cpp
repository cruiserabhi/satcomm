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

/* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFactoryStub.cpp
 *
 *
 */

#include <exception>
#include <memory>
#include <thread>

#include "SensorFactoryStub.hpp"
#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sensor {

SensorFactory::SensorFactory() {
    LOG(DEBUG, __FUNCTION__);
}

SensorFactory::~SensorFactory() {
    LOG(DEBUG, __FUNCTION__);
}

SensorFactoryStub::SensorFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
}

SensorFactoryStub::~SensorFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
}

SensorFactory &SensorFactoryStub::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static SensorFactoryStub instance;
    return instance;
}

SensorFactory &SensorFactory::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    return SensorFactoryStub::getInstance();
}

std::shared_ptr<ISensorFeatureManager> SensorFactoryStub::getSensorFeatureManager(
    telux::common::InitResponseCb clientCallback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(factoryGuard_);
    std::shared_ptr<SensorFeatureManagerStub> sensorFeatureManager = sensorFeatureManager_.lock();
    if (sensorFeatureManager == nullptr) {
        try {
            sensorFeatureManager = std::make_shared<SensorFeatureManagerStub>();

            // Add the client's init callback to the list
            sfmInitCallbacks_.push_back(clientCallback);

            // Set our own initCb to sensor feature manager, this will be executed when the sensor
            // feature manager completes it's initialization on a different thread
            auto initCb = [this](telux::common::ServiceStatus status) {
                LOG(INFO, "Received sensor feature manager initialization status: ",
                    static_cast<int>(status));
                if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                    sensorFeatureManager_.reset();
                }
                initCompleteNotifier(sfmInitCallbacks_, status);
            };
            // If init failed immediately, clear callbacks since we won't be notifying. Further,
            // delete sensor feature manager - we would be returning nullptr immediately
            if (sensorFeatureManager->init(initCb) == telux::common::Status::FAILED) {
                sfmInitCallbacks_.clear();
                sensorFeatureManager = nullptr;
            }
        } catch (std::exception &e) {
            LOG(ERROR, "Unable to initialize sensor feature manager");
            sensorFeatureManager = nullptr;
        }
    } else {
        telux::common::ServiceStatus serviceStatus = sensorFeatureManager->getServiceStatus();
        if (serviceStatus == telux::common::ServiceStatus::SERVICE_FAILED) {
            // If the sub-system has failed, return nullptr indicating that the sub-system was not
            // initialized, delete sensor feature manager so that we have a chance to re-init again
            LOG(ERROR,
                "Sensor feature manager has failed. No sensor feature manager instance provided");
            sensorFeatureManager = nullptr;
        } else if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            // If the sub-system is already available invoke callback immediately, but on a new
            // thread
            LOG(DEBUG,
                "Sensor feature manager is available, notifying initCb with SERVICE_AVAILABLE");
              if (clientCallback) {
                std::thread appCallbackThread([clientCallback] {
                    clientCallback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                });
                appCallbackThread.detach();
            }
        } else if (serviceStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            // If the sensor feature manager is still unavailable, just add the client callback to
            // the list When the sensor feature manager finishes initialization, we will notify this
            // callback via initCompleteNotifier()
            sfmInitCallbacks_.push_back(clientCallback);
        }
      }
    sensorFeatureManager_ = sensorFeatureManager;
    return sensorFeatureManager;
}

std::shared_ptr<ISensorManager> SensorFactoryStub::getSensorManager(
    telux::common::InitResponseCb callback) {

    std::function<std::shared_ptr<ISensorManager>(InitResponseCb)> createAndInit
        = [this](telux::common::InitResponseCb initCb) -> std::shared_ptr<ISensorManager> {
        std::shared_ptr<SensorManagerStub> manager = std::make_shared<SensorManagerStub>();
        if (telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Sensor manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
        " , callback = ", &smInitCallbacks_);
    auto manager = getManager<telux::sensor::ISensorManager>(type, sensorManager_,
        smInitCallbacks_, callback, createAndInit);
    return manager;
}

void SensorFactoryStub::initCompleteNotifier(std::vector<telux::common::InitResponseCb>
    &initCallbacks_,telux::common::ServiceStatus status){
    LOG(INFO, "Notifying sensor initialization status: ", static_cast<int>(status));
    std::vector<telux::common::InitResponseCb> callbacks;
    {
        std::lock_guard<std::mutex> lock(factoryGuard_);
        callbacks = initCallbacks_;
        initCallbacks_.clear();
    }
    for (auto &callback : callbacks) {
          callback(status);
    }
}
}
}

