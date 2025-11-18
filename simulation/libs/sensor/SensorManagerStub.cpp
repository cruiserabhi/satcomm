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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorManagerStub.cpp
 *
 *
 */
#include "SensorManagerStub.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "common/CommonUtils.hpp"

#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1
#define RPC_FAIL_SUFFIX " RPC Request failed - "
#define CHECK_SUB_SYSTEM_STATUS()                                                    \
    do {                                                                             \
        if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) { \
            return telux::common::Status::NOTREADY;                                  \
        }                                                                            \
    } while (0);

namespace telux {
namespace sensor {

SensorManagerStub::SensorManagerStub() {
    setServiceStatus(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
    LOG(DEBUG, __FUNCTION__);
    stub_ = std::move(CommonUtils::getGrpcStub<SensorClientService>());
}

SensorManagerStub::~SensorManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    cleanup();
}

void SensorManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_.shutdown();
}

telux::common::Status SensorManagerStub::init(telux::common::InitResponseCb initCb) {
   LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, initCb]() { this->initSync(initCb); }).share();
    taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

void SensorManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(serviceStatusMutex_);
    serviceStatus_ = status;
}

void SensorManagerStub::initSync(telux::common::InitResponseCb callback){
    LOG(DEBUG,__FUNCTION__);
    initCb_ = callback;
    ::sensorStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    if(reqstatus.ok()) {
        telux::common::ServiceStatus status = static_cast<telux::common::ServiceStatus>(response.service_status());
        setServiceStatus(status);
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        setServiceStatus(telux::common::ServiceStatus::SERVICE_FAILED);
    }
    if (initCb_ && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        initCb_(serviceStatus_);
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        return;
    }
    else {
        LOG(DEBUG, "Sensor sub-system is now available, retrieving sensor list");
        const ::google::protobuf::Empty request;
        ::sensorStub::SensorInfoResponse response;
        ClientContext context;
        ::grpc::Status reqstatus = stub_->GetSensorList(&context, request, &response);
        if(reqstatus.ok()) {
            for (const auto& Sensorinfo : response.sensor_info()) {
                SensorInfo info;
                info.id = Sensorinfo.id();
                info.type = static_cast<SensorType>(Sensorinfo.sensor_type());
                info.name = Sensorinfo.name();
                info.vendor = Sensorinfo.vendor();
                for (const auto& samplingRate : Sensorinfo.sampling_rates()) {
                    info.samplingRates.push_back(samplingRate);
                }
                info.maxSamplingRate = Sensorinfo.max_sampling_rate();
                info.maxBatchCountSupported = Sensorinfo.max_batch_count_supported();
                info.minBatchCountSupported = Sensorinfo.min_batch_count_supported();
                info.range = Sensorinfo.range();
                info.version = Sensorinfo.version();
                info.resolution = Sensorinfo.resolution();
                info.maxRange = Sensorinfo.max_range();

                sensorInfo_.push_back(info);
            }
            if (sensorInfo_.empty()) {
                LOG(ERROR, "Received sensor list with ", sensorInfo_.size(), " sensors");
            } else {
                LOG(DEBUG, "Received sensor list with ", sensorInfo_.size(), " sensors");
                setServiceStatus(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
        } else {
            LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
            setServiceStatus(telux::common::ServiceStatus::SERVICE_FAILED);
        }
    }
}

telux::common::ServiceStatus SensorManagerStub::getServiceStatus() {
    LOG(DEBUG,__FUNCTION__);
    std::lock_guard<std::mutex> lock(serviceStatusMutex_);
    return serviceStatus_;
}

telux::common::Status SensorManagerStub::getAvailableSensorInfo(std::vector<SensorInfo> &info){
    LOG(DEBUG,__FUNCTION__);
    CHECK_SUB_SYSTEM_STATUS();
    if (sensorInfo_.empty()) {
        LOG(ERROR, "sensorInfo_ is empty");
        return telux::common::Status::FAILED;
    }
    info = sensorInfo_;
    return telux::common::Status::SUCCESS;
}

telux::common::Status SensorManagerStub::getSensor(
    std::shared_ptr<ISensorClient> &sensor, std::string name){
    LOG(DEBUG,__FUNCTION__);
    return getSensorClient(sensor,name);
}

telux::common::Status SensorManagerStub::getSensorClient(
    std::shared_ptr<ISensorClient> &sensor, std::string name){
    LOG(DEBUG,__FUNCTION__);
    CHECK_SUB_SYSTEM_STATUS();
    try{
        SensorInfo &sensorInfo = getSensorInfo(name);
        LOG(DEBUG, "Creating the sensor client for sensor: ", name);
        sensor = std::make_shared<SensorClientStub>(sensorInfo, stub_);
        if (sensor) {
            std::dynamic_pointer_cast<SensorClientStub>(sensor)->init();
        }
        return telux::common::Status::SUCCESS;
    } catch (std::invalid_argument *e) {
        LOG(ERROR, "Unable to initialize sensor: ", e->what());
        sensor = nullptr;
        return telux::common::Status::INVALIDPARAM;
    } catch (std::bad_alloc *e) {
        LOG(ERROR, "Unable to initialize sensor: ", e->what());
        sensor = nullptr;
        return telux::common::Status::NOMEMORY;
    } catch (std::runtime_error *e) {
        LOG(ERROR, "Unable to initialize sensor: ", e->what());
        sensor = nullptr;
        return telux::common::Status::NOTREADY;
    } catch (std::exception *e) {
        LOG(ERROR, "Unable to initialize sensor: ", e->what());
        sensor = nullptr;
        return telux::common::Status::FAILED;
    }
}

SensorInfo &SensorManagerStub::getSensorInfo(std::string name) {
    for (SensorInfo &info : sensorInfo_) {
        if (info.name == name) {
            return info;
        }
    }
    // This method is private and all methods calling this method shall expect an exception
    throw new std::invalid_argument("No sensor available with given parameters");
}

telux::common::Status SensorManagerStub::setEulerAngleConfig(EulerAngleConfig eulerAngleConfig) {
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::SensorClientCommandReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    telux::common::Status status;

    if (eulerAngleConfig.roll < 0.0f || eulerAngleConfig.pitch < 0.0f
        || eulerAngleConfig.yaw < 0.0f) {
        LOG(ERROR, __FUNCTION__, " Negative parameters are not supported");
        return telux::common::Status::INVALIDPARAM;
    }

    if (eulerAngleConfig.roll > 360.0f || eulerAngleConfig.pitch > 360.0f
        || eulerAngleConfig.yaw > 360.0f){
        LOG(ERROR, __FUNCTION__, " Input values should be less than 360");
        return telux::common::Status::INVALIDPARAM;
    }

    ::grpc::Status reqstatus = stub_->SensorUpdateRotationMatrix(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    return status;
}

}
}