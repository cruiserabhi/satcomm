/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "FsManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/EventService.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#define FS_MANAGER_API_JSON "api/platform/IFsManager.json"
#define FS_EVENT_INFO_JSON "system-state/platform/FsManagerState.json"
#define FS_MANAGER_FILTER "fs_manager"
#define DEFAULT_DELIMITER " "

FsManagerServerImpl::FsManagerServerImpl()
   : otaSession_(false)
   , abSyncState_(false)
   , serverEvent_(ServerEventManager::getInstance())
   , clientEvent_(EventService::getInstance()){
    LOG(DEBUG, __FUNCTION__);
}

FsManagerServerImpl::~FsManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
}


telux::common::Status FsManagerServerImpl::registerDefaultIndications() {
    LOG(DEBUG, __FUNCTION__);

    auto status = serverEvent_.registerListener(shared_from_this(), FS_MANAGER_FILTER);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Registering default indications failed");
        return status;
    }

    return status;
}

void FsManagerServerImpl::notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr) {
    LOG(DEBUG, __FUNCTION__, ":: Service status Changed to ", srvStatusStr);

    onSSREvent(srvStatus);
}

telux::common::ServiceStatus FsManagerServerImpl::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void FsManagerServerImpl::setServiceStatus(telux::common::ServiceStatus srvStatus) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lock(mutex_);
    if (serviceStatus_ != srvStatus) {
        serviceStatus_ = srvStatus;
        std::string srvStrStatus = CommonUtils::mapServiceString(srvStatus);
        notifyServiceStateChanged(serviceStatus_, srvStrStatus);
    }
}

void FsManagerServerImpl::checkRebootDuringOTA() {
    LOG(DEBUG, __FUNCTION__);

    if (fsEventsMap_["MRC_OTA_START"]) {
        updateFsStateMachine("MRC_OTA_START", false);
        updateFsStateMachine("MRC_OTA_RESUME", true);
        otaSession_ = true;
    }
}

void FsManagerServerImpl::updateSystemStateJson() {
    LOG(DEBUG, __FUNCTION__);

    Json::Value root;
    Json::Value events(Json::objectValue);

    for (const auto& event : fsEventsMap_) {
        events[event.first] = event.second;
    }

    root["fsEventsState"] = events;

    JsonParser::writeToJsonFile(root, FS_EVENT_INFO_JSON);
}

grpc::Status FsManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG,__FUNCTION__);

    telux::common::Status status = telux::common::Status::SUCCESS;
    telux::common::ServiceStatus srvStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;

    status = registerDefaultIndications();
    if (status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::CANCELLED,
                ":: Could not register indication with EventMgr");
    }

    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, FS_MANAGER_API_JSON);

    if (errorCode == ErrorCode::SUCCESS) {
        std::lock_guard<std::mutex> lock(mutex_);
        cbDelay_ = rootNode["IFsManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["IFsManager"]["IsSubsystemReady"].asString();
        srvStatus = CommonUtils::mapServiceStatus(cbStatus);
        try{
            errorCode = JsonParser::readFromJsonFile(rootNode, FS_EVENT_INFO_JSON);
            if (errorCode == ErrorCode::SUCCESS) {
                // Iterate through the JSON object
                const Json::Value events = rootNode["fsEventsState"];
                std::lock_guard<std::mutex> lock(eventMutex_);
                for (Json::Value::const_iterator it = events.begin(); it != events.end(); ++it) {
                    std::string eventName = it.key().asString();
                    bool state = it->asBool();
                    fsEventsMap_[eventName] = state;
                }
                checkRebootDuringOTA();
            } else {
                LOG(ERROR, "Unable to read FS_EVENT_INFO_JSON JSON");
                srvStatus = telux::common::ServiceStatus::SERVICE_FAILED;
            }
        } catch(std::exception const & ex) {
            LOG(DEBUG, "Exception Occur ", ex.what());
            srvStatus = telux::common::ServiceStatus::SERVICE_FAILED;
        }
    } else {
        LOG(ERROR, "Unable to read FsManager JSON");
    }

    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));
    setServiceStatus(srvStatus);
    response->set_delay(cbDelay_);

    return setResponse(srvStatus,response);
}

