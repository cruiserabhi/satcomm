/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerServerImpl.hpp
 *
 *
 */


#include "SensorFeatureManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include <telux/common/CommonDefines.hpp>
#include "event/EventService.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "FileInfo.hpp"
#include <fstream>
#include <sstream>

#define SENSOR_FEATURE_MGR_API_JSON "api/sensor/ISensorFeatureManager.json"
#define SENSOR_FEATURE_INFO_JSON  "system-info/sensor/ISensorFeatureManager.json"
#define DEFAULT_DELIMITER " "

SensorFeatureManagerServerImpl::SensorFeatureManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

SensorFeatureManagerServerImpl::~SensorFeatureManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
}

grpc::Status SensorFeatureManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, sensorStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ISensorFeatureManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ISensorFeatureManager"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
        try{
            errorCode = JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_INFO_JSON);
            if (errorCode == ErrorCode::SUCCESS) {
                unsigned int numOfSensors = rootNode["features"].size();
                /* As Json::Value::ArrayIndex is a typedef of unsigned int. */
                for (Json::Value::ArrayIndex i = 0; i < numOfSensors; i++) {
                    std::string feature =  rootNode["features"][i].asString();
                    featureStatusMap_[feature] = false;
                }
            }
        } catch(std::exception const & ex) {
            LOG(DEBUG, "Exception Occur ", ex.what());
        }
    } else {
        LOG(ERROR, " Unable to read SensorFeatureManager JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {"sensor_feature"};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_delay(cbDelay);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    return grpc::Status::OK;
}

void SensorFeatureManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event){
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == "sensor_feature") {
        onEventUpdate(event.event());
    }
}

void SensorFeatureManagerServerImpl::onEventUpdate(std::string event){
    LOG(DEBUG, __FUNCTION__,event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token,event);
}

void SensorFeatureManagerServerImpl::handleEvent(std::string token , std::string event){
    LOG(DEBUG, __FUNCTION__, " The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, " The leftover string is: ", event);
    if (token == "sensor_event") {
        handleFeatureEvent(event);
    }
}

void SensorFeatureManagerServerImpl::handleFeatureEvent(std::string eventParams){
   LOG(DEBUG, __FUNCTION__);
    std::string featureName = "";
    int eventId = -1;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " The featureName is not passed");
    } else {
        featureName = token;
        if(featureStatusMap_.find(featureName) == featureStatusMap_.end()){
           LOG(INFO, __FUNCTION__, " The featureName not exists");
           return;
        }
        if(!featureStatusMap_[featureName]){
            LOG(INFO, __FUNCTION__, " Feature not enabled");
            return;
        }
    }
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
     if(token == "") {
        LOG(INFO, __FUNCTION__, " The eventId is not passed");
    } else {
        eventId =std::stoi(token);
    }
    std::string eventString =
        readBufferedEventStringFromFile("sim.sensor.sensor_buffered_events_file_name",eventId);
    if(eventString != "") {
        auto f = std::async(std::launch::async, [this,featureName,eventId,eventString](){
            this->triggerFeatureEvent(featureName,eventId,eventString);
        }).share();
        taskQ_->add(f);
    }
}

