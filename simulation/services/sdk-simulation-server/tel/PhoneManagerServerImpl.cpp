/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/common/Logger.hpp"
#include "PhoneManagerServerImpl.hpp"
#include "TelUtil.hpp"
#include <thread>

#define JSON_PATH1 "api/tel/IPhoneManagerSlot1.json"
#define JSON_PATH2 "api/tel/IPhoneManagerSlot2.json"

#define TEL_PHONE_MANAGER                       "IPhoneManager"
#define PHONE_EVENT_SIGNAL_STRENGTH_CHANGE      "signalStrengthUpdate"
#define PHONE_EVENT_CELL_INFO_CHANGE            "cellInfoListUpdate"
#define PHONE_EVENT_VOICE_SERVICE_STATE_CHANGE  "voiceServiceStateUpdate"
#define PHONE_EVENT_OPERATING_MODE_CHANGE       "operatingModeUpdate"
#define PHONE_EVENT_ECALL_OPERATING_MODE_CHANGE "eCallOperatingModeUpdate"
#define PHONE_EVENT_OPERATOR_INFO_CHANGE        "operatorInfoUpdate"

#define SLOT_1 1
#define SLOT_2 2
#define DEFAULT_SLOT_ID SLOT_1

PhoneManagerServerImpl::PhoneManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    modemMgr_ = std::make_shared<telux::common::ModemManagerImpl>();
}

PhoneManagerServerImpl::~PhoneManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (modemMgr_) {
        modemMgr_ = nullptr;
    }
}

telux::common::ServiceStatus PhoneManagerServerImpl::readSubsystemStatus(int slotId) {
    int cbDelay = 0;
    return readSubsystemStatus(slotId, cbDelay);
}