grpc::Status FsManagerServerImpl::GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ServiceStatus srvStatus = getServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));

    return setResponse(srvStatus,response);
}

void FsManagerServerImpl::updateFsStateMachine(std::string eventName, bool state) {
    LOG(DEBUG,__FUNCTION__);

    fsEventsMap_[eventName] = state;
}

void FsManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    LOG(DEBUG,__FUNCTION__);

    if (event.filter() == FS_MANAGER_FILTER) {
        onEventUpdate(event.event());
    }
}

void FsManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The FS event: ", event);

    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token,event);
}

void FsManagerServerImpl::handleEvent(std::string token, std::string event) {
    LOG(DEBUG, __FUNCTION__, " The FS event type is: ", token);
    LOG(DEBUG, __FUNCTION__, " The leftover string is: ", event);

    if (token == "otaAbSync") {
        handleOtaAbSyncEvent(event);
    } else if (token == "efsBackup") {
        handleEfsBackup(event);
    } else if (token == "efsRestore") {
        handleEfsRestore(event);
    } else if (token == "fsImminent") {
        handleFsOpImminentEvent(event);
    } else if (token == "ssr") {
        //INPUT-token: ssr
        //INPUT-event: SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
        handleSSREvent(event);
    } else {
        LOG(DEBUG, __FUNCTION__, ":: Invalid event ! Ignoring token: ",
                token, ", event: ", event);
    }
}

void FsManagerServerImpl::handleSSREvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: SSR event: ", eventParams);

    telux::common::ServiceStatus srvcStatus =
        telux::common::ServiceStatus::SERVICE_FAILED;
    if (eventParams == "SERVICE_AVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    } else if (eventParams == "SERVICE_UNAVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (eventParams == "SERVICE_FAILED") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(DEBUG, __FUNCTION__, ":: INVALID SSR event: ", eventParams);
        return;
    }

    setServiceStatus(srvcStatus);
}

grpc::Status FsManagerServerImpl::setResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG,__FUNCTION__);

    switch(srvStatus) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
            break;
        default:
            LOG(ERROR, __FUNCTION__, ":: Invalid service status");
            return grpc::Status(grpc::StatusCode::CANCELLED, ":: set service status failed");
    }

    response->set_delay(cbDelay_);

    return grpc::Status::OK;
}

void FsManagerServerImpl::onSSREvent(telux::common::ServiceStatus srvStatus) {
    LOG(DEBUG,__FUNCTION__);

    commonStub::GetServiceStatusReply ssrResp;
    ::eventService::EventResponse anyResponse;

    setResponse(srvStatus, &ssrResp);

    anyResponse.set_filter(FS_MANAGER_FILTER);
    anyResponse.mutable_any()->PackFrom(ssrResp);
    clientEvent_.updateEventQueue(anyResponse);
}

grpc::Status FsManagerServerImpl::setSrvcResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootNode;

    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, FS_MANAGER_API_JSON);

    if (errorCode == ErrorCode::SUCCESS) {
        int subSysDelay = rootNode["IFsManager"]["IsSubsystemReadyDelay"].asInt();

        switch(srvStatus) {
            case telux::common::ServiceStatus::SERVICE_AVAILABLE:
                response->set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
                break;
            case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
                response->set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
                break;
            case telux::common::ServiceStatus::SERVICE_FAILED:
                response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
                break;
            default:
                LOG(ERROR, __FUNCTION__, ":: Invalid service status");
                return grpc::Status(grpc::StatusCode::CANCELLED, ":: set service status failed");
        }
        response->set_delay(subSysDelay);
    } else {
        LOG(ERROR, "Unable to read FsManager JSON");
        response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
    }
    return grpc::Status::OK;
}

