/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       LocationConfiguratorServerImpl.hpp
 *
 *
 */

#include "LocationConfiguratorServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/EventService.hpp"

#define LOC_CONFIG_API_JSON "api/loc/ILocationConfigurator.json"
#define DEFAULT_DELIMITER " "

LocationConfiguratorServerImpl::LocationConfiguratorServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

LocationConfiguratorServerImpl::~LocationConfiguratorServerImpl() {
    LOG(DEBUG, __FUNCTION__, " Destructing");
}

grpc::Status LocationConfiguratorServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, locStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ILocationConfigurator"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ILocationConfigurator"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
    } else {
        LOG(ERROR, "Unable to read LocationConfigurator JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {"loc_config"};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
        taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    }
    response->set_delay(cbDelay);
    return grpc::Status::OK;
}

void LocationConfiguratorServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event){
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == "loc_config") {
        onEventUpdate(event.event());
    }
}

void LocationConfiguratorServerImpl::onEventUpdate(std::string event){
    LOG(DEBUG, __FUNCTION__,event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token,event);

}

void LocationConfiguratorServerImpl::handleEvent(std::string token, std::string event){
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", event);
    if (token == "xtra_status") {
        handleXtraUpdateEvent(event);
    } else if (token == "constellation_update"){
        handleGnssConstellationUpdateEvent(event);
    }
}

