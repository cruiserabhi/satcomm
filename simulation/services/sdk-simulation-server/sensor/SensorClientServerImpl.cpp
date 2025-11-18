/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerServerImpl.cpp
 *
 *
 */


#include <thread>
#include <chrono>
#include <sys/sysinfo.h>

#include "SensorClientServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/EventService.hpp"
#include "SensorReportService.hpp"
#include "FileInfo.hpp"

#define CSV_BATCH_COUNT 1000
#define SENSOR_CLIENT_API_JSON "api/sensor/ISensorClient.json"
#define SUPPORTED_SENSOR_JSON "api/sensor/SupportedSensors.json"

SensorClientServerImpl::SensorClientServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    init();
}

SensorClientServerImpl::~SensorClientServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
    if(fileBuffer_) {
        fileBuffer_->cleanup();
    }
}

telux::sensor::SensorType SensorClientServerImpl::getSensorType(std::string sensorType) {
    LOG(DEBUG,__FUNCTION__);
    telux::sensor::SensorType type = telux::sensor::SensorType::INVALID;
    if (sensorType == "Accelerometer") {
        type = telux::sensor::SensorType::ACCELEROMETER;
    } else if (sensorType == "Gyroscope") {
        type = telux::sensor::SensorType::GYROSCOPE;
    } else if (sensorType == "Accelerometer_Uncalibrated") {
        type = telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED;
    } else if (sensorType == "Gyroscope_Uncalibrated") {
        type = telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED;
    }
    return type;
}

void SensorClientServerImpl::updateSensorInfo(){
    LOG(DEBUG,__FUNCTION__);
    try{
        Json::Value rootNode;
        telux::common::ErrorCode errorCode
            = JsonParser::readFromJsonFile(rootNode, SUPPORTED_SENSOR_JSON);
        if (errorCode == ErrorCode::SUCCESS) {
            unsigned int numOfSensors = rootNode["sensors"].size();
        /* As Json::Value::ArrayIndex is a typedef of unsigned int. */
            for (Json::Value::ArrayIndex i = 0; i < numOfSensors; i++) {
                telux::sensor::SensorInfo info;
                info.id = std::stoi(rootNode["sensors"][i]["id"].asString());
                info.type = getSensorType(
                rootNode["sensors"][i]["sensor_type"].asString());
                info.name = rootNode["sensors"][i]["sensor_name"].asString();
                info.vendor = rootNode["sensors"][i]["vendor"].asString();
                for (Json::Value::ArrayIndex j = 0;
                    j < rootNode["sensors"][i]["sampling_rate"].size(); j++) {
                    info.samplingRates.push_back(
                    rootNode["sensors"][i]["sampling_rate"][j].asFloat());
                }
                info.maxSamplingRate =
                    std::stof(rootNode["sensors"][i]["max_sampling_rate"].asString());
                info.maxBatchCountSupported =
                    std::stoi(rootNode["sensors"][i]["max_batch_count"].asString());
                info.minBatchCountSupported =
                    std::stoi(rootNode["sensors"][i]["min_batch_count"].asString());
                info.range = std::stoi(rootNode["sensors"][i]["range"].asString());
                info.version = std::stoi(rootNode["sensors"][i]["version"].asString());
                info.resolution = std::stof(rootNode["sensors"][i]["resolution"].asString());
                info.maxRange = std::stof(rootNode["sensors"][i]["max_range"].asString());
                sensorInfo_.emplace_back(info);
            }
        }
    } catch(std::exception const & ex){
        LOG(DEBUG, "Exception Occur ", ex.what());
    }
}

grpc::Status SensorClientServerImpl::GetSensorList(ServerContext* context,
    const google::protobuf::Empty* request, sensorStub::SensorInfoResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    updateSensorInfo();
    for (const auto& dataStruct : sensorInfo_) {
        sensorStub::SensorInfo* data = response->add_sensor_info();
        data->set_id(dataStruct.id);
        data->set_sensor_type(static_cast<uint32_t>(dataStruct.type));
        data->set_name(dataStruct.name);
        data->set_vendor(dataStruct.vendor);
        for (const auto& samplingRate : dataStruct.samplingRates) {
            data->add_sampling_rates(samplingRate);
        }
        data->set_max_sampling_rate(dataStruct.maxSamplingRate);
        data->set_max_batch_count_supported(dataStruct.maxBatchCountSupported);
        data->set_min_batch_count_supported(dataStruct.minBatchCountSupported);
        data->set_range(dataStruct.range);
        data->set_version(dataStruct.version);
        data->set_resolution(dataStruct.resolution);
        data->set_max_range(dataStruct.maxRange);
    }
    sensorInfo_.clear();
    return grpc::Status::OK;
}

