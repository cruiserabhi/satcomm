/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DeviceInfoManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"

#define DEVICE_INFO_MANAGER_API_JSON "api/platform/IDeviceInfoManager.json"
#define DEVICE_INFO_MANAGER_SYSTEM_INFO_JSON "system-info/platform/IDeviceInfoManager.json"
#define META_BUILD_VER_INFO_FILE "system-info/platform/version_info.json"
#define DEVICEINFO_MANAGER_FILTER "deviceinfo_manager"
#define SUBSYSTEM_MANAGER_FILTER "subsystem_manager"
#define DEFAULT_DELIMITER " "

DeviceInfoManagerServerImpl::DeviceInfoManagerServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

DeviceInfoManagerServerImpl::~DeviceInfoManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
}

telux::common::Status DeviceInfoManagerServerImpl::registerDefaultIndications() {
    LOG(DEBUG, __FUNCTION__);

    auto status = serverEvent_.registerListener(shared_from_this(), DEVICEINFO_MANAGER_FILTER);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }

    status = serverEvent_.registerListener(shared_from_this(), SUBSYSTEM_MANAGER_FILTER);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Registering subsystem status indications failed");
        return status;
    }

    return status;
}

void DeviceInfoManagerServerImpl::notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr) {
    LOG(DEBUG, __FUNCTION__, ":: Service status Changed to ", srvStatusStr);
    onSSREvent(srvStatus);
}

telux::common::ServiceStatus DeviceInfoManagerServerImpl::getServiceStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void DeviceInfoManagerServerImpl::setServiceStatus(telux::common::ServiceStatus srvStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serviceStatus_ != srvStatus) {
        serviceStatus_ = srvStatus;
        std::string srvStrStatus = CommonUtils::mapServiceString(srvStatus);
        notifyServiceStateChanged(serviceStatus_, srvStrStatus);
    }
}

void DeviceInfoManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    if (event.filter() == DEVICEINFO_MANAGER_FILTER) {
        onDeviceInfoEventUpdate(event.event());
    } else if (event.filter() == SUBSYSTEM_MANAGER_FILTER) {
        onSubsystemEventUpdate(event.event());
    }
}

/* Get Notification for SSR*/
void DeviceInfoManagerServerImpl::onDeviceInfoEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The deviceinfo manager event: ", event);
    std::string token;

    /** INPUT-event:
     * ssr SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     * OUTPUT-token:
     * ssr
     **/
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    /** INPUT-token:
     * ssr
     * INPUT-event:
     * SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     **/
    handleEvent(token, event);
}

void DeviceInfoManagerServerImpl::onSubsystemEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The Subsystem manager event: ", event);
    std::string token;

    /** INPUT-event:
     * operational_status SUBSYSTEM PROC_TYPE STATUS
     * OUTPUT-token:
     * operational_status
     **/
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    /** INPUT-token:
     * operational_status
     * INPUT-event:
     * SUBSYSTEM PROC_TYPE STATUS
     **/
    handleEvent(token, event);
}

/** INPUT-token:
  * ssr
  * operational_status
  * INPUT-event:
  * SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
  * SUBSYSTEM PROC_TYPE STATUS
 */
void DeviceInfoManagerServerImpl::handleEvent(std::string token,std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The deviceinfo event type is: ", token,
            "The leftover string is: ", event);

    if (token == "ssr") {
        //INPUT-token: ssr
        //INPUT-event: SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
        handleSSREvent(event);
    } else if (token == "operational_status") {
        //INPUT-event: SUBSYSTEM PROC_TYPE STATUS
        handleOperationalStatusEvent(event);
    } else {
        LOG(DEBUG, __FUNCTION__, ":: Invalid event ! Ignoring token: ",
                token, ", event: ", event);
    }
}

bool DeviceInfoManagerServerImpl::isValidProcType(int procType) {
    return (procType == static_cast<int>(ProcType::LOCAL_PROC)) ||
            (procType == static_cast<int>(ProcType::REMOTE_PROC));
}

bool DeviceInfoManagerServerImpl::isValidSubsystem(int subsystem) {
    return (subsystem == static_cast<int>(Subsystem::NONE)) ||
            (subsystem == static_cast<int>(Subsystem::APSS)) ||
            (subsystem == static_cast<int>(Subsystem::MPSS));
}

