/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "ImsSettingsManagerServerImpl.hpp"

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/ImsSettingsManager.hpp>

#define JSON_PATH1 "api/tel/IImsSettingsManagerSlot1.json"
#define JSON_PATH2 "api/tel/IImsSettingsManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/IImsSettingsManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/IImsSettingsManagerStateSlot2.json"
#define IMS_SETTINGS_MANAGER                           "IImsSettingsManager"
#define IMS_SETTINGS_EVENT_SERVICE_CONFIGS_CHANGE      "imsServiceConfigsUpdate"
#define IMS_SETTINGS_EVENT_SIP_USER_AGENT_CHANGE       "imsSipUserAgentUpdate"
#define SLOT_1 1
#define SLOT_2 2

ImsSettingsManagerServerImpl::ImsSettingsManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status ImsSettingsManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status ImsSettingsManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = JSON_PATH1;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[IMS_SETTINGS_MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[IMS_SETTINGS_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_IMS_SETTINGS_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status ImsSettingsManagerServerImpl::GetServiceStatus(ServerContext* context,
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
    std::string srvStatus = rootObj[IMS_SETTINGS_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status ImsSettingsManagerServerImpl::RequestServiceConfig(ServerContext* context,
    const ::telStub::RequestServiceConfigRequest* request,
    telStub::RequestServiceConfigReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = IMS_SETTINGS_MANAGER;
    std::string method = "requestServiceConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        bool imsServiceEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]\
            ["ImsServiceConfigInfo"]["imsServiceEnabled"].asBool();
        bool voImsEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]\
            ["ImsServiceConfigInfo"]["voImsEnabled"].asBool();
        bool smsEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]\
            ["ImsServiceConfigInfo"]["smsEnabled"].asBool();
        bool rttEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]\
            ["ImsServiceConfigInfo"]["rttEnabled"].asBool();
        LOG(DEBUG, " imsServiceEnabled: ", imsServiceEnabled, " voImsEnabled: ", voImsEnabled,
            " smsEnabled: ", smsEnabled, " rttEnabled: ", rttEnabled);
        response->mutable_config()->set_is_ims_service_enabled_valid(true);
        response->mutable_config()->set_ims_service_enabled(imsServiceEnabled);
        response->mutable_config()->set_is_voims_enabled_valid(true);
        response->mutable_config()->set_voims_enabled(voImsEnabled);
        response->mutable_config()->set_is_sms_enabled_valid(true);
        response->mutable_config()->set_sms_enabled(smsEnabled);
        response->mutable_config()->set_is_rtt_enabled_valid(true);
        response->mutable_config()->set_rtt_enabled(rttEnabled);
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

grpc::Status ImsSettingsManagerServerImpl::SetServiceConfig(ServerContext* context,
    const ::telStub::SetServiceConfigRequest* request,
    telStub::SetServiceConfigReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->phone_id();
    std::string apiJsonPath = (slotId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = IMS_SETTINGS_MANAGER;
    std::string method = "setServiceConfig";
    JsonData data;
    bool imsServiceEnabled = request->config().ims_service_enabled();
    bool voImsEnabled = request->config().voims_enabled();
    bool smsEnabled = request->config().sms_enabled();
    bool rttEnabled = request->config().rtt_enabled();
    bool prevImsServiceEnabled = false, prevVoImsEnabled = false, prevSmsEnabled = false;
    bool prevRttEnabled = false;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        if (request->config().is_ims_service_enabled_valid()) {
            prevImsServiceEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]\
                ["ImsServiceConfigInfo"]["imsServiceEnabled"].asBool();
            data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsServiceConfigInfo"]["imsServiceEnabled"]
                = imsServiceEnabled;
        }
        if (request->config().is_voims_enabled_valid()) {
            prevVoImsEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsServiceConfigInfo"]\
                ["voImsEnabled"].asBool();
            data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsServiceConfigInfo"]["voImsEnabled"]
                = voImsEnabled;
        }
        if (request->config().is_sms_enabled_valid()) {
            prevSmsEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsServiceConfigInfo"]\
                ["smsEnabled"].asBool();
            data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsServiceConfigInfo"]["smsEnabled"]
                = smsEnabled;
        }
        if (request->config().is_rtt_enabled_valid()) {
            prevRttEnabled = data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsServiceConfigInfo"]\
                ["rttEnabled"].asBool();
            data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsServiceConfigInfo"]["rttEnabled"]
                = rttEnabled;
        }
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
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

    // send a notification for change
    triggerImsServiceConfigsChange(slotId, prevImsServiceEnabled, imsServiceEnabled,
        prevVoImsEnabled, voImsEnabled, prevSmsEnabled, smsEnabled, prevRttEnabled, rttEnabled);
    return grpc::Status::OK;
}

grpc::Status ImsSettingsManagerServerImpl::RequestSipUserAgent(ServerContext* context,
    const ::telStub::RequestSipUserAgentRequest* request,
    telStub::RequestSipUserAgentReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = IMS_SETTINGS_MANAGER;
    std::string method = "requestSipUserAgent";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        std::string sipUserAgent = data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsSipUserAgentInfo"]\
            ["sipUserAgent"].asString();
        LOG(DEBUG, __FUNCTION__, " sipUserAgent: ", sipUserAgent);
        response->set_sip_user_agent(sipUserAgent);
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

grpc::Status ImsSettingsManagerServerImpl::SetSipUserAgent(ServerContext* context,
    const ::telStub::SetSipUserAgentRequest* request,
    telStub::SetSipUserAgentReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->phone_id();
    std::string sipUserAgent = request->sip_user_agent();
    std::string apiJsonPath = (slotId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = IMS_SETTINGS_MANAGER;
    std::string method = "setSipUserAgent";
    JsonData data;
    std::string prevSipUserAgent = "";
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        prevSipUserAgent = data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsSipUserAgentInfo"]\
            ["sipUserAgent"].asString();
        data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsSipUserAgentInfo"]["sipUserAgent"] =
            sipUserAgent;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
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

    // send a notification
    triggerImsSipUserAgentChange(slotId, prevSipUserAgent, sipUserAgent);
    return grpc::Status::OK;
}

void ImsSettingsManagerServerImpl::triggerImsServiceConfigsChange(int slotId,
    bool prevImsServiceEnabled, bool imsServiceEnabled,
    bool prevVoImsEnabled, bool voImsEnabled, bool prevSmsEnabled, bool smsEnabled,
    bool prevRttEnabled, bool rttEnabled) {
   LOG(INFO, __FUNCTION__, " imsServiceEnabled is ",
        imsServiceEnabled, " voImsEnabled is ", voImsEnabled, " smsEnabled is ", smsEnabled,
        " rttEnabled is ", rttEnabled);
    // check the previous value then update and send notification.
    std::string stateJsonPath = (slotId == SLOT_1)?
        "tel/IImsSettingsManagerStateSlot1" : "tel/IImsSettingsManagerStateSlot2";
    if ((prevImsServiceEnabled != imsServiceEnabled) || (prevVoImsEnabled != voImsEnabled) ||
        (prevSmsEnabled != smsEnabled) || (prevRttEnabled != rttEnabled)) {
        // get the latest values
        imsServiceEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "true",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "imsServiceEnabled"}) != "false";
        voImsEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "true",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "voImsEnabled"}) != "false";
        smsEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "true",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "smsEnabled"}) != "false";
        rttEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "false",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "rttEnabled"}) != "false";

        ::telStub::ImsServiceConfigsChangeEvent imsServiceConfigsChangeEvent;
        ::eventService::EventResponse anyResponse;

        imsServiceConfigsChangeEvent.set_phone_id(slotId);
        imsServiceConfigsChangeEvent.mutable_config()->set_is_ims_service_enabled_valid(true);
        imsServiceConfigsChangeEvent.mutable_config()->set_ims_service_enabled(imsServiceEnabled);
        imsServiceConfigsChangeEvent.mutable_config()->set_is_voims_enabled_valid(true);
        imsServiceConfigsChangeEvent.mutable_config()->set_voims_enabled(voImsEnabled);
        imsServiceConfigsChangeEvent.mutable_config()->set_is_sms_enabled_valid(true);
        imsServiceConfigsChangeEvent.mutable_config()->set_sms_enabled(smsEnabled);
        imsServiceConfigsChangeEvent.mutable_config()->set_is_rtt_enabled_valid(true);
        imsServiceConfigsChangeEvent.mutable_config()->set_rtt_enabled(rttEnabled);
        anyResponse.set_filter(telux::tel::TEL_IMS_SETTINGS_FILTER);
        anyResponse.mutable_any()->PackFrom(imsServiceConfigsChangeEvent);
        // posting the event to EventService event queue
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    } else {
        LOG(ERROR, __FUNCTION__, " Data not changed, ignoring notification");
    }
}

