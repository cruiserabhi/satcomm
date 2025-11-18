/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SuppServicesManagerServerImpl.hpp"

#include "libs/common/CommonUtils.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/SuppServicesManager.hpp>

#define JSON_PATH1 "api/tel/ISuppServicesManagerSlot1.json"
#define JSON_PATH2 "api/tel/ISuppServicesManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/ISuppServicesManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/ISuppServicesManagerStateSlot2.json"
#define TEL_SUPP_SERVICES_MANAGER "ISuppServicesManager"

SuppServicesManagerServerImpl::SuppServicesManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status SuppServicesManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[TEL_SUPP_SERVICES_MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[TEL_SUPP_SERVICES_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay:: ", cbDelay, " cbStatus:: ", cbStatus);
    if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Json not found or service not available or failed");
        return grpc::Status(grpc::StatusCode::INTERNAL,
            " Json not found or service not available or failed");
     }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[TEL_SUPP_SERVICES_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::SetCallWaitingPref(ServerContext* context,
    const ::telStub::SetCallWaitingPrefRequest* request,
    telStub::SetCallWaitingPrefReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string apiJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = TEL_SUPP_SERVICES_MANAGER;
    std::string method = "setCallWaitingPref";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        ::telStub::SuppServicesStatus::Status suppServicesStatus
             = request->supp_services_status();
        data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallWaitingPref"]["SuppServicesStatus"]
            = static_cast<int>(suppServicesStatus);
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    std::string failureCause = data.apiRootObj[TEL_SUPP_SERVICES_MANAGER]\
        ["failureCause"].asString();
    LOG(DEBUG, __FUNCTION__,  " failureCause : ", failureCause);
    long value = CommonUtils::convertHexToInt(failureCause);

    response->set_failure_cause
       (static_cast<telStub::SuppServicesFailureCause_FailureCause>(value));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::RequestCallWaitingPref(ServerContext* context,
    const ::telStub::RequestCallWaitingPrefRequest* request,
    telStub::RequestCallWaitingPrefReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string apiJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = TEL_SUPP_SERVICES_MANAGER;
    std::string method = "requestCallWaitingPref";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        ::telStub::SuppServicesStatus_Status suppServicesStatus
             = static_cast<::telStub::SuppServicesStatus_Status>
                 (data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]\
                 ["CallWaitingPref"]["SuppServicesStatus"].asInt());
        response->set_supp_services_status(suppServicesStatus);
        std::string failureCause = data.apiRootObj[TEL_SUPP_SERVICES_MANAGER]\
            ["failureCause"].asString();
        LOG(DEBUG, __FUNCTION__, " failureCause : ", failureCause);
        long value = CommonUtils::convertHexToInt(failureCause);
        ::telStub::SuppServicesFailureCause_FailureCause fc
            = static_cast<::telStub::SuppServicesFailureCause_FailureCause>(value);
        response->set_failure_cause(fc);
    }

    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::SetForwardingPref(ServerContext* context,
    const ::telStub::SetForwardingPrefRequest* request,
    telStub::SetForwardingPrefReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string apiJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = TEL_SUPP_SERVICES_MANAGER;
    std::string method = "setForwardingPref";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        // update data based on the reason, if reason already available, update
        // the entry, else add a new entry for ForwardInfo.
        int forwardOperation = request->forward_req().operation();
        int forwardReason = request->forward_req().reason();
        LOG(DEBUG, __FUNCTION__," forwardOperation : ", forwardOperation, " forwardReason : ",
            forwardReason);
        int currentCount = data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]\
            ["CallForwardingPref"]["ForwardInfoList"].size();
        LOG(DEBUG, __FUNCTION__," current configcount is : ", currentCount);
        Json::Value newconfig;
        newconfig["ForwardOperation"] = forwardOperation;
        newconfig["ForwardReason"] = forwardReason;
        for (int idx = 0; idx < request->forward_req().service_class().size(); idx++) {
            newconfig["ServiceClass"][idx]
                = static_cast<int>(request->forward_req().service_class(idx));
        }
        switch(static_cast<telux::tel::ForwardOperation>(forwardOperation)) {
            case telux::tel::ForwardOperation::REGISTER:
                newconfig["Number"] = request->forward_req().number();
                newconfig["SuppServicesStatus"]
                    = static_cast<int>(telux::tel::SuppServicesStatus::ENABLED);
                break;
            case telux::tel::ForwardOperation::ACTIVATE:
                newconfig["SuppServicesStatus"]
                    = static_cast<int>(telux::tel::SuppServicesStatus::ENABLED);
                break;
            case telux::tel::ForwardOperation::DEACTIVATE:
                newconfig["SuppServicesStatus"]
                    = static_cast<int>(telux::tel::SuppServicesStatus::DISABLED);
                break;
            case telux::tel::ForwardOperation::ERASE:
                newconfig["Number"] = "";
                newconfig["SuppServicesStatus"]
                    = static_cast<int>(telux::tel::SuppServicesStatus::DISABLED);
                break;
            case telux::tel::ForwardOperation::UNKNOWN:
            default:
                LOG(ERROR, __FUNCTION__, " Invalid forward operation");
                newconfig["SuppServicesStatus"]
                    = static_cast<int>(telux::tel::SuppServicesStatus::DISABLED);
                break;
        }
        newconfig["NoReplyTimer"] = request->forward_req().no_reply_timer();

        bool reasonFound = false;
        for (int j = 0; j < currentCount; j++) {
            // check the forward reason stored and user requested reason and update it.
            if ((data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallForwardingPref"]\
                ["ForwardInfoList"][j]["ForwardReason"]) == newconfig["ForwardReason"]) {
                LOG(DEBUG, __FUNCTION__, " Matched Reason");
                if (newconfig["Number"].isNull()) {
                    newconfig["Number"] = data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]\
                    ["CallForwardingPref"]["ForwardInfoList"][j]["Number"].asString();
                }
                /* user will provide number only when they select "REGISTER" option,
                   for option "ACTIVATE" or "DEACTIVATE", first check the number from json,
                   if number is available, then allow for activation or deactivation else
                   send an error.*/
                LOG(DEBUG, __FUNCTION__, " Number stored or provided: ", newconfig["Number"],
                    " ForwardOperation : ", newconfig["ForwardOperation"]);
                if (newconfig["Number"] == "" &&
                    (newconfig["ForwardOperation"] == 1 || newconfig["ForwardOperation"] == 2)) {
                    LOG(ERROR, __FUNCTION__, " Before activating/deactivating supplementary"
                       " services register it first.");
                    newconfig["SuppServicesStatus"]
                        = static_cast<int>(telux::tel::SuppServicesStatus::DISABLED);
                    data.error = telux::common::ErrorCode::SUPS_FAILURE_CAUSE;
                }
                data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallForwardingPref"]\
                    ["ForwardInfoList"][j] = newconfig;
                reasonFound = true;
                break;
            }
        }

        if (!reasonFound) {
            LOG(DEBUG, __FUNCTION__, " Matching Reason not found");
            data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallForwardingPref"]\
            ["ForwardInfoList"][currentCount++] = newconfig;
        }

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
   }

    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    std::string failureCause = data.apiRootObj[TEL_SUPP_SERVICES_MANAGER]\
        ["failureCause"].asString();
    LOG(DEBUG, __FUNCTION__,  " failureCause : ", failureCause);
    long value = CommonUtils::convertHexToInt(failureCause);
    response->set_failure_cause
       (static_cast<telStub::SuppServicesFailureCause_FailureCause>(value));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::RequestForwardingPref(ServerContext* context,
    const ::telStub::RequestForwardingPrefRequest* request,
    telStub::RequestForwardingPrefReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string apiJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = TEL_SUPP_SERVICES_MANAGER;
    std::string method = "requestForwardingPref";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        int count = data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]\
            ["CallForwardingPref"]["ForwardInfoList"].size();
        // check the forward reason stored and user requested reason and update it.
        int reason = request->forward_reason();
        LOG(DEBUG, __FUNCTION__, " count : ", count, " Reason : ", reason);
        for (int j = 0; j < count; j++) {
            if ((data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallForwardingPref"]\
                ["ForwardInfoList"][j]["ForwardReason"]) == reason) {
                LOG(DEBUG, __FUNCTION__, " Matched Reason");
                telStub::ForwardInfo *result = response->add_forward_info();
                ::telStub::SuppServicesStatus_Status suppServicesStatus
                    = static_cast<telStub::SuppServicesStatus_Status>
                    (data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallForwardingPref"]\
                    ["ForwardInfoList"][j]["SuppServicesStatus"].asInt());
                result->set_status(suppServicesStatus);
                int serviceClassSize = data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]\
                    ["CallForwardingPref"]["ForwardInfoList"][j]["ServiceClass"].size();
                for (int idx = 0; idx < serviceClassSize; idx++) {
                    result->add_service_class(static_cast<telStub::ServiceClassType_Type>(idx));
                }
                string number = data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]\
                    ["CallForwardingPref"]["ForwardInfoList"][j]["Number"].asString();
                result->set_number(number);
                uint8_t noReplyTimer = data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]\
                    ["CallForwardingPref"]["ForwardInfoList"][j]["NoReplyTimer"].asInt();
                result->set_no_reply_timer(noReplyTimer);
            }
        }
        std::string failureCause = data.apiRootObj[TEL_SUPP_SERVICES_MANAGER]\
            ["failureCause"].asString();
        LOG(DEBUG, __FUNCTION__,  " failureCause : ", failureCause);
        long value = CommonUtils::convertHexToInt(failureCause);
        ::telStub::SuppServicesFailureCause_FailureCause fc =
            static_cast<telStub::SuppServicesFailureCause_FailureCause>(value);
        response->set_failure_cause(fc);
    }

    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::SetOirPref(ServerContext* context,
    const ::telStub::SetOirPrefRequest* request,
    telStub::SetOirPrefReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string apiJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = TEL_SUPP_SERVICES_MANAGER;
    std::string method = "setOirPref";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        ::telStub::SuppServicesStatus_Status suppServicesStatus
            = request->supp_services_status();
        data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallOirPref"]["SuppServicesStatus"]
            = static_cast<int>(suppServicesStatus);
        std::vector<uint8_t> serviceClassTypes;
        for (auto sct : request->service_class()) {
            serviceClassTypes.emplace_back(static_cast<uint8_t>(sct));
        }
        std::string value = CommonUtils::convertVectorToString(serviceClassTypes, false);
        data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallOirPref"]["ServiceClass"]
            = value;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    std::string failureCause = data.apiRootObj[TEL_SUPP_SERVICES_MANAGER]\
        ["failureCause"].asString();
    LOG(DEBUG, __FUNCTION__,  " failureCause : ", failureCause);
    long value = CommonUtils::convertHexToInt(failureCause);
    response->set_failure_cause
       (static_cast<telStub::SuppServicesFailureCause_FailureCause>(value));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status SuppServicesManagerServerImpl::RequestOirPref(ServerContext* context,
    const ::telStub::RequestOirPrefRequest* request, telStub::RequestOirPrefReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::vector<uint8_t> serviceClassTypes;
    for (auto sct : request->service_class()) {
        serviceClassTypes.emplace_back(static_cast<uint8_t>(sct));
    }
    std::string value = CommonUtils::convertVectorToString(serviceClassTypes, false);
    std::string apiJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (slotId == DEFAULT_SLOT_ID)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = TEL_SUPP_SERVICES_MANAGER;
    std::string method = "requestOirPref";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        ::telStub::SuppServicesStatus_Status suppServicesStatus
             = static_cast<telStub::SuppServicesStatus_Status>
            (data.stateRootObj[TEL_SUPP_SERVICES_MANAGER]["CallOirPref"]\
            ["SuppServicesStatus"].asInt());
        response->set_supp_services_status(suppServicesStatus);
        ::telStub::SuppSvcProvisionStatus_Status provisionStatus
            = static_cast<telStub::SuppSvcProvisionStatus_Status>
            (data.apiRootObj[TEL_SUPP_SERVICES_MANAGER]["requestOirPref"]\
            ["suppSvcProvisionStatus"].asInt());
        response->set_provision_status(provisionStatus);
        std::string failureCause = data.apiRootObj[TEL_SUPP_SERVICES_MANAGER]\
            ["failureCause"].asString();
        LOG(DEBUG, __FUNCTION__,  " failureCause : ", failureCause);
        long value = CommonUtils::convertHexToInt(failureCause);
        ::telStub::SuppServicesFailureCause_FailureCause fc
            = static_cast<telStub::SuppServicesFailureCause_FailureCause>(value);
        response->set_failure_cause(fc);
    }

    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void SuppServicesManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    LOG(DEBUG, __FUNCTION__);
    std::string event = message.event();
    onEventUpdate(event);
}

void SuppServicesManagerServerImpl::onEventUpdate(std::string event) {
    LOG(ERROR, __FUNCTION__, " Event not supported");
}
