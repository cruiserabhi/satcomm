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
 * @file       SensorClientStub.cpp
 *
 *
 */
#include "SensorClientStub.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

//Default cb delay.
#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1
#define RPC_FAIL_SUFFIX " RPC Request failed - "
#define SENSOR_SAMPLE_DATE_INDEX 2

namespace telux{
namespace sensor{

SensorClientStub::SensorClientStub(SensorInfo sensorInfo, std::shared_ptr<::sensorStub::SensorClientService::Stub> stub)
    :sensorInfo_(sensorInfo)
    ,stub_(stub)
    ,isConfigurationRequired_(true)
    ,sensorSessionActive_(false)
    ,lastReceivedEvent_(0) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ISensorEventListener>>();
    // To get clean logs
    // [<sensor-id, <sensor-name>]: <log string>
    sensorLogPrefix_ = std::string("[")
                            .append(std::to_string(sensorInfo_.id))
                            .append(", ")
                            .append(sensorInfo_.name)
                            .append("]: ");

    config_.samplingRate = 0;
    config_.batchCount = 0;
    config_.isRotated = 1;
    config_.validityMask.reset();
    config_.updateMask.reset();
    updateSensorSamplingMap();
}

void SensorClientStub::init() {
    LOG(DEBUG,__FUNCTION__);
    auto myself = shared_from_this();
    myself_ = myself;
    std::vector<std::string> filters = {"sensor_mgr"};
    auto &clientEventManager = telux::common::ClientEventManager::getInstance();
    clientEventManager.registerListener(myself_, filters);
}

void SensorClientStub::updateSensorSamplingMap(){
    LOG(DEBUG,__FUNCTION__);
    sensorSamplingMap_[12] = 8;
    sensorSamplingMap_[26] = 4;
    sensorSamplingMap_[52] = 2;
    sensorSamplingMap_[104] = 1;
}

uint64_t SensorClientStub::getSamplesToSkip(uint64_t sampleRate){
    LOG(DEBUG,__FUNCTION__);
    auto it = sensorSamplingMap_.find(sampleRate);
    if (it != sensorSamplingMap_.end()) {
        return it->second;
    } else {
        return 1;
    }
}

SensorClientStub::~SensorClientStub() {
    LOG(DEBUG, sensorLogPrefix_, __FUNCTION__);
}

SensorInfo SensorClientStub::getSensorInfo() {
    SensorInfo sensorInfo={};
    ::sensorStub::SensorClientCommandReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    ::grpc::Status reqstatus = stub_->GetSensorInfo(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        if(status == telux::common::Status::SUCCESS){
            return sensorInfo_;
        }
    }
    return sensorInfo;
}

telux::common::Status SensorClientStub::configure(SensorConfiguration configuration){
    LOG(DEBUG, __FUNCTION__);
    // Whenever a configuration request is received, the configuration could contain multiple
    // requests embedded in it
    // - For continuous streaming, samplingRate and batchCount are mandatory
    // - (Future) for threshold, MotionSensorData thresholds would be mandatory and so on
    // In this method, we check for individual type of requests and configure them accordingly
    bool valid = false;
    telux::common::Status status = telux::common::Status::FAILED;

    // If none of the validity bits are set, do not proceed
    if (configuration.validityMask.none()) {
        return telux::common::Status::INVALIDPARAM;
    }

    SensorConfiguration mergedConfig = mergeConfiguration(configuration);
    if (checkStreamingConfiguration(mergedConfig)) {
        valid = true;
        LOG(DEBUG, sensorLogPrefix_, "Configuring sensor for continuous stream");
        if (sensorSessionActive_) {
            LOG(ERROR, sensorLogPrefix_,
            "Request to configure rejected since the sensor has been activated");
            return telux::common::Status::INVALIDSTATE;
        }
        float samplingRate = updateSamplingRate(mergedConfig.samplingRate);
        uint32_t batchCount = updateBatchCount(mergedConfig.batchCount);
        bool isRotated = mergedConfig.isRotated;
        if(samplingRate == 0 || batchCount == 0) {
            LOG(DEBUG, __FUNCTION__, samplingRate ," ",batchCount);
            return telux::common::Status::INVALIDPARAM;
        }
        LOG(DEBUG, sensorLogPrefix_, "Request to configure: ", samplingRate, ", ", batchCount, ", "
            , isRotated);
        ::sensorStub::SensorClientCommandReply response;
        const ::google::protobuf::Empty request;
        ClientContext context;
        ::grpc::Status reqstatus = stub_->Configure(&context, request, &response);
        if(reqstatus.ok()) {
            status = static_cast<telux::common::Status>(response.status());
        } else {
            LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        }
        if (status == telux::common::Status::SUCCESS) {
            onConfigurationUpdate(sensorInfo_.id,samplingRate,batchCount,isRotated);
            isConfigurationRequired_ = false;
        }
    } else {
        LOG(INFO, sensorLogPrefix_, "Streaming configuration not valid");
    }
    if (!valid) {
          LOG(ERROR, sensorLogPrefix_, "No valid configuration found");
          return telux::common::Status::INVALIDPARAM;
    }
    return status;
}

float SensorClientStub::updateSamplingRate(float sampleRate) {
    float rate = 0;
    if(sampleRate > sensorInfo_.samplingRates.back()) {
        return rate;
    }
    if(sampleRate >= sensorInfo_.maxSamplingRate
        && sampleRate <=sensorInfo_.samplingRates.back()) {
        return sensorInfo_.maxSamplingRate;
    }
    for(auto x: sensorInfo_.samplingRates) {
        if(x <= sampleRate){
            rate=x;
        }
    }
    return rate;
}

uint32_t SensorClientStub::updateBatchCount(uint32_t batchCount) {
    uint32_t max = sensorInfo_.maxBatchCountSupported;
    uint32_t min = sensorInfo_.minBatchCountSupported;
    if(batchCount > max || batchCount < min) return 0;
    return (batchCount/10)*10;
}

SensorConfiguration SensorClientStub::getConfiguration(){
    SensorConfiguration sensorConfig={};
    ::sensorStub::SensorClientCommandReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    ::grpc::Status reqstatus = stub_->GetConfiguration(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        if(status == telux::common::Status::SUCCESS){
            return config_;
        }
    }
    return sensorConfig;
}

telux::common::Status SensorClientStub::activate(){
    LOG(DEBUG, sensorLogPrefix_, " Request to activate");
    if(sensorSessionActive_) {
        LOG(DEBUG, sensorLogPrefix_, " Sensor session already active");
        return telux::common::Status::NOTALLOWED;
    }
    if (isConfigurationRequired_) {
        LOG(INFO, sensorLogPrefix_, "Configuration of sensor necessary before activation...");
        SensorConfiguration configuration;
        // If we already have a valid configuration, use it, else create one
        if (config_.validityMask.test(SensorConfigParams::SAMPLING_RATE)
            && config_.validityMask.test(SensorConfigParams::BATCH_COUNT)
            && config_.validityMask.test(SensorConfigParams::ROTATE)) {
            configuration = config_;
        } else {
            if (sensorInfo_.samplingRates.size() > 0) {
                configuration.samplingRate = sensorInfo_.samplingRates[0];
            } else {
                return telux::common::Status::FAILED;
            }
            configuration.batchCount = sensorInfo_.maxBatchCountSupported;
            configuration.isRotated = true;
            configuration.validityMask.set(SensorConfigParams::SAMPLING_RATE);
            configuration.validityMask.set(SensorConfigParams::BATCH_COUNT);
            configuration.validityMask.set(SensorConfigParams::ROTATE);
        }
        telux::common::Status status = configure(configuration);
        if (status != telux::common::Status::SUCCESS) {
            LOG(INFO, sensorLogPrefix_,
                "Configuration of sensor failed. Not activating the sensor.");
            return status;
        }
    }
    std::vector<std::string> filters = {"SENSOR_REPORTS"};
    auto &sensorReportListener = telux::common::SensorReportListener::getInstance();
    sensorReportListener.registerListener(shared_from_this(), filters);
    telux::common::Status status = telux::common::Status::FAILED;

    ::sensorStub::ActivateRequest request;
    ::sensorStub::SensorClientCommandReply response;
    ClientContext context;
    ::sensorStub::SensorType sensorType;
    if(sensorInfo_.type == SensorType::ACCELEROMETER ||
        sensorInfo_.type == SensorType::ACCELEROMETER_UNCALIBRATED) {
        sensorType = ::sensorStub::SensorType::ACCEL;
    } else {
        sensorType = ::sensorStub::SensorType::GYRO;
    }
    request.set_sensor_type(sensorType);

    ::grpc::Status reqstatus = stub_->Activate(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        sensorSessionActive_ = true;
        sampleCountFromMap_ = getSamplesToSkip(config_.samplingRate);
    }
    return status;
}

telux::common::Status SensorClientStub::deactivate(){
    LOG(DEBUG, sensorLogPrefix_, "Request to deactivate");
    if(!sensorSessionActive_) {
        LOG(DEBUG, sensorLogPrefix_, " Sensor session already inactive");
        return telux::common::Status::NOTALLOWED;
    }
    std::vector<std::string> filters = {"SENSOR_REPORTS"};
    auto &sensorReportListener = telux::common::SensorReportListener::getInstance();
    sensorReportListener.deregisterListener(shared_from_this(), filters);
    telux::common::Status status = telux::common::Status::FAILED;

    ::sensorStub::DeactivateRequest request;
    ::sensorStub::SensorClientCommandReply response;
    ClientContext context;
    ::sensorStub::SensorType sensorType;
    if(sensorInfo_.type == SensorType::ACCELEROMETER ||
        sensorInfo_.type == SensorType::ACCELEROMETER_UNCALIBRATED) {
        sensorType = ::sensorStub::SensorType::ACCEL;
    } else {
        sensorType = ::sensorStub::SensorType::GYRO;
    }
    request.set_sensor_type(sensorType);

    ::grpc::Status reqstatus = stub_->Deactivate(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        sensorSessionActive_ = false;
        isConfigurationRequired_ = true;
        receivedSampleCount_ = 0;
        events_.clear();
        sampleCountFromMap_ = 1;
        firstSample_ = true;
    }
    return status;
}

telux::common::Status SensorClientStub::registerListener(
    std::weak_ptr<ISensorEventListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status SensorClientStub::deregisterListener(
    std::weak_ptr<ISensorEventListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

telux::common::Status SensorClientStub::selfTest(
    SelfTestType selfTestType, SelfTestResultCallback cb){
    LOG(DEBUG, sensorLogPrefix_, __FUNCTION__);
    if (!cb) {
        LOG(ERROR, sensorLogPrefix_, "Callback cannot be nullptr");
        return telux::common::Status::INVALIDPARAM;
    }
    ::sensorStub::SelfTestRequest request;
    ::sensorStub::SelfTestResponse response;
    ClientContext context;
    ::sensorStub::SelfTestType selfTest_Type;
    if(selfTestType == telux::sensor::SelfTestType::POSITIVE) {
        selfTest_Type = ::sensorStub::SelfTestType::SelfTest_Positive;
    } else if(selfTestType == telux::sensor::SelfTestType::NEGATIVE) {
        selfTest_Type = ::sensorStub::SelfTestType::SelfTest_Negative;
    } else if(selfTestType == telux::sensor::SelfTestType::ALL) {
        selfTest_Type = ::sensorStub::SelfTestType::SelfTest_All;
    }
    request.set_selftest_type(selfTest_Type);
    ::sensorStub::SensorType sensorType;
    if(sensorInfo_.type == SensorType::ACCELEROMETER ||
        sensorInfo_.type == SensorType::ACCELEROMETER_UNCALIBRATED) {
        sensorType = ::sensorStub::SensorType::ACCEL;
    } else {
        sensorType = ::sensorStub::SensorType::GYRO;
    }
    request.set_sensor_type(sensorType);

    grpc::Status reqStatus = stub_->SelfTest(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    if(status == telux::common::Status::SUCCESS) {
        telux::common::ErrorCode errorCode =
            static_cast<telux::common::ErrorCode>(response.error());
        ::sensorStub::SelfTestResult selfTestResult = response.selftest_result();
        if(selfTestResult == ::sensorStub::SelfTestResult::Sensor_Busy) {
            errorCode = telux::common::ErrorCode::DEVICE_IN_USE;
        }
        int cbDelay = static_cast<int>(response.delay());
        auto f = std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                cb(errorCode);
        }).share();
        taskQ_.add(f);
    }

    return status;
}

telux::common::Status SensorClientStub::selfTest(SelfTestType selfTestType,
    SelfTestExResultCallback cb) {
    LOG(DEBUG, sensorLogPrefix_, __FUNCTION__);
    if (!cb) {
        LOG(ERROR, sensorLogPrefix_, "Callback cannot be nullptr");
        return telux::common::Status::INVALIDPARAM;
    }
    ::sensorStub::SelfTestRequest request;
    ::sensorStub::SelfTestResponse response;
    ClientContext context;
    ::sensorStub::SelfTestType selfTest_Type;
    if(selfTestType == telux::sensor::SelfTestType::POSITIVE) {
        selfTest_Type = ::sensorStub::SelfTestType::SelfTest_Positive;
    } else if(selfTestType == telux::sensor::SelfTestType::NEGATIVE) {
        selfTest_Type = ::sensorStub::SelfTestType::SelfTest_Negative;
    } else if(selfTestType == telux::sensor::SelfTestType::ALL) {
        selfTest_Type = ::sensorStub::SelfTestType::SelfTest_All;
    }
    request.set_selftest_type(selfTest_Type);
    ::sensorStub::SensorType sensorType;
    if(sensorInfo_.type == SensorType::ACCELEROMETER ||
        sensorInfo_.type == SensorType::ACCELEROMETER_UNCALIBRATED) {
        sensorType = ::sensorStub::SensorType::ACCEL;
    } else {
        sensorType = ::sensorStub::SensorType::GYRO;
    }
    request.set_sensor_type(sensorType);

    grpc::Status reqStatus = stub_->SelfTest(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
        return telux::common::Status::FAILED;
    }
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    if(status == telux::common::Status::SUCCESS) {
        telux::common::ErrorCode errorCode =
            static_cast<telux::common::ErrorCode>(response.error());
        SelfTestResultParams selfTestResultParams;
        ::sensorStub::SelfTestResult selfTestResult = response.selftest_result();
        if(selfTestResult == ::sensorStub::SelfTestResult::Sensor_Busy) {
            selfTestResultParams.sensorResultType_ = SensorResultType::HISTORICAL;
        } else {
            selfTestResultParams.sensorResultType_ = SensorResultType::CURRENT;
        }
        selfTestResultParams.timestamp_ = response.timestamp();
        int cbDelay = static_cast<int>(response.delay());
        auto f = std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                cb(errorCode, selfTestResultParams);
        }).share();
        taskQ_.add(f);
    }