void LocationConfiguratorServerImpl::handleXtraUpdateEvent(std::string event){
    LOG(DEBUG, __FUNCTION__);
    int validity=0;
    int dataStatus=0;
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The validity is not passed");
    } else {
        try {
            validity = std::stoi(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The dataStatus is not passed");
    } else {
        try {
            dataStatus = std::stoi(token);
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", std::to_string(validity),
                {"ILocationConfigurator", "XtraParams", "xtraValidForHours"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",std::to_string(dataStatus),
                {"ILocationConfigurator","XtraParams", "xtraDataStatus"});
    auto f = std::async(std::launch::async, [this](){
        this->triggerXtraStatusEvent();
    }).share();
    taskQ_->add(f);
}

void LocationConfiguratorServerImpl::handleGnssConstellationUpdateEvent(std::string event) {
    std::string enabledMask = " ";
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Mask is not passed");
        enabledMask = "0X1FFFFF";
    } else {
        try {
            enabledMask = token;
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", enabledMask,
        {"ILocationConfigurator", "GnssSignalType"});
    auto f = std::async(std::launch::async, [this](){
        this->triggerGnssConstellationUpdateEvent();
    }).share();
    taskQ_->add(f);
}

void LocationConfiguratorServerImpl::triggerXtraStatusEvent() {
    LOG(DEBUG, __FUNCTION__);
    uint32_t enable = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
        {"ILocationConfigurator", "XtraParams", "enable"}));
    uint32_t dataStatus;
    if(xtraConsent_) {
        dataStatus = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "XtraParams", "xtraDataStatus"}));
    } else {
        //If Xtra consent is false, Data status is Unknown.
        dataStatus = 0;
    }
    uint32_t validHours = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
        {"ILocationConfigurator", "XtraParams", "xtraValidForHours"}));
    LOG(DEBUG, __FUNCTION__,enable,dataStatus,validHours);
    std::lock_guard<std::mutex> lck(mtx_);
    ::locStub::XtraStatusEvent xtraEvent;
    ::eventService::EventResponse anyResponse;
    xtraEvent.set_enable(enable);
    xtraEvent.set_validity(validHours);
    xtraEvent.set_datastatus(dataStatus);
    xtraEvent.set_consent(xtraConsent_);
    anyResponse.set_filter("loc_config");
    anyResponse.mutable_any()->PackFrom(xtraEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void LocationConfiguratorServerImpl::triggerGnssConstellationUpdateEvent() {
    LOG(DEBUG, __FUNCTION__);
    std::string enabledMask = CommonUtils::readSystemDataValue("loc/ILocationConfigurator",
        "0x1FFFFF", {"ILocationConfigurator","GnssSignalType"});
    std::lock_guard<std::mutex> lck(mtx_);
    ::locStub::GnssUpdateEvent GnssEvent;
    ::eventService::EventResponse anyResponse;
    GnssEvent.set_enabledmask(std::stoul(enabledMask,nullptr,16));
    anyResponse.set_filter("loc_config");
    anyResponse.mutable_any()->PackFrom(GnssEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);

}

grpc::Status LocationConfiguratorServerImpl::RegisterListener (ServerContext* context,
    const locStub::RegisterListenerRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    if(request->xtra_indication() == true){
        auto f = std::async(std::launch::async, [this](){
            this->triggerXtraStatusEvent();
        }).share();
     taskQ_->add(f);
    }
    if(request->gnss_indication() == true){
        auto f = std::async(std::launch::async, [this](){
            this->triggerGnssConstellationUpdateEvent();
        }).share();
    taskQ_->add(f);
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureCTUNC (ServerContext* context,
    const locStub::ConfigureCTUNCRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    bool enable = request->enable();
    float timeUncertainty = request->time_uncertainty();
    uint32_t energyBudget = request->energy_budget();
    apiJsonReader("configureCTunc", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", std::to_string(enable),
            {"ILocationConfigurator", "CTunc", "enable"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(timeUncertainty), {"ILocationConfigurator", "CTunc", "timeUncertainty"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(energyBudget), {"ILocationConfigurator", "CTunc", "energyBudget"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigurePACE (ServerContext* context,
    const locStub::ConfigurePACERequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    bool enable = request->enable();
    apiJsonReader("configurePACE", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enable), {"ILocationConfigurator", "PACE", "enable"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::DeleteAllAidingData (ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("deleteAllAidingData", response);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureLeverArm (ServerContext* context,
    const locStub::ConfigureLeverArmRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureLeverArm", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        for(auto itr: request->lever_arm_config_info()) {
            if(itr.first == 1) {
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.forward_offset()),
                        {"ILocationConfigurator", "LeverArm", "GNSSTOVRPforwardOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.sideways_offset()),
                        {"ILocationConfigurator", "LeverArm", "GNSSTOVRPsidewaysOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.up_offset()),
                        {"ILocationConfigurator", "LeverArm", "GNSSTOVRPupOffset"});
            }
            if(itr.first == 2) {
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.forward_offset()),
                        {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSforwardOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.sideways_offset()),
                        {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSsidewaysOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.up_offset()),
                        {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSupOffset"});
            }
            if(itr.first == 3) {
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.forward_offset()),
                        {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSforwardOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.sideways_offset()),
                        {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSsidewaysOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.up_offset()),
                        {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSupOffset"});
            }
        }
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureConstellations (ServerContext* context,
    const locStub::ConfigureConstellationsRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureConstellations", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        std::string blacklist = "";
        for(int ind = 0; ind < request->sv_black_list_info_size(); ind++) {
            blacklist += std::to_string(
                static_cast<int>(request->sv_black_list_info(ind).constellation()));
            blacklist += " : ";
            blacklist += std::to_string(request->sv_black_list_info(ind).sv_id());
            blacklist += ", ";
        }
        if(!blacklist.empty()) {
            blacklist.pop_back();
            blacklist.pop_back();
        }
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", blacklist,
            {"ILocationConfigurator", "configureConstellations", "Blacklist"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureMinGpsWeek (ServerContext* context,
    const locStub::ConfigureMinGpsWeekRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t minGpsWeek = request->min_gps_week();
    apiJsonReader("configureMinGpsWeek", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(minGpsWeek), {"ILocationConfigurator", "MinGpsWeek", "mingpsweek"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestMinGpsWeek(ServerContext* context,
    const google::protobuf::Empty* request, locStub::RequestMinGpsWeekReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", "requestMinGpsWeek", status, errorCode,
        cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    uint16_t minGpsWeek = 0;
    if (errorCode == ErrorCode::SUCCESS) {
        minGpsWeek = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "MinGpsWeek", "mingpsweek"}));
    }
    response->set_min_gps_week(minGpsWeek);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureMinSVElevation (ServerContext* context,
    const locStub::ConfigureMinSVElevationRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t minSVElevation = request->min_sv_elevation();
    apiJsonReader("configureMinSVElevation", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(minSVElevation),
                {"ILocationConfigurator", "MinSvElevation", "minSVElevation"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestMinSVElevation (ServerContext* context,
        const google::protobuf::Empty* request, locStub::RequestMinSVElevationReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", "requestMinSVElevation", status,
        errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    uint16_t minSVElevation = 0;
    if (errorCode == ErrorCode::SUCCESS) {
        minSVElevation = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator",
            "0", {"ILocationConfigurator", "MinSvElevation", "minSVElevation"}));
    }
    response->set_min_sv_elevation(minSVElevation);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureRobustLocation (ServerContext* context,
    const locStub::ConfigureRobustLocationRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    bool enable = request->enable();
    bool enableForE911 = request->enable_for_e911();
    apiJsonReader("configureRobustLocation", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", std::to_string(enable),
            {"ILocationConfigurator", "RobustLocation", "enable"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enableForE911),
                {"ILocationConfigurator", "RobustLocation", "enableForE911"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestRobustLocation (ServerContext* context,
        const google::protobuf::Empty* request, locStub::RequestRobustLocationReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", "requestRobustLocation", status,
        errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    response->mutable_robust_location_configuration()->set_enabled(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "RobustLocation", "enable"})));
    response->mutable_robust_location_configuration()->set_enabled_for_e911(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "RobustLocation", "enableForE911"})));
    response->mutable_robust_location_configuration()->set_valid_mask(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "7",
            {"ILocationConfigurator", "RobustLocation", "validity"})));
    response->mutable_robust_location_configuration()->mutable_version()->set_major_version(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "1",
            {"ILocationConfigurator", "RobustLocation", "majorversion"})));
    response->mutable_robust_location_configuration()->mutable_version()->set_minor_version(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "1",
            {"ILocationConfigurator", "RobustLocation", "minorversion"})));

    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureSecondaryBand (ServerContext* context,
    const locStub::ConfigureSecondaryBandRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureSecondaryBand", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        std::string secBandSet = "";
        for(int ind = 0; ind < request->constellation_set_size(); ind++) {
            int id = static_cast<int>(request->constellation_set(ind));
            secBandSet += std::to_string(id);
            secBandSet += ", ";
        }
        if(!secBandSet.empty()) {
            secBandSet.pop_back();
            secBandSet.pop_back();
        }
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", secBandSet,
            {"ILocationConfigurator", "SecondaryBand", "Set"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestSecondaryBandConfig (ServerContext* context,
    const google::protobuf::Empty* request, locStub::RequestSecondaryBandConfigReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", "requestSecondaryBandConfig", status,
        errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    std::string str = CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "8",
        {"ILocationConfigurator", "SecondaryBand", "Set"});
    for(size_t itr = 0; itr < str.size(); itr++) {
        if(str[itr] >= '0' && str[itr] <= '8') {
            int constel = (str[itr] - 48);
            response->add_constellation_set(static_cast<::locStub::GnssConstellationType>(constel));
        }
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::DeleteAidingData (ServerContext* context,
    const locStub::DeleteAidingDataRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t aidingDataMask = request->aiding_data_mask();
    apiJsonReader("deleteAidingData", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(aidingDataMask),
                {"ILocationConfigurator", "DeleteAidingData", "aidingDataMask"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureDR (ServerContext* context,
    const locStub::ConfigureDRRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureDR", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().speed_factor())),
                {"ILocationConfigurator", "configureDR", "speedFactor"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().speed_factor_unc())),
                {"ILocationConfigurator", "configureDR", "speedFactorUnc"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().gyro_factor())),
                {"ILocationConfigurator", "configureDR", "gyroFactor"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().gyro_factor_unc())),
                {"ILocationConfigurator", "configureDR", "gyroFactorUnc"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().roll_offset())),
                {"ILocationConfigurator", "configureDR", "rollOffset"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().yaw_offset())),
                {"ILocationConfigurator", "configureDR", "yawOffset"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().pitch_offset())),
                {"ILocationConfigurator", "configureDR", "pitchOffset"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().offset_unc())),
                {"ILocationConfigurator", "configureDR", "offsetUnc"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->config().valid_mask())),
                {"ILocationConfigurator", "configureDR", "validity"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureEngineState (ServerContext* context,
    const locStub::ConfigureEngineStateRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t engineType = request->engine_type();
    uint16_t engineState = request->engine_state();
    apiJsonReader("configureEngineState", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(engineType)),
                {"ILocationConfigurator", "EngineState", "engineType"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(engineState)),
                {"ILocationConfigurator", "EngineState", "engineState"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ProvideConsentForTerrestrialPositioning (
    ServerContext* context, const locStub::ProvideConsentForTerrestrialPositioningRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    bool consent = request->user_consent();
    apiJsonReader("provideConsentForTerrestrialPositioning", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(consent),
                {"ILocationConfigurator", "ConsentForTerrestrialPositioning", "Consent"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureNmeaTypes (ServerContext* context,
    const locStub::ConfigureNmeaTypesRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t nmeaType = request->nmea_type();
    apiJsonReader("configureNmeaTypes", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(nmeaType),
            {"ILocationConfigurator", "configureNmeaTypes", "sentenceConfig"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureNmea(ServerContext* context,
    const locStub::ConfigureNmeaRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t nmeaType = request->nmea_type();
    uint16_t engineType = request->engine_type();
    uint16_t datumType;
    if(request->datum_type() == ::locStub::DatumType::WGS_84) {
        datumType = 0;
    } else if(request->datum_type() == ::locStub::DatumType::PZ_90) {
        datumType = 1;
    }
    apiJsonReader("configureNmea", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(nmeaType), {"ILocationConfigurator", "configureNmea", "sentenceConfig"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(datumType), {"ILocationConfigurator", "configureNmea", "datumType"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(engineType), {"ILocationConfigurator", "configureNmea", "engineType"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureEngineIntegrityRisk (ServerContext* context,
    const locStub::ConfigureEngineIntegrityRiskRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t integRisk = request->integrity_risk();
    uint16_t engineType = request->engine_type();
    apiJsonReader("configureEngineIntegrityRisk", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(engineType),
            {"ILocationConfigurator", "configureEngineIntegrityRisk", "engineType"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(integRisk),
            {"ILocationConfigurator", "configureEngineIntegrityRisk", "integrityRisk"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureXtraParams (ServerContext* context,
    const locStub::ConfigureXtraParamsRequest* request,
        locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureXtraParams", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->enable())),
                {"ILocationConfigurator", "XtraParams", "enable"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_interval_minute())),
                {"ILocationConfigurator", "XtraParams", "downloadIntervalMinute"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_timeout_sec())),
                {"ILocationConfigurator", "XtraParams", "downloadTimeoutSec"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_retry_interval_minute())),
                {"ILocationConfigurator", "XtraParams", "downloadRetryIntervalMinute"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_retry_attempts())),
                {"ILocationConfigurator", "XtraParams", "downloadRetryAttempts"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            request->ca_path(), {"ILocationConfigurator", "XtraParams", "caPath"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->integrity_download_enabled())),
                {"ILocationConfigurator", "XtraParams", "isIntegrityDownloadEnabled"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->integrity_download_interval_minute())),
                {"ILocationConfigurator", "XtraParams", "integrityDownloadIntervalMinute"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->daemon_debug_log_level())),
                {"ILocationConfigurator", "XtraParams", "daemonDebugLogLevel"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", request->server_urls(),
            {"ILocationConfigurator", "XtraParams", "serverURLs"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", request->ntp_server_urls(),
            {"ILocationConfigurator", "XtraParams", "ntpServerURLs"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", request->nts_server_url(),
            {"ILocationConfigurator", "XtraParams", "ntsServerURL"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->diag_logging_enabled())),
                {"ILocationConfigurator", "XtraParams", "diagLoggingEnabled"});
    }
    if(xtraEnabled_!= request->enable()){
    auto f = std::async(std::launch::async, [this](){
        this->triggerXtraStatusEvent();
    }).share();
    taskQ_->add(f);
    xtraEnabled_ = request->enable();
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestXtraStatus (ServerContext* context,
    const google::protobuf::Empty* request,
        locStub::RequestXtraStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", "requestXtraStatus", status,
        errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    response->mutable_xtra_status()->set_feature_enabled(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "XtraParams", "enable"})));
    response->mutable_xtra_status()->set_xtra_valid_for_hours(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "XtraParams", "xtraValidForHours"})));
    if(xtraConsent_) {
        response->mutable_xtra_status()->set_xtra_data_status(
            std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "XtraParams", "xtraDataStatus"})));
    } else {
        response->mutable_xtra_status()->set_xtra_data_status(0);
    }
    response->mutable_xtra_status()->set_consent(xtraConsent_);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::InjectMerkleTree (ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("injectMerkleTreeInformation", response);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureOsnma (ServerContext* context,
    const locStub::ConfigureOsnmaRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureOsnma", response);
    bool enable = request->enable();
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enable),
            {"ILocationConfigurator", "configureOsnma", "enable"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ProvideXtraConsent (ServerContext* context,
    const locStub::XtraConsentRequest* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("provideConsentForXtra", response);
    bool consent = request->consent();
    xtraConsent_ = consent;
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(consent),
            {"ILocationConfigurator", "provideXtraConsent", "consent"});
    }
    return grpc::Status::OK;
}

void LocationConfiguratorServerImpl::apiJsonReader(
    std::string apiName, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", apiName, status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}