grpc::Status SensorClientServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, sensorStub::GetServiceStatusReply* response) {
    LOG(DEBUG,__FUNCTION__);
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, SENSOR_CLIENT_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ISensorClient"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ISensorClient"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
    } else {
        LOG(ERROR, "Unable to read SensorClient JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    response->set_delay(cbDelay);
    std::vector<std::string> filters = {"sensor_mgr"};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);
    return grpc::Status::OK;
}

inline bool fileExists(const std::string &csvFile) {
    std::ifstream f(csvFile.c_str());
    return f.good();
}

bool SensorClientServerImpl::init() {
    LOG(DEBUG, __FUNCTION__);
    SimulationConfigParser configParser;
    std::string fileName = configParser.getValue("sim.sensor.sensor_report_file_name");
    std::string filePath = std::string(DEFAULT_SIM_CSV_FILE_PATH) + fileName;
    if (!fileExists(filePath)) {
        filePath = std::string(DEFAULT_SIM_FILE_PREFIX)
            + std::string(DEFAULT_SIM_CSV_FILE_PATH) + fileName;
        if (!fileExists(filePath)) {
            LOG(DEBUG, __FUNCTION__ , " Failed to open CSV");
            return false;
        }
    }
    fileBuffer_ = std::make_shared<FileBuffer>(filePath, CSV_BATCH_COUNT);
    fileBuffer_->startBuffering();

    bufferingInitialized_ = true;
    std::string replayCsvStr = configParser.getValue("sim.sensor.sensor_report_replay");
    if(replayCsvStr == "TRUE") {
        replayCsv_ = true;
    }

    accelSelfTestCache_.insert({telux::sensor::SelfTestType::POSITIVE,0});
    accelSelfTestCache_.insert({telux::sensor::SelfTestType::NEGATIVE,0});
    accelSelfTestCache_.insert({telux::sensor::SelfTestType::ALL,0});
    gyroSelfTestCache_.insert({telux::sensor::SelfTestType::POSITIVE,0});
    gyroSelfTestCache_.insert({telux::sensor::SelfTestType::NEGATIVE,0});
    gyroSelfTestCache_.insert({telux::sensor::SelfTestType::ALL,0});

    return true;
}

void SensorClientServerImpl::updateStreamRequest() {
    LOG(DEBUG, __FUNCTION__);
    if(bufferingInitialized_) {
        auto &sensorReportService = SensorReportService::getInstance();
        size_t clientSize = sensorReportService.getClientsForFilter("SENSOR_REPORTS");
        LOG(DEBUG, __FUNCTION__, " Client size- ", clientSize);
        if(clientSize == 1) {
            //Initializing/Resetting the flag.
            stopStreamingData_ = false;
            //Starting the stream.
            auto f = std::async(std::launch::deferred,
                [=]() {
                    this->startStreaming();
                }).share();
            taskQ_.add(f);
        }
    }
}


