/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  LocationManagerServerImpl.hpp
 *
 *
 */

#include "LocationManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "LocationReportService.hpp"
#include "event/EventService.hpp"
#include "FileInfo.hpp"
#include <telux/loc/LocationDefines.hpp>
#include <thread>
#include <chrono>

#include <fstream>
#include <sstream>
#include <vector>
#include <utility>

#define LOC_MGR_API_JSON "api/loc/ILocationManager.json"
#define CSV_BATCH_COUNT 1000
#define DEFAULT_DELIMITER " "
#define DEFAULT_CAPABILITIES 0x12D

LocationManagerServerImpl::LocationManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    init();
}

inline bool fileExists(const std::string &csvFile) {
    std::ifstream f(csvFile.c_str());
    return f.good();
}

bool LocationManagerServerImpl::init() {
    LOG(DEBUG, __FUNCTION__);
    SimulationConfigParser configParser;
    std::string fileName = configParser.getValue("sim.loc.location_report_file_name");
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
    std::string replayCsvStr = configParser.getValue("sim.loc.location_report_replay");
    if(replayCsvStr == "TRUE") {
        replayCsv_ = true;
    }
    return true;
}

void LocationManagerServerImpl::startStreaming() {
    LOG(DEBUG, __FUNCTION__);
    while(true) {
        if(fileBuffer_->getNextBuffer(requestBuffer_)) {
            while(!requestBuffer_.empty()) {
                //Send requestBuffer_[0] to clients via streams.
                ::locStub::StartReportsEvent startReportsEvent;
                ::eventService::EventResponse anyResponse;
                startReportsEvent.set_loc_report(requestBuffer_[0]);
                anyResponse.set_filter("LOC_REPORTS");
                anyResponse.mutable_any()->PackFrom(startReportsEvent);
                //posting the event to EventService event queue
                auto &locationReportService = LocationReportService::getInstance();
                locationReportService.updateEventQueue(anyResponse);

                // Sleep to match the frequency by extracting the current timestamp
                // and subtracting from the previous.
                std::size_t pos = requestBuffer_[0].find(',');
                uint64_t currentTimestamp = std::stoull(requestBuffer_[0].substr(0, pos));
                if(previousTimestamp_ != 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(currentTimestamp - previousTimestamp_));
                }
                previousTimestamp_ = currentTimestamp;

                // Store last location for fetching terrestrial position.
                std::size_t pos2 = requestBuffer_[0].find(',', pos);
                uint32_t opt = std::stoul(requestBuffer_[0].substr(pos + 1, pos2));
                if(opt == telux::loc::GnssReportType::LOCATION) {
                    lastLocInfo_ = requestBuffer_[0];
                }
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
            previousTimestamp_ = 0;
            if(replayCsv_) {
                LOG(INFO, " Last batch streamed. Replaying CSV.");
                triggerResetWindowEvent();
                //Restart buffering
                fileBuffer_->startBuffering();
            } else {
                LOG(INFO, " Last batch streamed. Streaming stopped.");
                triggerStreamingStoppedEvent();
                return;
            }
        }
    }
}

void LocationManagerServerImpl::triggerResetWindowEvent() {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::ResetWindowEvent resetWindowEvent;
    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter("LOC_REPORTS");
    anyResponse.mutable_any()->PackFrom(resetWindowEvent);
    //posting the event to EventService event queue
    auto &locationReportService = LocationReportService::getInstance();
    locationReportService.updateEventQueue(anyResponse);
}

void LocationManagerServerImpl::triggerStreamingStoppedEvent() {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::StreamingStoppedEvent streamingStoppedEvent;
    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter("LOC_REPORTS");
    anyResponse.mutable_any()->PackFrom(streamingStoppedEvent);
    //posting the event to EventService event queue
    auto &locationReportService = LocationReportService::getInstance();
    locationReportService.updateEventQueue(anyResponse);
}

LocationManagerServerImpl::~LocationManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
    if(fileBuffer_) {
        fileBuffer_->cleanup();
    }
}