grpc::Status ImsSettingsManagerServerImpl::RequestVonr(ServerContext* context,
    const ::telStub::RequestVonrRequest* request,
    telStub::RequestVonrReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = IMS_SETTINGS_MANAGER;
    std::string method = "requestVonr";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        bool enable = data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsVonrEnable"]
           .asBool();
        LOG(DEBUG, __FUNCTION__, " IMS VoNR enable: ", enable);
        response->set_enable(enable);
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

grpc::Status ImsSettingsManagerServerImpl::SetVonr(ServerContext* context,
    const ::telStub::SetVonrRequest* request,
    telStub::SetVonrReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->phone_id();
    std::string apiJsonPath = (slotId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = IMS_SETTINGS_MANAGER;
    std::string method = "setVonr";
    JsonData data;
    std::string prevSipUserAgent = "";
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        data.stateRootObj[IMS_SETTINGS_MANAGER]["ImsVonrEnable"] =
            request->enable();
        LOG(DEBUG, __FUNCTION__, " IMS VoNR enable: ", request->enable());
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
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

void ImsSettingsManagerServerImpl::handleImsServiceConfigsChange(std::string eventParams) {
    LOG(INFO, __FUNCTION__);
    int slotId;
    JsonData data;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    try {
        if (token == "") {
            LOG(INFO, __FUNCTION__, " The Slot id is not passed! Assuming default Slot Id");
            slotId = DEFAULT_SLOT_ID;
        } else {
            slotId = std::stoi(token);
        }
        if ((slotId == SLOT_2) && (!(telux::common::DeviceConfig::isMultiSimSupported()))) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
        if ((slotId < DEFAULT_SLOT_ID) || (slotId > SLOT_2)) {
            LOG(ERROR, __FUNCTION__, " Invalid slot Id");
            return;
        }
        LOG(DEBUG, __FUNCTION__, " The Slot id is: ", slotId ,
            " leftover string is: ", eventParams);

        // Fetch imsServiceEnabled
        int imsServiceEnabled;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(INFO, __FUNCTION__, " imsServiceEnabled not passed ");
            imsServiceEnabled = false; // disabled by default
        } else {
            imsServiceEnabled = std::stoi(token);
        }
        // Fetch voImsEnabled
        int voImsEnabled;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(INFO, __FUNCTION__, " voImsEnabled not passed ");
            voImsEnabled = false; // disabled by default
        } else {
            voImsEnabled = std::stoi(token);
        }
        // Fetch smsEnabled
        int smsEnabled;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(INFO, __FUNCTION__, " smsEnabled not passed ");
            smsEnabled = false; // disabled by default
        } else {
            smsEnabled = std::stoi(token);
        }
        // Fetch rttEnabled
        int rttEnabled;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(INFO, __FUNCTION__, " rttEnabled not passed ");
            rttEnabled = false; // disbled by defalt
        } else {
            rttEnabled = std::stoi(token);
        }
        // check the previous value then update and send notification.
        std::string stateJsonPath = (slotId == SLOT_1)?
            "tel/IImsSettingsManagerStateSlot1" : "tel/IImsSettingsManagerStateSlot2";
        bool prevImsServiceEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "true",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "imsServiceEnabled"}) != "false";
        bool prevVoImsEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "true",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "voImsEnabled"}) != "false";
        bool prevSmsEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "true",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "smsEnabled"}) != "false";
        bool prevRttEnabled = CommonUtils::readSystemDataValue(stateJsonPath, "false",
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "rttEnabled"}) != "false";
        CommonUtils::writeSystemDataValue<bool>(stateJsonPath, imsServiceEnabled,
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "imsServiceEnabled"});
        CommonUtils::writeSystemDataValue<bool>(stateJsonPath, voImsEnabled,
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "voImsEnabled"});
        CommonUtils::writeSystemDataValue<bool>(stateJsonPath, smsEnabled,
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "smsEnabled"});
        CommonUtils::writeSystemDataValue<bool>(stateJsonPath, rttEnabled,
            {IMS_SETTINGS_MANAGER, "ImsServiceConfigInfo", "rttEnabled"});

        triggerImsServiceConfigsChange(slotId, prevImsServiceEnabled, imsServiceEnabled,
            prevVoImsEnabled, voImsEnabled, prevSmsEnabled, smsEnabled,
            prevRttEnabled, rttEnabled);
     } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
    }
}