void SensorFeatureManagerServerImpl::triggerFeatureEvent(std::string featureName,
    int id, std::string eventString){
    LOG(DEBUG, __FUNCTION__);
     std::lock_guard<std::mutex> lck(mtx_);
    ::sensorStub::FeatureEvent featureEvent;
    ::eventService::EventResponse anyResponse;
    featureEvent.set_id(id);
    featureEvent.set_featurename(featureName);
    featureEvent.set_events(eventString);
    anyResponse.set_filter("sensor_feature");
    anyResponse.mutable_any()->PackFrom(featureEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

inline bool fileExists(const std::string &csvFile) {
    std::ifstream f(csvFile.c_str());
    return f.good();
}

std::string SensorFeatureManagerServerImpl::readBufferedEventStringFromFile(std::string fileName,
    int eventId){
    LOG(DEBUG, __FUNCTION__);
    bool eventFound=false;
    std::shared_ptr<SimulationConfigParser> configParser =
        std::make_shared<SimulationConfigParser>();
    std::string file = configParser->getValue(fileName);
    std::string csvFilePath = std::string(DEFAULT_SIM_CSV_FILE_PATH) + file;
    if (!fileExists(csvFilePath)) {
        csvFilePath = std::string(DEFAULT_SIM_FILE_PREFIX)
            + std::string(DEFAULT_SIM_CSV_FILE_PATH) + file;
        if (!fileExists(csvFilePath)) {
            LOG(ERROR, __FUNCTION__, "file not exists: ", csvFilePath);
            return "";
        }
    }
    std::ifstream ifs(csvFilePath);
    if(!ifs.is_open()) {
        LOG(ERROR, __FUNCTION__, "Could not open the file: ", csvFilePath);
        return "";
    }

    if(ifs.good()) {
        LOG(DEBUG,__FUNCTION__, " Begin Reading ", csvFilePath);
    }
    std::string line;
    // skip the copyright
    while (ifs.peek() != EOF) {
        std::getline(ifs, line);
        // each line of copyright starts with "##"
        if (line.size() != 0 && line.find("##") != 0) {
            break;
        }
    }

    while (false == eventFound) {
        std::size_t pos = line.find(',');
        if (pos != std::string::npos) {
            std::string id = line.substr(0, pos);
            if(std::stoi(id) == eventId){
                eventFound = true;
                break;
            }
        }
        if (ifs.peek() == EOF) {
            break;
        }
        std::getline(ifs, line);
    }

    if(eventFound == false){
       LOG(ERROR, __FUNCTION__, "EventId not Found in file: ", csvFilePath);
       return "";
    }
    return line;
}

grpc::Status SensorFeatureManagerServerImpl::EnableFeature(ServerContext* context,
    const sensorStub::SensorEnableFeature* request,
    sensorStub::SensorFeatureManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("enableFeature", response);
    if (response->status() == ::commonStub::Status::SUCCESS) {
        if(featureStatusMap_.find(request->feature()) == featureStatusMap_.end()){
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,"feature not exists");
        }
        featureStatusMap_[request->feature()] = true;
    }
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::DisableFeature(ServerContext* context,
    const sensorStub::SensorEnableFeature* request,
    sensorStub::SensorFeatureManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("disableFeature", response);
    if (response->status() == ::commonStub::Status::SUCCESS) {
        if(featureStatusMap_.find(request->feature()) == featureStatusMap_.end()){
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,"feature not exists");
        }
        featureStatusMap_[request->feature()] = false;
    }
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::GetFeatureList(ServerContext* context,
    const google::protobuf::Empty* request, sensorStub::GetFeatureListReply* response){
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    telux::common::Status status;
    int cbDelay = 100;
    std::string featureList = "";
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ISensorFeatureManager"]["DefaultCallbackDelay"].asInt();
        std::string cbStatus = rootNode["ISensorFeatureManager"]["getAvailableFeatures"]
            ["status"].asString();
        status = CommonUtils::mapStatus(cbStatus);
        for (auto const& x : featureStatusMap_) {
            featureList = featureList + x.first + ",";
        }
        featureList.pop_back();
    } else {
        LOG(ERROR, " Unable to read SensorFeatureManager JSON");
    }
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_delay(cbDelay);
    response->set_list(featureList);
    return grpc::Status::OK;
}

void SensorFeatureManagerServerImpl::apiJsonReader(
    std::string apiName, sensorStub::SensorFeatureManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
    telux::common::Status status;
    int cbDelay = 100;
    cbDelay = rootNode["ISensorFeatureManager"]["DefaultCallbackDelay"].asInt();
    std::string cbStatus = rootNode["ISensorFeatureManager"][apiName]["status"].asString();
    status = CommonUtils::mapStatus(cbStatus);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_delay(cbDelay);
}