void FsManagerServerImpl::handleEfsBackup(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: EfsBackup event: ", eventParams);

    std::string eventName;
    std::string errorCode;

    try {
        eventName = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        errorCode = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    } catch (exception const & ex) {
        LOG(ERROR, __FUNCTION__, ":: Exception Occured: ", ex.what());
        return;
    }

    if(!isValidKey(eventName))
    {
        LOG(ERROR, __FUNCTION__, "Invlaid Event Name", eventName);
        return;
    }


    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (fsEventsMap_[eventName] != true) {
            LOG(DEBUG, eventName, " is not Valid");
            return;
        }

    }

    platformStub::DefaultReply response;
    response.set_error(static_cast<::commonStub::ErrorCode>(CommonUtils::mapErrorCode(errorCode)));

    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (eventName == "EFS_BACKUP_START" && errorCode == "SUCCESS") {
            updateFsStateMachine("EFS_BACKUP_START", false);
            updateFsStateMachine("EFS_BACKUP_END", true);
        } else if (eventName == "EFS_BACKUP_START" && errorCode == "FAILURE") {
            updateFsStateMachine("EFS_BACKUP_START", false);
        } else if (eventName == "EFS_BACKUP_END" && errorCode == "SUCCESS") {
            updateFsStateMachine("EFS_BACKUP_END", false);
        } else if (eventName == "EFS_BACKUP_END" && errorCode == "FAILURE") {
            updateFsStateMachine("EFS_BACKUP_END", false);
        } else {
            LOG(ERROR, "Invalid eventName or errorCode", eventName, errorCode);
            return;
        }
    }

    triggerFsEvent(eventName,&response);
}

void FsManagerServerImpl::handleEfsRestore(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: EfsRestore event: ", eventParams);

    std::string eventName;
    std::string errorCode;

    try {
        eventName = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        errorCode = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    } catch (exception const & ex) {
        LOG(ERROR, __FUNCTION__, ":: Exception Occured: ", ex.what());
        return;
    }

    if(!isValidKey(eventName))
    {
        LOG(ERROR, __FUNCTION__, "Invlaid Event Name", eventName);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (fsEventsMap_[eventName] != true) {
            LOG(DEBUG, eventName, " is not Valid");
            return;
        }

    }

    platformStub::DefaultReply response;
    response.set_error(static_cast<::commonStub::ErrorCode>(CommonUtils::mapErrorCode(errorCode)));

    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (eventName == "EFS_RESTORE_START" && errorCode == "SUCCESS") {
            updateFsStateMachine("EFS_RESTORE_START", false);
            updateFsStateMachine("EFS_RESTORE_END", true);
        } else if (eventName == "EFS_RESTORE_START" && errorCode == "FAILURE") {
            updateFsStateMachine("EFS_RESTORE_START", true);
        } else if (eventName == "EFS_RESTORE_END" && errorCode == "SUCCESS") {
            updateFsStateMachine("EFS_RESTORE_END", false);
            updateFsStateMachine("EFS_RESTORE_START", true);
        } else if (eventName == "EFS_RESTORE_END" && errorCode == "FAILURE") {
            updateFsStateMachine("EFS_RESTORE_END", false);
            updateFsStateMachine("EFS_RESTORE_START", true);
        } else {
            LOG(ERROR, "Invalid eventName or errorCode", eventName, errorCode);
            return;
        }
    }

    triggerFsEvent(eventName,&response);
}