telux::common::ServiceStatus PhoneManagerServerImpl::readSubsystemStatus(int slotId, int &cbDelay) {
    Json::Value rootObj;
    std::string filePath = (slotId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }

    cbDelay = rootObj[TEL_PHONE_MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj[TEL_PHONE_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus, " slotId::", slotId);
    return status;
}

grpc::Status PhoneManagerServerImpl::InitService(ServerContext* context,
    const ::google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    int cbDelay = 0;
    telux::common::ServiceStatus status = readSubsystemStatus(DEFAULT_SLOT_ID, cbDelay);
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_PHONE_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    } else {
        LOG(ERROR, __FUNCTION__, " Json not found or service not available or failed");
        return grpc::Status(grpc::StatusCode::INTERNAL,
            " Json not found or service not available or failed");
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);
    modemMgr_->init();
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus status = readSubsystemStatus(DEFAULT_SLOT_ID);
    if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        return grpc::Status::OK;
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::IsSubsystemReady(ServerContext* context,
    const google::protobuf::Empty *request, commonStub::IsSubsystemReadyReply* response) {
    LOG(DEBUG, __FUNCTION__);
    bool status = false;
    telux::common::ServiceStatus servstatus = readSubsystemStatus(DEFAULT_SLOT_ID);
    if (servstatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        response->set_is_ready(status);
        return grpc::Status::OK;
    } else {
        status = true;
    }
    response->set_is_ready(status);
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetPhoneIds(ServerContext *context,
    const google::protobuf::Empty *request, telStub::GetPhoneIdsReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data = telux::tel::TelUtil::readGetPhoneIdsRespFromJsonFile(response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting phone Ids");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetPhoneId(ServerContext *context,
    const google::protobuf::Empty *request, telStub::GetPhoneIdReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data = telux::tel::TelUtil::readGetPhoneIdRespFromJsonFile(response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting phone Id");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetCellularCapabilities(ServerContext* context,
    const google::protobuf::Empty *request, telStub::CellularCapabilityInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data = telux::tel::TelUtil::readCellularCapabilitiesRespFromJsonFile(response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting cellular capability");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetOperatingMode(ServerContext* context,
    const ::telStub::SetOperatingModeRequest* request, telStub::SetOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    return modemMgr_->setOperatingMode(request, response);
}

grpc::Status PhoneManagerServerImpl::GetOperatingMode(ServerContext* context,
    const google::protobuf::Empty* request, telStub::GetOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    return modemMgr_->getOperatingMode(request, response);
}

grpc::Status PhoneManagerServerImpl::ResetWwan(ServerContext* context,
    const google::protobuf::Empty* request, telStub::ResetWwanReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data = telux::tel::TelUtil::readResetWwanRespFromJsonFile(response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in resetting WWAN");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::RequestVoiceServiceState(ServerContext* context,
    const telStub::RequestVoiceServiceStateRequest* request,
    telStub::RequestVoiceServiceStateReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    JsonData data = telux::tel::TelUtil::readVoiceServiceStateRespFromJsonFile(phoneId, response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting voice service state");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetRadioPower(ServerContext* context,
    const telStub::SetRadioPowerRequest* request, telStub::SetRadioPowerReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error =
        telux::tel::TelUtil::writeSetRadioPowerToJsonFileAndReply(request->phone_id(),
        request->enable(), response);
    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in writing radio power state");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::RequestCellInfoList(ServerContext* context,
    const telStub::RequestCellInfoListRequest* request,
    telStub::RequestCellInfoListReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    JsonData data = telux::tel::TelUtil::readCellInfoListRespFromJsonFile(phoneId, response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting cell info list");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetCellInfoListRate(ServerContext* context,
    const telStub::SetCellInfoListRateRequest* request,
    telStub::SetCellInfoListRateReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error =
        telux::tel::TelUtil::writeSetCellInfoListRateToJsonFileAndReply(
        request->cell_info_rate(), response);
    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in writing cell info rate");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetSignalStrength(ServerContext* context,
    const telStub::GetSignalStrengthRequest* request,
    telStub::GetSignalStrengthReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    JsonData data = telux::tel::TelUtil::readSignalStrengthRespFromJsonFile(phoneId, response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting signal strength");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetECallOperatingMode(ServerContext* context,
    const telStub::SetECallOperatingModeRequest* request,
    telStub::SetECallOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    telux::common::ErrorCode error = telux::tel::TelUtil::writeEcallOperatingModeToJsonFileAndReply(
        phoneId, request->ecall_mode(),
        telStub::ECallModeReason_Reason::ECallModeReason_Reason_NORMAL, response);
    LOG(DEBUG, __FUNCTION__, " error: ", response->error(), " status: ", response->status());
    if (error != telux::common::ErrorCode::SUCCESS) {
        //On error condition notification should not be send.
        return grpc::Status::OK;
    }
    ::telStub::ECallModeInfoChangeEvent operatingModeEvent;
    error = telux::tel::TelUtil::readEcallOperatingModeEventFromJsonFile(phoneId,
        operatingModeEvent);

    if (error == telux::common::ErrorCode::SUCCESS) {
        ::eventService::EventResponse anyResponse;
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(operatingModeEvent);
        auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
        }).share();
        taskQ_->add(f);
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to read eCall operating mode");
        return grpc::Status(grpc::StatusCode::INTERNAL, " Internal Error");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetECallOperatingMode(ServerContext* context,
    const telStub::GetECallOperatingModeRequest* request,
    telStub::GetECallOperatingModeReply* response) {

    int phoneId = request->phone_id();
    JsonData data = telux::tel::TelUtil::readEcallOperatingModeRespFromJsonFile(phoneId, response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting eCall operating mode");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::RequestOperatorInfo(ServerContext* context,
    const telStub::RequestOperatorInfoRequest* request,
    telStub::RequestOperatorInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = request->phone_id();
    JsonData data = telux::tel::TelUtil::readRequestOperatorInfoRespFromJsonFile(phoneId, response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting operator info");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::ConfigureSignalStrength(ServerContext* context,
    const telStub::ConfigureSignalStrengthRequest* request,
    telStub::ConfigureSignalStrengthReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = request->phone_id();
    std::vector<telStub::ConfigureSignalStrength> signalStrengthConfigs = {};
    for (auto config : request->config()) {
        signalStrengthConfigs.emplace_back(config);
    }
    telux::common::ErrorCode error =
        telux::tel::TelUtil::writeConfigureSignalStrengthToJsonFileAndReply(
        phoneId, signalStrengthConfigs, response);
    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in configuring signal strength");
    }
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::ConfigureSignalStrengthEx(ServerContext* context,
    const telStub::ConfigureSignalStrengthExRequest* request,
    telStub::ConfigureSignalStrengthExReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = request->phone_id();
    std::vector<telStub::ConfigureSignalStrengthEx> signalStrengthConfigEx = {};
    for (auto config : request->config()) {
        signalStrengthConfigEx.emplace_back(config);
    }
    uint16_t hysTimer = static_cast<uint16_t>(request->hysteresis_ms());

    telux::common::ErrorCode error =
        telux::tel::TelUtil::writeConfigureSignalStrengthExToJsonFileAndReply(
        phoneId, signalStrengthConfigEx, response, hysTimer);
    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in configuring signal strength");
    }
    return grpc::Status::OK;
}

void PhoneManagerServerImpl::triggerChangeEvent(::eventService::EventResponse anyResponse) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void PhoneManagerServerImpl::handleSignalStrengthChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    // Split the event string into parameters( for phoneId, SignalStrength1, SignalStrength2 ...)
    // based on delimeter as ","
    std::stringstream ss(eventParams);
    std::vector<std::string> params;
    while (getline(ss, eventParams, ',')) {
        params.emplace_back(eventParams);
    }

    int phoneId;
    bool notify = false;
    telux::common::ErrorCode errorCode = telux::tel::TelUtil::writeSignalStrengthToJsonFile(params,
        phoneId, notify);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        ::telStub::SignalStrengthChangeEvent signalStrengthChangeEvent;
        errorCode = telux::tel::TelUtil::readSignalStrengthEventFromJsonFile(phoneId,
            signalStrengthChangeEvent);

        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            ::eventService::EventResponse anyResponse;
            anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
            anyResponse.mutable_any()->PackFrom(signalStrengthChangeEvent);
            modemMgr_->updateSignalStrength(signalStrengthChangeEvent.phone_id());
            // notify signal strength only if any criteria is met
            LOG(INFO, __FUNCTION__, " notification needed : ", notify);
            if (notify) {
                auto f = std::async(std::launch::async, [this, anyResponse]() {
                    this->triggerChangeEvent(anyResponse);
                }).share();
                taskQ_->add(f);
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Unable to read signal strength");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write signal strength");
    }
}

void PhoneManagerServerImpl::handleCellInfoChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);

    // Split the event string into parameters( for phoneId, CellInfo1, CellInfo2 ...) based on
    // delimeter as ","
    std::stringstream ss(eventParams);
    std::vector<string> params;
    while (getline(ss, eventParams, ',')) {
        params.emplace_back(eventParams);
    }

    for(std::string str:params) {
        LOG(DEBUG, __FUNCTION__," Param: ", str);
    }

    int phoneId;
    telux::common::ErrorCode errorCode = telux::tel::TelUtil::writeCellInfoListToJsonFile(params,
        phoneId);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        ::telStub::CellInfoListEvent cellInfoListEvent;
        errorCode = telux::tel::TelUtil::readCellInfoListEventFromJsonFile(phoneId,
            cellInfoListEvent);

        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            ::eventService::EventResponse anyResponse;
            anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
            anyResponse.mutable_any()->PackFrom(cellInfoListEvent);
            auto f = std::async(std::launch::async, [this, anyResponse]() {
                this->triggerChangeEvent(anyResponse);
            }).share();
            taskQ_->add(f);
        } else {
            LOG(ERROR, __FUNCTION__, " Unable to read cell info list");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write cell info list");
    }
}

void PhoneManagerServerImpl::handleVoiceServiceStateChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId;
    telux::common::ErrorCode errorCode = telux::tel::TelUtil::writeVoiceServiceStateToJsonFile(
        eventParams, phoneId);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        ::telStub::VoiceServiceStateEvent voiceServiceStateEvent;
        errorCode = telux::tel::TelUtil::readVoiceServiceStateEventFromJsonFile(phoneId,
            voiceServiceStateEvent);

        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            ::eventService::EventResponse anyResponse;
            anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
            anyResponse.mutable_any()->PackFrom(voiceServiceStateEvent);
            auto f = std::async(std::launch::async, [this, anyResponse]() {
                this->triggerChangeEvent(anyResponse);
            }).share();
            taskQ_->add(f);
        } else {
            LOG(ERROR, __FUNCTION__, " Unable to read voice service state");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write voice service state");
    }
}

void PhoneManagerServerImpl::handleOperatingModeChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    ::telStub::OperatingModeEvent oldOperatingModeEvent;
    telux::common::ErrorCode errorCode = telux::tel::TelUtil::readOperatingModeEventFromJsonFile(
        oldOperatingModeEvent);
    errorCode = telux::tel::TelUtil::writeOperatingModeToJsonFile(
        eventParams, phoneId);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        ::telStub::OperatingModeEvent newOperatingModeEvent;
        errorCode = telux::tel::TelUtil::readOperatingModeEventFromJsonFile(newOperatingModeEvent);
        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            if (newOperatingModeEvent.operating_mode() == oldOperatingModeEvent.operating_mode()) {
                LOG(ERROR, __FUNCTION__, " Current operating mode and new operating mode is same");
                return;
            }
            ::eventService::EventResponse anyResponse;
            anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
            modemMgr_->updateOperatingModeState(newOperatingModeEvent.operating_mode());
            anyResponse.mutable_any()->PackFrom(newOperatingModeEvent);
            auto f = std::async(std::launch::async, [this, anyResponse]() {
                this->triggerChangeEvent(anyResponse);
            }).share();
            taskQ_->add(f);
        } else {
            LOG(ERROR, __FUNCTION__, " Unable to read operating mode");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write operating mode");
    }
}

void PhoneManagerServerImpl::handleECallOperatingModeChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    telux::common::ErrorCode errorCode = telux::tel::TelUtil::writeEcallOperatingModeToJsonFile(
        eventParams, phoneId);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        ::telStub::ECallModeInfoChangeEvent operatingModeEvent;
        errorCode = telux::tel::TelUtil::readEcallOperatingModeEventFromJsonFile(phoneId,
            operatingModeEvent);

        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            ::eventService::EventResponse anyResponse;
            anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
            //Notify modem manager
            anyResponse.mutable_any()->PackFrom(operatingModeEvent);
            auto f = std::async(std::launch::async, [this, anyResponse]() {
                this->triggerChangeEvent(anyResponse);
            }).share();
            taskQ_->add(f);
        } else {
            LOG(ERROR, __FUNCTION__, " Unable to read eCall operating mode");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write eCall operating mode");
    }
}

