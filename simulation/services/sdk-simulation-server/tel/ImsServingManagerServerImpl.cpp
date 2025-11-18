/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include"ImsServingManagerServerImpl.hpp"

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#include <telux/tel/ImsServingSystemManager.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/common/DeviceConfig.hpp>

#define JSON_PATH1 "api/tel/IImsServingSystemManagerSlot1.json"
#define JSON_PATH2 "api/tel/IImsServingSystemManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/IImsServingSystemManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/IImsServingSystemManagerStateSlot2.json"
#define MANAGER "IImsServingSystemManager"
#define SLOT_1 1
#define SLOT_2 2

#define IMS_SERVING_EVENT_REG_STATUS_CHANGE       "regStatusUpdate"
#define IMS_SERVING_EVENT_SERVICES_INFO_CHANGE    "serviceInfoUpdate"
#define IMS_SERVING_EVENT_PDP_STATUS_INFO_CHANGE  "pdpStatusInfoUpdate"

ImsServingManagerServerImpl::ImsServingManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

ImsServingManagerServerImpl::~ImsServingManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status ImsServingManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_IMS_SERVING_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::RequestRegistrationInfo(ServerContext* context,
    const ::telStub::RequestRegistrationInfoRequest* request,
    telStub::RequestRegistrationInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRegistrationInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        telStub::RegistrationStatus regStatus =
                static_cast<telStub::RegistrationStatus>(data.stateRootObj[MANAGER]\
                ["ImsRegistrationInfo"]["RegStatus"].asInt());
            telStub::RadioTechnology rat =
                static_cast<telStub::RadioTechnology>(data.stateRootObj[MANAGER]\
                ["ImsRegistrationInfo"]["rat"].asInt());
            int errorCode = data.stateRootObj[MANAGER]["ImsRegistrationInfo"]["errorCode"].asInt();
            std::string errorString =
                data.stateRootObj[MANAGER]["ImsRegistrationInfo"]["errorString"].asString();
            response->set_ims_reg_status(regStatus);
            response->set_rat(rat);
            response->set_error_code(errorCode);
            response->set_error_string(errorString);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::RequestServiceInfo(ServerContext* context,
    const ::telStub::RequestServiceInfoRequest* request,
    telStub::RequestServiceInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestServiceInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        telStub::CellularService_Status sms =
            static_cast<telStub::CellularService_Status>(data.stateRootObj[MANAGER]\
                ["ImsServiceInfo"]["sms"].asInt());
        telStub::CellularService_Status voice =
            static_cast<telStub::CellularService_Status>(data.stateRootObj[MANAGER]\
                ["ImsServiceInfo"]["voice"].asInt());
        response->set_sms(sms);
        response->set_voice(voice);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::RequestPdpStatus(ServerContext* context,
    const ::telStub::RequestPdpStatusRequest* request,
    telStub::RequestPdpStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestPdpStatus";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        bool value = data.stateRootObj[MANAGER]["ImsPdpStatusInfo"]["isPdpConnected"].asBool();
        telStub::PdpFailureCode pdpFailure =
            static_cast<telStub::PdpFailureCode>(data.stateRootObj[MANAGER]\
            ["ImsPdpStatusInfo"]["failureCode"].asInt());
        telStub::EndReasonType dataCallEndReason =
            static_cast<telStub::EndReasonType>(data.stateRootObj[MANAGER]
            ["ImsPdpStatusInfo"]["failureReason"].asInt());
        std::string apnName = data.stateRootObj[MANAGER]["ImsPdpStatusInfo"]["apnName"].asString();
        response->set_is_pdp_connected(value);
        response->set_failure_code(pdpFailure);
        response->set_failure_reason(dataCallEndReason);
        response->set_apn_name(apnName);

    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void ImsServingManagerServerImpl::handleImsRegStatusChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    Json::Value rootObj;
    std::string jsonfilename;
    ::telStub::ImsRegStatusChangeEvent imsRegStatusEvent;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }
        if(phoneId == SLOT_2) {
            if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
                LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
                return;
            }
        }

        // Read string to get registration status
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int regStatus = std::stoi(token);
        if (regStatus < (static_cast<int>(telStub::RegistrationStatus::UNKOWN_STATE)) ||
            regStatus > (static_cast<int>(telStub::RegistrationStatus::LIMITED_REGISTERED))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for registration status");
            return;
        }

        // Read string to get radio technology
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int rat = std::stoi(token);
        if (regStatus < (static_cast<int>(telStub::RadioTechnology::RADIO_TECH_UNKNOWN)) ||
            regStatus > (static_cast<int>(telStub::RadioTechnology::RADIO_TECH_NR5G))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for radio technology");
            return;
        }

        // Read string to get error code and error string
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int errorCode = std::stoi(token);
        std::string errorString = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);

        jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }
        rootObj[MANAGER]["ImsRegistrationInfo"]["RegStatus"] = regStatus;
        rootObj[MANAGER]["ImsRegistrationInfo"]["rat"] = rat;
        rootObj[MANAGER]["ImsRegistrationInfo"]["errorCode"] = errorCode;
        rootObj[MANAGER]["ImsRegistrationInfo"]["errorString"] = errorString;
        imsRegStatusEvent.set_phone_id(phoneId);
        imsRegStatusEvent.set_ims_reg_status(static_cast<telStub::RegistrationStatus>(regStatus));
        imsRegStatusEvent.set_rat(static_cast<telStub::RadioTechnology>(rat));
        imsRegStatusEvent.set_error_code(errorCode);
        imsRegStatusEvent.set_error_string(errorString);
        LOG(DEBUG, __FUNCTION__, " regStatus: ", regStatus, " rat: ", rat,
            " errorCode: ", errorCode, " errorString: ", errorString);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    if (JsonParser::writeToJsonFile(rootObj, jsonfilename) == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_IMS_SERVING_FILTER);
        anyResponse.mutable_any()->PackFrom(imsRegStatusEvent);
        auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
        }).share();
        taskQ_->add(f);
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write registration information");
    }
}