void ImsSettingsManagerServerImpl::triggerImsSipUserAgentChange(int slotId,
    std::string prevSipUserAgent, std::string sipUserAgent) {
    LOG(DEBUG, " prevSipUserAgent : ", prevSipUserAgent, " sipUserAgent : ", sipUserAgent);
    std::string stateJsonPath = (slotId == SLOT_1)?
        "tel/IImsSettingsManagerStateSlot1" : "tel/IImsSettingsManagerStateSlot2";
    if (prevSipUserAgent.compare(sipUserAgent) != 0) {
        // read the latest value
        sipUserAgent = CommonUtils::readSystemDataValue(stateJsonPath, "",
            {IMS_SETTINGS_MANAGER, "ImsSipUserAgentInfo", "sipUserAgent"});

        ::telStub::ImsSipUserAgentChangeEvent imsSipUserAgentChangeEvent;
        ::eventService::EventResponse anyResponse;

        imsSipUserAgentChangeEvent.set_phone_id(slotId);
        imsSipUserAgentChangeEvent.set_sip_user_agent(sipUserAgent);
        anyResponse.set_filter(telux::tel::TEL_IMS_SETTINGS_FILTER);
        anyResponse.mutable_any()->PackFrom(imsSipUserAgentChangeEvent);
        // posting the event to EventService event queue
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    } else {
        LOG(ERROR, __FUNCTION__, " Data not changed, ignoring notification");
    }
}