void DeviceInfoManagerServerImpl::handleOperationalStatusEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: operational_status event: ", eventParams);

    std::istringstream iss(eventParams);
    int subsystem, procType;
    std::string operationalStatus;

    if (!(iss >> subsystem >> procType >> operationalStatus)) {
        LOG(DEBUG, __FUNCTION__, "Invalid input: ",eventParams);
        return;
    }

    if (!(isValidSubsystem(subsystem) && isValidProcType(procType))) {
        LOG(DEBUG, __FUNCTION__, "Invalid subsystem/procType: ", subsystem, procType);
        return;
    }

    commonStub::OperationalStatus opStatus =
        commonStub::OperationalStatus::NONOPERATIONAL;
    if (operationalStatus == "OPERATIONAL") {
        opStatus = commonStub::OperationalStatus::OPERATIONAL;
    } else if (operationalStatus == "NONOPERATIONAL") {
        opStatus = commonStub::OperationalStatus::NONOPERATIONAL;
    } else {
        LOG(DEBUG, __FUNCTION__, ":: INVALID operational status: ", operationalStatus);
        return;
    }

    onSubsystemEvent(subsystem, procType, opStatus);
}

void DeviceInfoManagerServerImpl::onSubsystemEvent(int subsystem, int procType,
    commonStub::OperationalStatus opStatus) {
    LOG(DEBUG,__FUNCTION__);

    ::platformStub::SubsystemStatusreply subsystemResp;
    ::eventService::EventResponse anyResponse;

    subsystemResp.set_status(opStatus);
    subsystemResp.set_subsystem(subsystem);
    subsystemResp.set_proc_type(procType);

    anyResponse.set_filter(SUBSYSTEM_MANAGER_FILTER);
    anyResponse.mutable_any()->PackFrom(subsystemResp);
    clientEvent_.updateEventQueue(anyResponse);
}

void DeviceInfoManagerServerImpl::handleSSREvent(std::string eventParams) {
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

grpc::Status DeviceInfoManagerServerImpl::setResponse(telux::common::ServiceStatus srvStatus,
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

void DeviceInfoManagerServerImpl::onSSREvent(telux::common::ServiceStatus srvStatus) {
    LOG(DEBUG,__FUNCTION__);

    commonStub::GetServiceStatusReply ssrResp;
    ::eventService::EventResponse anyResponse;

    setResponse(srvStatus, &ssrResp);

    anyResponse.set_filter(DEVICEINFO_MANAGER_FILTER);
    anyResponse.mutable_any()->PackFrom(ssrResp);
    clientEvent_.updateEventQueue(anyResponse);
}

grpc::Status DeviceInfoManagerServerImpl::InitService(ServerContext* context,
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
        = JsonParser::readFromJsonFile(rootNode, DEVICE_INFO_MANAGER_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        std::lock_guard<std::mutex> lock(mutex_);
        cbDelay_ = rootNode["IDeviceInfoManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["IDeviceInfoManager"]["IsSubsystemReady"].asString();
        srvStatus = CommonUtils::mapServiceStatus(cbStatus);
    } else {
        LOG(ERROR, "Unable to read DeviceInfoManager JSON");
    }

    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));
    setServiceStatus(srvStatus);
    response->set_delay(cbDelay_);

    return setResponse(srvStatus, response);
}

grpc::Status DeviceInfoManagerServerImpl::GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ServiceStatus srvStatus = getServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));

    return setResponse(srvStatus,response);
}

grpc::Status DeviceInfoManagerServerImpl::GetPlatformVersion(ServerContext* context,
const google::protobuf::Empty* request, platformStub::PlatformVersionInfo* response){
    LOG(DEBUG,__FUNCTION__);

    std::string apiJsonPath = DEVICE_INFO_MANAGER_API_JSON;
    std::string systemInfoJsonPath = META_BUILD_VER_INFO_FILE;
    std::string subsystem = "IDeviceInfoManager";
    std::string method = "GetPlatformVersion";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, systemInfoJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    std::string modem;
    std::string meta_build_id;
    std::string apps_fsl;
    std::string apps;

    if (data.status == telux::common::Status::SUCCESS) {
        modem = data.stateRootObj["Image_Build_IDs"]["modem"].asString();
        meta_build_id = data.stateRootObj["Metabuild_Info"]["Meta_Build_ID"].asString();
        apps_fsl = data.stateRootObj["Image_Build_IDs"]["apps_fsl"].asString();
        apps = data.stateRootObj["Image_Build_IDs"]["apps"].asString();
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_modem_details(modem);
    response->set_meta_details(meta_build_id);
    response->set_external_app(apps_fsl);
    response->set_integrated_app(apps);

    return grpc::Status::OK;
}

grpc::Status DeviceInfoManagerServerImpl::GetIMEI(ServerContext* context,
const google::protobuf::Empty* request, platformStub::PlatformImeiInfo* response){
    LOG(DEBUG,__FUNCTION__);

    std::string apiJsonPath = DEVICE_INFO_MANAGER_API_JSON;
    std::string systemInfoJsonPath = DEVICE_INFO_MANAGER_SYSTEM_INFO_JSON;
    std::string subsystem = "IDeviceInfoManager";
    std::string method = "GetIMEI";
    std::string imei;
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, systemInfoJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        imei = data.stateRootObj["IDeviceInfoManager"]["GetIMEI"]["imei"].asString();
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_imei_info(imei);

    return grpc::Status::OK;
}