void PhoneManagerServerImpl::handleOperatorInfoChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    telux::common::ErrorCode errorCode = telux::tel::TelUtil::writeOperatorInfoToJsonFile(
        eventParams, phoneId);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        ::telStub::OperatorInfoEvent operatorInfoEvent;
        errorCode = telux::tel::TelUtil::readOperatorInfoEventFromJsonFile(phoneId,
            operatorInfoEvent);
        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            ::eventService::EventResponse anyResponse;
            anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
            //Notify modem manager
            anyResponse.mutable_any()->PackFrom(operatorInfoEvent);
            auto f = std::async(std::launch::async, [this, anyResponse]() {
                this->triggerChangeEvent(anyResponse);
            }).share();
            taskQ_->add(f);
        } else {
            LOG(ERROR, __FUNCTION__, " Unable to read operator info");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Unable to write operator info");
    }
}

void PhoneManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_PHONE_FILTER) {
        std::string event = message.event();
        onEventUpdate(event);
    }
}

void PhoneManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event: ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token: ", token );
    if (PHONE_EVENT_SIGNAL_STRENGTH_CHANGE == token) {
        handleSignalStrengthChanged(event);
    } else if (PHONE_EVENT_CELL_INFO_CHANGE == token) {
        handleCellInfoChanged(event);
    } else if (PHONE_EVENT_VOICE_SERVICE_STATE_CHANGE == token) {
        handleVoiceServiceStateChanged(event);
    } else if (PHONE_EVENT_OPERATING_MODE_CHANGE == token) {
       handleOperatingModeChanged(event);
    } else if (PHONE_EVENT_ECALL_OPERATING_MODE_CHANGE == token) {
       handleECallOperatingModeChanged(event);
    } else if (PHONE_EVENT_OPERATOR_INFO_CHANGE == token) {
       handleOperatorInfoChanged(event);
    } else {
        LOG(ERROR, __FUNCTION__, " Event not supported");
    }
}