void ImsServingManagerServerImpl::handleImsServiceInfoChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    Json::Value rootObj;
    std::string jsonfilename;
    ::telStub::ImsServiceInfoChangeEvent imsServiceInfoEvent;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }
        if(phoneId == SLOT_2) {
            if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
                LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
                return;
            }
        }

        // Read string to get IMS SMS status
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int smsStatus = std::stoi(token);
        if (smsStatus < (static_cast<int>(telStub::CellularService::UNKNOWN)) ||
            smsStatus > (static_cast<int>(telStub::CellularService::FULL_SERVICE))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for IMS SMS status");
            return;
        }

        // Read string to get IMS voice status
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int voiceStatus = std::stoi(token);
        if (voiceStatus < (static_cast<int>(telStub::CellularService::UNKNOWN)) ||
            voiceStatus > (static_cast<int>(telStub::CellularService::FULL_SERVICE))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for IMS voice status");
            return;
        }

        jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }
        rootObj[MANAGER]["ImsServiceInfo"]["sms"] = smsStatus;
        rootObj[MANAGER]["ImsServiceInfo"]["voice"] = voiceStatus;
        imsServiceInfoEvent.set_phone_id(phoneId);
        imsServiceInfoEvent.set_sms(static_cast<telStub::CellularService_Status>(smsStatus));
        imsServiceInfoEvent.set_voice(static_cast<telStub::CellularService_Status>(voiceStatus));
        LOG(DEBUG, __FUNCTION__, " IMS SMS status: ", smsStatus, " voice status: ", voiceStatus);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    if (JsonParser::writeToJsonFile(rootObj, jsonfilename) == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_IMS_SERVING_FILTER);
        anyResponse.mutable_any()->PackFrom(imsServiceInfoEvent);
        auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
        }).share();
        taskQ_->add(f);
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write service information");
    }
}

void ImsServingManagerServerImpl::handleImsPdpStatusInfoChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    Json::Value rootObj;
    std::string jsonfilename;
    ::telStub::ImsPdpStatusInfoChangeEvent imsPdpInfoEvent;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }
        if(phoneId == SLOT_2) {
            if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
                LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
                return;
            }
        }

        // Read string to get pdp connected status
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int isConnected = std::stoi(token);

        // Read string to get pdp failure code
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int pdpFailure = std::stoi(token);
        if (pdpFailure < (static_cast<int>(telStub::PdpFailureCode::OTHER_FAILURE)) ||
            pdpFailure > (static_cast<int>(telStub::PdpFailureCode::USER_AUTH_FAILED))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for pdp failure code");
            return;
        }

        // Read string to get pdp failure reason
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int dataCallEndReason = std::stoi(token);
        if (dataCallEndReason < (static_cast<int>(telStub::EndReasonType::CE_UNKNOWN)) ||
            dataCallEndReason > (static_cast<int>(telStub::EndReasonType::CE_HANDOFF))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for pdp failure reason");
            return;
        }

        // Read string to get apn name
        std::string apnName = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);

        jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }
        rootObj[MANAGER]["ImsPdpStatusInfo"]["isPdpConnected"] = isConnected;
        rootObj[MANAGER]["ImsPdpStatusInfo"]["failureCode"] = pdpFailure;
        rootObj[MANAGER]["ImsPdpStatusInfo"]["failureReason"] = dataCallEndReason;
        rootObj[MANAGER]["ImsPdpStatusInfo"]["apnName"] = apnName;
        imsPdpInfoEvent.set_phone_id(phoneId);
        imsPdpInfoEvent.set_is_pdp_connected(isConnected);
        imsPdpInfoEvent.set_failure_code(static_cast<telStub::PdpFailureCode>(pdpFailure));
        imsPdpInfoEvent.set_failure_reason(static_cast<telStub::EndReasonType>(dataCallEndReason));
        imsPdpInfoEvent.set_apn_name(apnName);
        LOG(DEBUG, __FUNCTION__, " pdp connected status: ", isConnected,
            " pdp failure code: ", pdpFailure, " pdp failure reason: ", dataCallEndReason,
            " apn name: ", apnName);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    if (JsonParser::writeToJsonFile(rootObj, jsonfilename) == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_IMS_SERVING_FILTER);
        anyResponse.mutable_any()->PackFrom(imsPdpInfoEvent);
        auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
        }).share();
        taskQ_->add(f);
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write service information");
    }
}

void ImsServingManagerServerImpl::triggerChangeEvent(
    ::eventService::EventResponse anyResponse) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void ImsServingManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_IMS_SERVING_FILTER) {
        std::string event = message.event();
        onEventUpdate(event);
    }
}

void ImsServingManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event: ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token: ", token );
    if (IMS_SERVING_EVENT_REG_STATUS_CHANGE == token) {
        handleImsRegStatusChanged(event);
    } else if (IMS_SERVING_EVENT_SERVICES_INFO_CHANGE == token) {
        handleImsServiceInfoChanged(event);
    } else if (IMS_SERVING_EVENT_PDP_STATUS_INFO_CHANGE == token) {
        handleImsPdpStatusInfoChanged(event);
    } else {
        LOG(ERROR, __FUNCTION__, " Event not supported");
    }
}