void SensorClientServerImpl::startStreaming() {
    LOG(DEBUG, __FUNCTION__);
    while(true) {
        if(fileBuffer_->getNextBuffer(requestBuffer_)) {
            while(!requestBuffer_.empty()) {
                //Send requestBuffer_[0] to clients via streams.
                std::vector<std::string> message = CommonUtils::splitString(requestBuffer_[0]);
                uint64_t currentTimestamp = std::stoull(message[2]);
                if(lastBatchStreamed_) {
                    /**
                     * During replay, we may reach EOF in between the sample processing for a batch.
                     * This 104Hz sleep is to synchronize the last sample and the first sample of the
                     * CSV since we can't calculate the time difference.
                     */
                    std::this_thread::sleep_for(
                            std::chrono::nanoseconds(8500000));
                    lastBatchStreamed_ = false;
                } else {
                    if(previousTimestamp_ != 0) {
                        std::this_thread::sleep_for(
                            std::chrono::nanoseconds(currentTimestamp-previousTimestamp_));
                    }
                }
                previousTimestamp_ = currentTimestamp;
                //Updating timestamp of sample going out
                timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                uint64_t sampleTimestamp = (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
                message[3] = std::to_string(sampleTimestamp);
                std::string temp = "";
                for(auto s: message) {
                    temp += s;
                    temp += ",";
                }
                temp.pop_back();
                requestBuffer_[0] = temp;
                //Send requestBuffer_[0] to clients via streams.
                ::sensorStub::StartReportsEvent startReportsEvent;
                ::eventService::EventResponse anyResponse;
                startReportsEvent.set_sensor_report(requestBuffer_[0]);
                anyResponse.set_filter("SENSOR_REPORTS");
                anyResponse.mutable_any()->PackFrom(startReportsEvent);
                //posting the event to EventService event queue
                auto &SensorReportService = SensorReportService::getInstance();
                SensorReportService.updateEventQueue(anyResponse);
                requestBuffer_.erase(requestBuffer_.begin());
                // Stop Stream on Request as per config.
                // Will be checked for last client on stop reports.
                if(stopStreamingData_) {
                    LOG(INFO, " Last client de-registered. Streaming stopped.");
                    return;
                }
            }
        } else {
            //EOF is reached and request buffer is empty.
            if(replayCsv_) {
                LOG(INFO, " Last batch streamed. Replaying CSV.");
                //Restart buffering
                fileBuffer_->startBuffering();
                //To continue filling the batch since the CSV can terminate in between a batch fill.
                lastBatchStreamed_ = true;
            } else {
                LOG(INFO, " Last batch streamed. Streaming stopped.");
                triggerStreamingStoppedEvent();
                return;
            }
        }
    }
}

void SensorClientServerImpl::triggerStreamingStoppedEvent() {
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::StreamingStoppedEvent streamingStoppedEvent;
    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter("SENSOR_REPORTS");
    anyResponse.mutable_any()->PackFrom(streamingStoppedEvent);
    //posting the event to EventService event queue
    auto &SensorReportService = SensorReportService::getInstance();
    SensorReportService.updateEventQueue(anyResponse);
}

grpc::Status SensorClientServerImpl::Configure(ServerContext* context,
    const google::protobuf::Empty* request,
    sensorStub::SensorClientCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configure", response);
    return grpc::Status::OK;
}

grpc::Status SensorClientServerImpl::GetConfiguration(ServerContext* context,
    const google::protobuf::Empty* request,
    sensorStub::SensorClientCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("getConfiguration", response);
    return grpc::Status::OK;
}

grpc::Status SensorClientServerImpl::GetSensorInfo(ServerContext* context,
    const google::protobuf::Empty* request,
    sensorStub::SensorClientCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("getSensorInfo", response);
    return grpc::Status::OK;
}

grpc::Status SensorClientServerImpl::Activate(ServerContext* context,
    const sensorStub::ActivateRequest* request,
    sensorStub::SensorClientCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("activate", response);
    if (response->status() == ::commonStub::Status::SUCCESS) {
        ::sensorStub::SensorType sensorType = request->sensor_type();
        if(sensorType == ::sensorStub::SensorType::ACCEL) {
            activeAccelCount_++;
        } else {
            activeGyroCount_++;
        }
        updateStreamRequest();
    }
    return grpc::Status::OK;
}

grpc::Status SensorClientServerImpl::Deactivate(ServerContext* context,
    const sensorStub::DeactivateRequest* request,
    sensorStub::SensorClientCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("deactivate", response);
    if (response->status() == ::commonStub::Status::SUCCESS) {
        if(bufferingInitialized_) {
            auto &SensorReportService = SensorReportService::getInstance();
            size_t clientSize = SensorReportService.getClientsForFilter("SENSOR_REPORTS");
            LOG(DEBUG, __FUNCTION__, " Client size: ", clientSize);
            ::sensorStub::SensorType sensorType = request->sensor_type();
            if(sensorType == ::sensorStub::SensorType::ACCEL) {
                activeAccelCount_--;
            } else {
                activeGyroCount_--;
            }
            if(clientSize == 0) {
                SimulationConfigParser configParser;
                std::string stopStreamStr =
                    configParser.getValue("sim.sensor.sensor_report_consumption");
                if(stopStreamStr == "TRUE") {
                    stopStreamingData_ = true;
                }
            }
        }
    }
    return grpc::Status::OK;
}

grpc::Status SensorClientServerImpl::SensorUpdateRotationMatrix(ServerContext* context,
    const ::google::protobuf::Empty* request,
    sensorStub::SensorClientCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("sensorUpdateRotationMatrix", response);
    return grpc::Status::OK;
}

grpc::Status SensorClientServerImpl::SelfTest (ServerContext* context,
    const sensorStub::SelfTestRequest* request, sensorStub::SelfTestResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::SelfTestType selfTestType = request->selftest_type();
    telux::sensor::SelfTestType selfTest_Type;
    if(selfTestType == ::sensorStub::SelfTestType::SelfTest_Positive) {
        selfTest_Type = telux::sensor::SelfTestType::POSITIVE;
    } else if(selfTestType == ::sensorStub::SelfTestType::SelfTest_Negative) {
        selfTest_Type = telux::sensor::SelfTestType::NEGATIVE;
    } else {
        selfTest_Type = telux::sensor::SelfTestType::ALL;
    }
    ::sensorStub::SensorType sensorType = request->sensor_type();
    bool isActive = false;
    if((sensorType == ::sensorStub::SensorType::ACCEL) && (activeAccelCount_ > 0)) {
        isActive = true;
    } else if ((sensorType == ::sensorStub::SensorType::GYRO) && (activeGyroCount_ > 0)) {
        isActive = true;
    }
    telux::common::ErrorCode errorCode;
    telux::common::Status status;
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, SENSOR_CLIENT_API_JSON);
    int cbDelay = rootNode["ISensorClient"]["selfTest"]["callbackDelay"].asInt();
    response->set_delay(cbDelay);
    std::string statusStr = rootNode["ISensorClient"]["selfTest"]["status"].asString();
    status = CommonUtils::mapStatus(statusStr);
    response->set_status(static_cast<::commonStub::Status>(status));
    std::string errorStr = rootNode["ISensorClient"]["selfTest"]["error"].asString();
    errorCode = CommonUtils::mapErrorCode(errorStr);

    if(status == telux::common::Status::SUCCESS) {
        if((errorCode != telux::common::ErrorCode::SUCCESS) &&
            (errorCode != telux::common::ErrorCode::INFO_UNAVAILABLE)) {
            errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
        }
        uint64_t timestamp;
        if(!isActive) {
            response->set_selftest_result(::sensorStub::Sensor_Idle);
            // Calculating Boot time stamp in ns.
            CommonUtils::calculateBootTimeStamp(timestamp);
            if(sensorType == ::sensorStub::SensorType::ACCEL) {
                accelSelfTestCache_[selfTest_Type] = timestamp;
            } else {
                gyroSelfTestCache_[selfTest_Type] = timestamp;
            }
        } else {
            response->set_selftest_result(::sensorStub::Sensor_Busy);
            // If sensor session is active and this is the first self test.
            if(sensorType == ::sensorStub::SensorType::ACCEL) {
                timestamp = accelSelfTestCache_[selfTest_Type];
            } else {
                timestamp = gyroSelfTestCache_[selfTest_Type];
            }
            if(timestamp == 0) {
                CommonUtils::calculateBootTimeStamp(timestamp);
                if(sensorType == ::sensorStub::SensorType::ACCEL) {
                    accelSelfTestCache_[selfTest_Type] = timestamp;
                } else {
                    gyroSelfTestCache_[selfTest_Type] = timestamp;
                }
                errorCode = telux::common::ErrorCode::INFO_UNAVAILABLE;
            }
        }
        response->set_timestamp(timestamp);
        response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    }
    return grpc::Status::OK;
}

void SensorClientServerImpl::apiJsonReader(
    std::string apiName, sensorStub::SensorClientCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, SENSOR_CLIENT_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ISensorClient", apiName, status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}

void SensorClientServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event){
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == "sensor_mgr") {
        std::string eventStr = event.event();
        std::string token = EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(ERROR, __FUNCTION__, "The event flag is not set!");
            return;
        }
        handleEvent(token,eventStr);
    }
}

void SensorClientServerImpl::handleEvent(std::string token , std::string event) {
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", event);
    if (token == "selfTestFailed") {
        triggerSelfTestFailedEvent(event);
    }
}

void SensorClientServerImpl::triggerSelfTestFailedEvent(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    uint32_t mask = 0;
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " sensor mask is not passed");
        return;
    } else {
        try {
            mask = std::stoul(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    ::sensorStub::SelfTestFailedEvent selfTestFailedEvent;
    ::eventService::EventResponse anyResponse;
    selfTestFailedEvent.set_sensor_mask(mask);
    anyResponse.set_filter("sensor_mgr");
    anyResponse.mutable_any()->PackFrom(selfTestFailedEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}