grpc::Status LocationManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, locStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ILocationManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ILocationManager"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
        std::vector<std::string> filters = {"loc_mgr"};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
        capabilityMask_ = DEFAULT_CAPABILITIES;
        SimulationConfigParser configParser;
        std::string capabilities = configParser.getValue("sim.loc.location_default_capabilities");
        if(!capabilities.empty()) {
            capabilityMask_ = std::stoul(capabilities,nullptr,16);
        }
        auto f = std::async(std::launch::async, [this](){
            this->triggerCapabilitiesUpdateEvent();
        }).share();
        taskQ_.add(f);
    } else {
        LOG(ERROR, "Unable to read LocationManager JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    response->set_delay(cbDelay);
    return grpc::Status::OK;
}

void LocationManagerServerImpl::updateStreamRequest() {
    LOG(DEBUG, __FUNCTION__);
    if(bufferingInitialized_) {
        auto &locationReportService = LocationReportService::getInstance();
        size_t clientSize = locationReportService.getClientsForFilter("LOC_REPORTS");
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

grpc::Status LocationManagerServerImpl::StartBasicReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startBasicReports", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        updateStreamRequest();
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StartDetailedReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startDetailedReports", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        updateStreamRequest();
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StartDetailedEngineReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startDetailedEngineReports", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        updateStreamRequest();
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StopReports(ServerContext* context,
    const google::protobuf::Empty* request, google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    if(bufferingInitialized_) {
        auto &locationReportService = LocationReportService::getInstance();
        size_t clientSize = locationReportService.getClientsForFilter("LOC_REPORTS");
        LOG(DEBUG, __FUNCTION__, " Client size: ", clientSize);
        if(clientSize == 0) {
            SimulationConfigParser configParser;
            std::string stopStreamStr =
                configParser.getValue("sim.loc.location_report_consumption");
            if(stopStreamStr == "TRUE") {
                stopStreamingData_ = true;
            }
        }
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::RegisterLocationSystemInfo(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("registerForSystemInfoUpdates", response);
    auto f = std::async(std::launch::async, [this](){
        this->triggerSysinfoUpdateEvent();
    }).share();
    taskQ_.add(f);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::DeregisterLocationSystemInfo(ServerContext* context,
      const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("deRegisterForSystemInfoUpdates", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::GetTerrestrialPosition(ServerContext* context,
      const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("getTerrestrialPosition", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::CancelTerrestrialPosition(ServerContext* context,
      const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("cancelTerrestrialPositionRequest", response);
    return grpc::Status::OK;
}

void LocationManagerServerImpl::apiJsonReader(
    std::string apiName, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationManager", apiName, status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}

grpc::Status LocationManagerServerImpl::RequestEnergyConsumedInfo(ServerContext* context,
    const google::protobuf::Empty* request, locStub::RequestEnergyConsumedInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationManager", "requestEnergyConsumedInfo",
        status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    if (errorCode == ErrorCode::SUCCESS) {
        uint32_t validity =
            std::stoi(telux::common::CommonUtils::readSystemDataValue("loc/ILocationManager",
                "0", {"ILocationManager", "GnssEnergyConsumedInfo", "valid"}));

        uint32_t energyConsumed =
            std::stoi(telux::common::CommonUtils::readSystemDataValue("loc/ILocationManager",
                "0", {"ILocationManager", "GnssEnergyConsumedInfo", "energySinceFirstBoot"}));

        {
            CommonUtils::writeSystemDataValue<string>("loc/ILocationManager", "1",
                {"ILocationManager", "GnssEnergyConsumedInfo", "valid"});
            CommonUtils::writeSystemDataValue<string>("loc/ILocationManager",
                std::to_string(energyConsumed + 100),
                    {"ILocationManager", "GnssEnergyConsumedInfo", "energySinceFirstBoot"});
        }
        response->set_validity(validity);
        response->set_energy_consumed(energyConsumed);
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::GetYearOfHw(ServerContext* context,
    const google::protobuf::Empty* request, locStub::GetYearOfHwReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationManager", "getYearOfHw",
        status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    if (errorCode == ErrorCode::SUCCESS) {
        uint16_t yearOfHw = std::stoi(telux::common::CommonUtils::readSystemDataValue(
            "loc/ILocationManager", "0", {"ILocationManager", "yearOfHw"}));
        if (yearOfHw == 0) {
            yearOfHw = 2023;
            CommonUtils::writeSystemDataValue<string>("loc/ILocationManager",
                std::to_string(yearOfHw), {"ILocationManager", "yearOfHw"});
        }
        response->set_year_of_hw(yearOfHw);
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::GetCapabilities(ServerContext* context,
    const google::protobuf::Empty* request, locStub::GetCapabilitiesReply* response) {
    LOG(DEBUG, __FUNCTION__);
    response->set_loc_capability(capabilityMask_);
    return grpc::Status::OK;
}

void LocationManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event){
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == "loc_mgr") {
        onEventUpdate(event.event());
    }
}

void LocationManagerServerImpl::onEventUpdate(std::string event){
    LOG(DEBUG, __FUNCTION__,event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token,event);
}

void LocationManagerServerImpl::handleEvent(std::string token, std::string event) {
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", event);
    if (token == "capabilities_update") {
        handleCapabilitiesUpdate(event);
    } else if (token == "sysinfo_update_current") {
        handleSysInfoUpdateCurrent(event);
    } else if (token == "sysinfo_update_leapsecond") {
        handleSysInfoUpdateLeapSecond(event);
    } else if (token == "disaster_crisis_report") {
        handleDisasterCrisisReport(event);
    }
}

void LocationManagerServerImpl::handleDisasterCrisisReport(std::string event) {
    LOG(DEBUG, __FUNCTION__, " Path: ", event);

    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, event);
    if (errorCode == ErrorCode::SUCCESS) {
        int dctype =  rootNode["disaster_crisis"][0].asInt();
        uint32_t numValidBits =  rootNode["disaster_crisis"][1].asInt();
        bool prnValid =  rootNode["disaster_crisis"][2].asBool();
        uint32_t prn =  rootNode["disaster_crisis"][3].asUInt();
        std::vector<uint8_t> data;
        unsigned size = rootNode["disaster_crisis"].size();
        for (unsigned i = 4; i < size; ++i) {
            data.push_back(rootNode["disaster_crisis"][i].asInt());
            LOG(DEBUG, __FUNCTION__, " DC report Data: ", rootNode["disaster_crisis"][i]);
        }

        ::locStub::GnssReportDCType type =
              static_cast<::locStub::GnssReportDCType>(dctype);

        auto f = std::async(std::launch::async,
                            [this, type, numValidBits, prnValid, prn, data](){
                                ::locStub::GnssDisasterCrisisReport report;
                                ::eventService::EventResponse anyResponse;
                                report.set_dc_report_type(type);
                                report.set_num_valid_bits(numValidBits);
                                report.set_prn_validity(prnValid);
                                report.set_prn(prn);
                                report.mutable_dc_report_data()->Add(data.begin(), data.end());
                                anyResponse.set_filter("loc_mgr");
                                anyResponse.mutable_any()->PackFrom(report);
                                auto& eventImpl = EventService::getInstance();
                                eventImpl.updateEventQueue(anyResponse);
                        }).share();
        taskQ_.add(f);
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read JSON");
        return;
    }

}

void LocationManagerServerImpl::handleCapabilitiesUpdate(std::string event) {
    std::string capability;
    uint32_t mask = 0;
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " capability_action is not passed");
    } else {
        try {
            capability = token;
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " capability is not passed");
    } else {
        try {
            mask = std::stoul(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    if(capability == "Add") {
        capabilityMask_ |= mask;
    } else {
        capabilityMask_ ^= mask;
    }
    auto f = std::async(std::launch::async, [this](){
        this->triggerCapabilitiesUpdateEvent();
    }).share();
    taskQ_.add(f);
}


void LocationManagerServerImpl::triggerCapabilitiesUpdateEvent() {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::CapabilitiesUpdateEvent capabilitiesUpdateEvent;
    ::eventService::EventResponse anyResponse;
    capabilitiesUpdateEvent.set_capability_mask(capabilityMask_);
    anyResponse.set_filter("loc_mgr");
    anyResponse.mutable_any()->PackFrom(capabilitiesUpdateEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void LocationManagerServerImpl::handleSysInfoUpdateCurrent(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " sysinfo_current is not passed");
    } else {
        try {
            current_ = std::stoull(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }

    auto f = std::async(std::launch::async, [this](){
        this->triggerSysinfoUpdateEvent();
    }).share();
    taskQ_.add(f);
}

void LocationManagerServerImpl::handleSysInfoUpdateLeapSecond(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " system_week is not passed");
    } else {
        try {
            systemWeek_ = std::stoull(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " system_msec is not passed");
    } else {
        try {
            systemMsec_ = std::stoull(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " leap_seconds_before_change is not passed");
    } else {
        try {
            leapSecondsBeforeChange_ = std::stoull(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " leap_seconds_after_change is not passed");
    } else {
        try {
            leapSecondsAfterChange_ = std::stoull(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    leapsecondValidity_ = 0x03;
    gnssValidity_ = 0x03;
    auto f = std::async(std::launch::async, [this](){
        this->triggerSysinfoUpdateEvent();
    }).share();
    taskQ_.add(f);
}

void LocationManagerServerImpl::triggerSysinfoUpdateEvent() {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::SysInfoUpdateEvent sysInfoUpdateEvent;
    ::eventService::EventResponse anyResponse;
    sysInfoUpdateEvent.set_sysinfo_validity(sysinfoValidity_);
    sysInfoUpdateEvent.set_leapsecond_validity(leapsecondValidity_);
    sysInfoUpdateEvent.set_current(current_);
    sysInfoUpdateEvent.set_leap_seconds_before_change(leapSecondsBeforeChange_);
    sysInfoUpdateEvent.set_leap_seconds_after_change(leapSecondsAfterChange_);
    sysInfoUpdateEvent.set_gnss_validity(gnssValidity_);
    sysInfoUpdateEvent.set_system_week(systemWeek_);
    sysInfoUpdateEvent.set_system_msec(systemMsec_);
    sysInfoUpdateEvent.set_system_clk_time_bias(systemClkTimeBias_);
    sysInfoUpdateEvent.set_system_clk_time_unc_ms(systemClkTimeUncMs_);
    sysInfoUpdateEvent.set_ref_f_count(refFCount_);
    sysInfoUpdateEvent.set_clock_resets(clockResets_);
    anyResponse.set_filter("loc_mgr");
    anyResponse.mutable_any()->PackFrom(sysInfoUpdateEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

grpc::Status LocationManagerServerImpl::GetLastLocation(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LastLocationInfo* response) {
    LOG(DEBUG, __FUNCTION__);
    response->set_loc_report(lastLocInfo_);
    return grpc::Status::OK;
}