    return status;
}

telux::common::Status SensorClientStub::enableLowPowerMode(){
    LOG(DEBUG, sensorLogPrefix_, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status SensorClientStub::disableLowPowerMode(){
    LOG(DEBUG, sensorLogPrefix_, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

void SensorClientStub::onConfigurationUpdate(int sensorId, float samplingRate, int batchCount, bool isRotated) {
    LOG(INFO, sensorLogPrefix_, "Received configuration update on sensor: [", sensorId, ", ",
        samplingRate, ", ", batchCount, ", ", isRotated, "]");
    // If it is an update we should be concerned with in this Sensor object
    if (sensorId == sensorInfo_.id) {
        // Update the configuration and validity flag
        SensorConfiguration configuration;
        configuration.samplingRate = samplingRate;
        configuration.validityMask.set(SensorConfigParams::SAMPLING_RATE);
        configuration.batchCount = batchCount;
        configuration.validityMask.set(SensorConfigParams::BATCH_COUNT);
        configuration.isRotated= isRotated;
        configuration.validityMask.set(SensorConfigParams::ROTATE);

        // Set validity bits in the config cache as well for sampling rate, batch count and Rotate
        config_.validityMask.set(SensorConfigParams::SAMPLING_RATE);
        config_.validityMask.set(SensorConfigParams::BATCH_COUNT);
        config_.validityMask.set(SensorConfigParams::ROTATE);

        // Update the config cache and get the update mask, note that config cache
        // does not need the update flag to be set
        SensorConfigMask mask = updateConfig(configuration);
        configuration.updateMask = mask;
        auto f = std::async(std::launch::async, [this, configuration]() {
            notifyConfigurationUpdate(configuration);
        }).share();
        taskQ_.add(f);
    }
}

SensorConfiguration SensorClientStub::mergeConfiguration(SensorConfiguration requestedConfig) {
    SensorConfiguration mergedConfiguration = config_;

    LOG(DEBUG, "Merging configurations");
    // Merge the requested config with current config, with priority to requested configuration
    if (requestedConfig.validityMask.test(SensorConfigParams::SAMPLING_RATE)) {
        mergedConfiguration.samplingRate = requestedConfig.samplingRate;
        mergedConfiguration.validityMask.set(SensorConfigParams::SAMPLING_RATE);
    }
    if (requestedConfig.validityMask.test(SensorConfigParams::BATCH_COUNT)) {
        mergedConfiguration.batchCount = requestedConfig.batchCount;
        mergedConfiguration.validityMask.set(SensorConfigParams::BATCH_COUNT);
    }
    if (requestedConfig.validityMask.test(SensorConfigParams::ROTATE)) {
        mergedConfiguration.isRotated = requestedConfig.isRotated;
        mergedConfiguration.validityMask.set(SensorConfigParams::ROTATE);
    }
    return mergedConfiguration;
}

bool SensorClientStub::checkStreamingConfiguration(SensorConfiguration configuration) {
    // If both the required validity bits are set
    if (configuration.validityMask.test(SensorConfigParams::SAMPLING_RATE)
        && configuration.validityMask.test(SensorConfigParams::BATCH_COUNT)) {
        return true;
    }
    return false;
}

SensorConfigMask SensorClientStub::updateConfig(SensorConfiguration &configuration) {
    SensorConfigMask mask;
    if (config_.samplingRate != configuration.samplingRate) {
        mask.set(SensorConfigParams::SAMPLING_RATE);
        config_.samplingRate = configuration.samplingRate;
    }
    if (config_.batchCount != configuration.batchCount) {
        mask.set(SensorConfigParams::BATCH_COUNT);
        config_.batchCount = configuration.batchCount;
    }
    if (config_.isRotated != configuration.isRotated) {
        mask.set(SensorConfigParams::ROTATE);
        config_.isRotated = configuration.isRotated;
    }
    return mask;
}

void SensorClientStub::notifyConfigurationUpdate(SensorConfiguration configuration) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ISensorEventListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                sp->onConfigurationUpdate(configuration);
            }
        }
    }
}

void SensorClientStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::sensorStub::StartReportsEvent>()) {
        ::sensorStub::StartReportsEvent startEvent;
        event.UnpackTo(&startEvent);
        parseRequest(startEvent);
    } else if (event.Is<::sensorStub::StreamingStoppedEvent>()) {
        LOG(DEBUG, __FUNCTION__, " StreamingStopped update");
        ::sensorStub::StreamingStoppedEvent streamingStoppedEvent;
        event.UnpackTo(&streamingStoppedEvent);
        handleStreamingStoppedEvent();
    } else if (event.Is<::sensorStub::SelfTestFailedEvent>()) {
        LOG(DEBUG, __FUNCTION__, " SelfTestFailed update");
        ::sensorStub::SelfTestFailedEvent selfTestFailedEvent;
        event.UnpackTo(&selfTestFailedEvent);
        handleSelfTestFailedEvent(selfTestFailedEvent);
    }
}

void SensorClientStub::handleStreamingStoppedEvent() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [this]() { deactivate(); }).share();
    taskQ_.add(f);
}

void SensorClientStub::handleSelfTestFailedEvent(
    ::sensorStub::SelfTestFailedEvent &selfTestFailedEvent) {
    LOG(DEBUG, __FUNCTION__);
    uint32_t mask = selfTestFailedEvent.sensor_mask();
    if ((mask & telux::sensor::SelfTestFail::ACCEL)
        && (sensorInfo_.type == SensorType::ACCELEROMETER
            || sensorInfo_.type == SensorType::ACCELEROMETER_UNCALIBRATED)) {
        notifySelfTestFailedEvent();
    } else if ((mask & telux::sensor::SelfTestFail::GYRO)
               && (sensorInfo_.type == SensorType::GYROSCOPE
                   || sensorInfo_.type == SensorType::GYROSCOPE_UNCALIBRATED)) {
        notifySelfTestFailedEvent();
    }
}