void ImsSettingsManagerServerImpl::handleImsSipUserAgentChange(std::string eventParams) {
    LOG(INFO, __FUNCTION__);
    int slotId;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    try {
        if (token == "") {
            LOG(INFO, __FUNCTION__, " The Slot id is not passed! Assuming default Slot Id");
            slotId = DEFAULT_SLOT_ID;
        } else {
            slotId = std::stoi(token);
        }
        if ((slotId == SLOT_2) && (!(telux::common::DeviceConfig::isMultiSimSupported()))) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
        if ((slotId < DEFAULT_SLOT_ID) || (slotId > SLOT_2)) {
            LOG(ERROR, __FUNCTION__, " Invalid slot Id");
            return;
        }
        LOG(DEBUG, __FUNCTION__, " The Slot id is: ", slotId ,
            " leftover string is: ", eventParams);

        // Fetch sip user agent
        std::string sipUserAgent;
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token == "") {
            LOG(INFO, __FUNCTION__, " sipUserAgent not passed ");
            sipUserAgent = ""; // null
        } else {
            sipUserAgent = token;
        }
        std::string stateJsonPath = (slotId == SLOT_1)?
            "tel/IImsSettingsManagerStateSlot1" : "tel/IImsSettingsManagerStateSlot2";
        std::string prevSipUserAgent = CommonUtils::readSystemDataValue(stateJsonPath, "",
            {IMS_SETTINGS_MANAGER, "ImsSipUserAgentInfo", "sipUserAgent"});
        CommonUtils::writeSystemDataValue<string>(stateJsonPath, sipUserAgent,
            {IMS_SETTINGS_MANAGER, "ImsSipUserAgentInfo", "sipUserAgent"});

        triggerImsSipUserAgentChange(slotId, prevSipUserAgent, sipUserAgent);
     } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
    }
}

void ImsSettingsManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    LOG(INFO, __FUNCTION__);
    if (message.filter() == telux::tel::TEL_IMS_SETTINGS_FILTER) {
        std::string event = message.event();
        onEventUpdate(event);
    }
}

void ImsSettingsManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event: ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token: ", token );
    if (IMS_SETTINGS_EVENT_SERVICE_CONFIGS_CHANGE == token) {
        handleImsServiceConfigsChange(event);
    } else if (IMS_SETTINGS_EVENT_SIP_USER_AGENT_CHANGE == token) {
        handleImsSipUserAgentChange(event);
    } else {
        LOG(ERROR, __FUNCTION__, " Event not supported");
    }
}