void FsManagerServerImpl::handleFsOpImminentEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);

    std::string eventName;
    std::string token;
    uint32_t timeToExpiry;

    try {
        eventName = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if(token == "") {
            LOG(DEBUG, __FUNCTION__, " The timeToExpiry is not passed");
            return;
        } else {
            if (token.find('-') != std::string::npos) {
                LOG(ERROR, __FUNCTION__, " Negative numbers are not allowed.");
                return;
            }
            try {
                // Convert to uint32_t
                timeToExpiry = static_cast<uint32_t>(std::stoull(token));
            } catch (const std::invalid_argument& e) {
                LOG(ERROR, __FUNCTION__, " Invalid input: not a valid number.");
                return;
            } catch (const std::out_of_range& e) {
                LOG(ERROR, __FUNCTION__, " timeToExpiry out of range for uint32_t.");
                return;
            }
        }
    } catch (exception const & ex) {
        LOG(ERROR, __FUNCTION__, ":: Exception Occured: ", ex.what());
        return;
    }

    if (eventName != "FS_OPERATION_IMMINENT") {
        LOG(DEBUG, eventName, " event is invalid");
        return;
    }

    platformStub::DefaultReply response;
    response.set_delay(timeToExpiry);

    triggerFsEvent(eventName,&response);
}

bool FsManagerServerImpl::isValidKey(const std::string& key) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lock(eventMutex_);
    return fsEventsMap_.find(key) != fsEventsMap_.end();
}

void FsManagerServerImpl::handleOtaAbSyncEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: otaAbSync event: ", eventParams);

    std::string eventName;
    std::string errorCode;

    try {
        eventName = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        errorCode = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    } catch (exception const & ex) {
        LOG(ERROR, __FUNCTION__, ":: Exception Occured: ", ex.what());
        return;
    }

    if(!isValidKey(eventName))
    {
        LOG(ERROR, __FUNCTION__, "Invlaid Event Name", eventName);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (fsEventsMap_[eventName] != true) {
            LOG(DEBUG, eventName, " is not Valid");
            return;
        }

    }

    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (eventName == "MRC_OTA_START" && errorCode == "SUCCESS") {
            updateFsStateMachine("MRC_OTA_START", false);
            updateSystemStateJson();
        } else if (eventName == "MRC_OTA_START" && errorCode == "FAILURE") {
            updateFsStateMachine("MRC_OTA_START", false);
            updateSystemStateJson();
            otaSession_ = false;
        } else if (eventName == "MRC_OTA_RESUME" && errorCode == "SUCCESS") {
            updateFsStateMachine("MRC_OTA_RESUME", false);
            updateSystemStateJson();
        } else if (eventName == "MRC_OTA_RESUME" && errorCode == "FAILURE") {
            updateFsStateMachine("MRC_OTA_RESUME", false);
            updateSystemStateJson();
            otaSession_ = false;
        } else if (eventName == "MRC_OTA_END" && errorCode == "SUCCESS") {
            updateFsStateMachine("MRC_OTA_END", false);
            abSyncState_ = true;
        } else if (eventName == "MRC_OTA_END" && errorCode == "FAILURE") {
            updateFsStateMachine("MRC_OTA_END", false);
            otaSession_ = false;
        } else if (eventName == "MRC_ABSYNC" && errorCode == "SUCCESS") {
            updateFsStateMachine("MRC_ABSYNC", false);
            abSyncState_ = false;
            otaSession_ = false;
        } else if (eventName == "MRC_ABSYNC" && errorCode == "FAILURE") {
            updateFsStateMachine("MRC_ABSYNC", false);
            abSyncState_ = false;
            otaSession_ = false;
        } else {
            LOG(ERROR, "Invalid eventName or errorCode", eventName, errorCode);
            return;
        }
    }

    platformStub::DefaultReply response;
    response.set_error(static_cast<::commonStub::ErrorCode>(CommonUtils::mapErrorCode(errorCode)));

    triggerFsEvent(eventName,&response);
}

void FsManagerServerImpl::updateFsEventReply(platformStub::DefaultReply* source,
    platformStub::DefaultReply* destination) {
    LOG(DEBUG, __FUNCTION__);

    destination->set_error(source->error());
    destination->set_status(source->status());
    destination->set_delay(source->delay());
}