void SensorClientStub::notifySelfTestFailedEvent() {
    LOG(ERROR, sensorLogPrefix_, __FUNCTION__);
     if (listenerMgr_) {
        std::vector<std::weak_ptr<ISensorEventListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                sp->onSelfTestFailed();
            }
        }
    }
}

void SensorClientStub::notifySensorEvent(std::shared_ptr<std::vector<SensorEvent>> events) {
    LOG(DEBUG, sensorLogPrefix_, "Notifying sensor event");
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ISensorEventListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                sp->onEvent(events);
            }
        }
    }
}

void SensorClientStub::parseSensorEvents(std::vector<std::vector<std::string>> events, uint32_t count)
{
    LOG(DEBUG, __FUNCTION__);
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t t = (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
    LOG(DEBUG, sensorLogPrefix_, "Received sensor event with ", count, " events @ ", t, ", after ",
        t - lastReceivedEvent_);
    lastReceivedEvent_ = t;

    std::shared_ptr<std::vector<SensorEvent>> sensorEvents
        = std::make_shared<std::vector<SensorEvent>>();
    for (uint32_t i = 0; i < count; ++i) {
        SensorEvent event;
        size_t itr = SENSOR_SAMPLE_DATE_INDEX;
        event.timestamp           = std::stoull(events[i][itr++]);
        event.uncalibrated.data.x = std::stof(events[i][itr++]);
        event.uncalibrated.data.y = std::stof(events[i][itr++]);
        event.uncalibrated.data.z = std::stof(events[i][itr++]);
        event.uncalibrated.bias.x = std::stof(events[i][itr++]);
        event.uncalibrated.bias.y = std::stof(events[i][itr++]);
        event.uncalibrated.bias.z = std::stof(events[i][itr++]);
        LOG(DEBUG, sensorLogPrefix_, event.timestamp, ", ", event.uncalibrated.data.x, ", ",
            event.uncalibrated.data.y, ", ", event.uncalibrated.data.z, ", ", event.uncalibrated.bias.x, ", ",
            event.uncalibrated.bias.y, ", ", event.uncalibrated.bias.z);
        sensorEvents->push_back(event);
    }
    notifySensorEvent(sensorEvents);
}

void SensorClientStub::batchSensorEvents(std::vector<std::string> message)
{
    if (firstSample_) {
        events_.push_back(message);
        firstSample_ = false;
    }
    else {
        ++receivedSampleCount_;
        if (receivedSampleCount_ == sampleCountFromMap_) {
            events_.push_back(message);
            receivedSampleCount_ = 0;
            if (events_.size() == config_.batchCount) {
                parseSensorEvents(events_, config_.batchCount);
                events_.clear();
            }
        }
    }
}

void SensorClientStub::parseRequest(::sensorStub::StartReportsEvent startEvent) {
    LOG(DEBUG, __FUNCTION__);
    std::string msg = startEvent.sensor_report();
    std::vector<std::string> message = CommonUtils::splitString(msg);
    uint32_t type = std::stoul(message[0]);
    uint32_t rotated = std::stoul(message[1]);
    if ( (type == static_cast<uint32_t>(sensorInfo_.type)) && (rotated == config_.isRotated)
    && (sensorSessionActive_) )
    {
        batchSensorEvents(message);
    }
}


}

}