/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "DataFilterServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

#define DATA_FILTER_API_SLOT1_JSON "api/data/IDataFilterManagerSlot1.json"
#define DATA_FILTER_API_SLOT2_JSON "api/data/IDataFilterManagerSlot2.json"
#define DATA_FILTER_STATE_SLOT1_JSON "system-state/data/IDataFilterManagerStateSlot1.json"
#define DATA_FILTER_STATE_SLOT2_JSON "system-state/data/IDataFilterManagerStateSlot2.json"

#define SLOT_2 2

DataFilterServerImpl::DataFilterServerImpl(
    std::shared_ptr<DataConnectionServerImpl> dcmServerImpl)
    :dcmServerImpl_(dcmServerImpl) {

    LOG(DEBUG, __FUNCTION__);
}

DataFilterServerImpl::~DataFilterServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status DataFilterServerImpl::InitService(ServerContext* context,
    const dataStub::SlotInfo* request,
    dataStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    std::string filePath = (request->slot_id() == SLOT_2)? DATA_FILTER_API_SLOT2_JSON
        : DATA_FILTER_API_SLOT1_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataFilterManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataFilterManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    std::vector<std::string> filters = {"data_connection_server"};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);

    return grpc::Status::OK;
}

grpc::Status DataFilterServerImpl::SetDataRestrictMode(ServerContext* context,
    const dataStub::SetDataRestrictModeRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_API_SLOT2_JSON : DATA_FILTER_API_SLOT1_JSON;
    std::string stateJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_STATE_SLOT2_JSON : DATA_FILTER_STATE_SLOT1_JSON;
    std::string subsystem = "IDataFilterManager";
    std::string method = "setDataRestrictMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (!dcmServerImpl_->isAnyDataCallActive(static_cast<SlotId>(request->slot_id()))) {
        data.error = telux::common::ErrorCode::GENERIC_FAILURE;
        data.status = telux::common::Status::FAILED;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        std::string filterMode = convertFilterEnumToString(
            request->filter_mode().filter_mode());
        std::string autoExitMode = convertFilterEnumToString(
            request->filter_mode().filter_auto_exit());
        data.stateRootObj[subsystem]["requestDataRestrictMode"]["filter_mode"] = filterMode;
        data.stateRootObj[subsystem]["requestDataRestrictMode"]["filter_auto_exit"] = autoExitMode;

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        sendDataRestrictModeEvent(request->slot_id(), filterMode, autoExitMode);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataFilterServerImpl::GetDataRestrictMode(ServerContext* context,
    const dataStub::GetDataRestrictModeRequest* request,
    dataStub::GetDataRestrictModeReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_API_SLOT2_JSON : DATA_FILTER_API_SLOT1_JSON;
    std::string stateJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_STATE_SLOT2_JSON : DATA_FILTER_STATE_SLOT1_JSON;
    std::string subsystem = "IDataFilterManager";
    std::string method = "requestDataRestrictMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        response->mutable_filter_mode()->set_filter_mode(
            convertFilterStringToEnum(
            data.stateRootObj[subsystem][method]["filter_mode"].asString()));
        response->mutable_filter_mode()->set_filter_auto_exit(
            convertFilterStringToEnum(
            data.stateRootObj[subsystem][method]["filter_auto_exit"].asString()));
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataFilterServerImpl::AddDataRestrictFilter(ServerContext* context,
    const dataStub::AddDataRestrictFilterRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_API_SLOT2_JSON : DATA_FILTER_API_SLOT1_JSON;
    std::string stateJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_STATE_SLOT2_JSON : DATA_FILTER_STATE_SLOT1_JSON;
    std::string subsystem = "IDataFilterManager";
    std::string method = "addDataRestrictFilter";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (!dcmServerImpl_->isAnyDataCallActive(static_cast<SlotId>(request->slot_id()))) {
        data.error = telux::common::ErrorCode::GENERIC_FAILURE;
        data.status = telux::common::Status::FAILED;
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataFilterServerImpl::RemoveAllDataRestrictFilter(ServerContext* context,
    const dataStub::RemoveDataRestrictFilterRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_API_SLOT2_JSON : DATA_FILTER_API_SLOT1_JSON;
    std::string stateJsonPath = (request->slot_id() == SLOT_2)?
        DATA_FILTER_STATE_SLOT2_JSON : DATA_FILTER_STATE_SLOT1_JSON;
    std::string subsystem = "IDataFilterManager";
    std::string method = "removeAllDataRestrictFilters";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

std::string DataFilterServerImpl::convertFilterEnumToString(
    ::dataStub::DataRestrictMode::DataRestrictModeType status) {
    switch (status) {
        case ::dataStub::DataRestrictMode::ENABLE:
            return "ENABLE";
        case ::dataStub::DataRestrictMode::DISABLE:
            return "DISABLE";
        default:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

::dataStub::DataRestrictMode::DataRestrictModeType DataFilterServerImpl::convertFilterStringToEnum(
    std::string status) {

    if (status == "ENABLE") {
        return ::dataStub::DataRestrictMode::ENABLE;
    } else if (status == "DISABLE") {
        return ::dataStub::DataRestrictMode::DISABLE;
    }
    return ::dataStub::DataRestrictMode::UNKNOWN;
}

void DataFilterServerImpl::onServerEvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::dataStub::NoActiveDataCall>()) {
        ::dataStub::NoActiveDataCall callEvent;
        event.UnpackTo(&callEvent);

        std::string apiJsonPath = (callEvent.slot_id() == SLOT_2)?
            DATA_FILTER_API_SLOT2_JSON : DATA_FILTER_API_SLOT1_JSON;
        std::string stateJsonPath = (callEvent.slot_id() == SLOT_2)?
            DATA_FILTER_STATE_SLOT2_JSON : DATA_FILTER_STATE_SLOT1_JSON;
        std::string subsystem = "IDataFilterManager";
        std::string method = "setDataRestrictMode";
        JsonData data;
        telux::common::ErrorCode error =
            CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

        if (error != ErrorCode::SUCCESS) {
            return;
        }

        if (data.status == telux::common::Status::SUCCESS &&
            data.error == telux::common::ErrorCode::SUCCESS) {
            std::string mode = "DISABLE";
            data.stateRootObj[subsystem]["requestDataRestrictMode"]["filter_mode"]
                = mode;
            data.stateRootObj[subsystem]["requestDataRestrictMode"]["filter_auto_exit"]
                = mode;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
            sendDataRestrictModeEvent(callEvent.slot_id(), mode, mode);
        }
    }
}

void DataFilterServerImpl::sendDataRestrictModeEvent(int slot_id, std::string filterMode,
    std::string autoExitMode) {
    //sending event for notification on client side
    ::dataStub::SetDataRestrictModeRequest modeEvent;
    ::eventService::EventResponse anyResponse;

    modeEvent.set_slot_id(slot_id);
    modeEvent.mutable_filter_mode()->set_filter_mode(
        convertFilterStringToEnum(filterMode));
    modeEvent.mutable_filter_mode()->set_filter_auto_exit(
        convertFilterStringToEnum(autoExitMode));

    anyResponse.set_filter("data_filter");
    anyResponse.mutable_any()->PackFrom(modeEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}