void FsManagerServerImpl::triggerFsEvent(std::string fsEventName,
    platformStub::DefaultReply* response){
    LOG(DEBUG, __FUNCTION__);

    ::platformStub::FsEventReply fsEvent;
    ::eventService::EventResponse anyResponse;

    updateFsEventReply(response, fsEvent.mutable_reply());
    fsEvent.mutable_fs_event_name()->set_fs_event_name(fsEventName);

    anyResponse.set_filter(FS_MANAGER_FILTER);
    anyResponse.mutable_any()->PackFrom(fsEvent);
    clientEvent_.updateEventQueue(anyResponse);
}

grpc::Status FsManagerServerImpl::StartEfsBackup(ServerContext* context,
    const google::protobuf::Empty* request, platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    apiJsonReader("startEfsBackup", response);
    std::lock_guard<std::mutex> lock(eventMutex_);
    updateFsStateMachine("EFS_BACKUP_START", true);

    return grpc::Status::OK;
}

grpc::Status FsManagerServerImpl::PrepareForEcall(ServerContext* context,
    const google::protobuf::Empty* request, platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    apiJsonReader("prepareForEcall", response);
    return grpc::Status::OK;
}

grpc::Status FsManagerServerImpl::ECallCompleted(ServerContext* context,
    const google::protobuf::Empty* request, platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    apiJsonReader("eCallCompleted", response);
    return grpc::Status::OK;
}

grpc::Status FsManagerServerImpl::PrepareForOta(ServerContext* context,
    const ::platformStub::FsEventName* request,
    platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    std::string fsEventName;
    fsEventName = request->fs_event_name();
    apiJsonReader("prepareForOta", response);

    std::lock_guard<std::mutex> lock(eventMutex_);
    if (fsEventName == "MRC_OTA_START" && !otaSession_) {
        fsEventsMap_["MRC_OTA_START"] = true;
        otaSession_ = true;
        updateSystemStateJson();
        response->set_status(static_cast<::commonStub::Status>(CommonUtils::mapStatus("SUCCESS")));
    // The OTA RESUME is allowed in case of below scenarios:
    // 1. When OTA START is true and reboot happens, OTA RESUME and otaSession_ will be set to true
    // 2. When there is no otaSession_ in progress.
    } else if ((fsEventName == "MRC_OTA_RESUME") &&
        ((otaSession_ && fsEventsMap_[fsEventName]) || (!otaSession_))) {
        fsEventsMap_["MRC_OTA_RESUME"] = true;
        otaSession_ = true;
        response->set_status(static_cast<::commonStub::Status>(CommonUtils::mapStatus("SUCCESS")));
    } else {
        response->set_status(static_cast<::commonStub::Status>(CommonUtils::mapStatus("FAILED")));
    }

    return grpc::Status::OK;
}

grpc::Status FsManagerServerImpl::OtaCompleted(ServerContext* context,
    const google::protobuf::Empty* request,
    platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    apiJsonReader("otaCompleted", response);

    std::lock_guard<std::mutex> lock(eventMutex_);
    if (otaSession_ && !fsEventsMap_["MRC_OTA_START"] &&
            !fsEventsMap_["MRC_OTA_RESUME"] && !abSyncState_) {
        fsEventsMap_["MRC_OTA_END"] = true;
    } else {
        response->set_status(static_cast<::commonStub::Status>(CommonUtils::mapStatus("FAILED")));
    }

    return grpc::Status::OK;
}

grpc::Status FsManagerServerImpl::StartAbSync(ServerContext* context,
const google::protobuf::Empty* request, platformStub::DefaultReply* response) {
    LOG(DEBUG,__FUNCTION__);

    apiJsonReader("startAbSync", response);

    std::lock_guard<std::mutex> lock(eventMutex_);
    if (otaSession_ && !abSyncState_) {
        response->set_status(static_cast<::commonStub::Status>(CommonUtils::mapStatus("FAILED")));
    } else {
        fsEventsMap_["MRC_ABSYNC"] = true;
    }

    return grpc::Status::OK;
}

void FsManagerServerImpl::apiJsonReader(
    std::string apiName, platformStub::DefaultReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, FS_MANAGER_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;

    CommonUtils::getValues(rootNode, "IFsManager", apiName, status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}