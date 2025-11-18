/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "BridgeServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define BRIDGE_MANAGER_API_LOCAL_JSON "api/data/IBridgeManager.json"
#define BRIDGE_MANAGER_STATE_JSON "system-state/data/IBridgeManagerState.json"

BridgeServerImpl::BridgeServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

BridgeServerImpl::~BridgeServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status BridgeServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request,
    dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = BRIDGE_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IBridgeManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IBridgeManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status BridgeServerImpl::SetInterfaceBridge(ServerContext* context,
    const dataStub::SetInterfaceBridgeRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = BRIDGE_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = BRIDGE_MANAGER_STATE_JSON;
    std::string subsystem = "IBridgeManager";
    std::string method = "setInterfaceBridge";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        int entryIdx = 0;
        LOG(DEBUG, __FUNCTION__, " adding ifaceType::", request->interface_type(),
            " bridgeId::", request->bridge_id());
        Json::Value newBridgeConfigEntry;
        int currentEntryCount =
            data.stateRootObj[subsystem]["bridgeConfig"].size();
        bool entryExists = isBridgeConfigAvailable(subsystem, data, request, entryIdx);
        if (!entryExists) {
            newBridgeConfigEntry["ifaceType"] = request->interface_type();
            newBridgeConfigEntry["bridgeId"] = request->bridge_id();
            data.stateRootObj[subsystem]["bridgeConfig"][currentEntryCount] =
                newBridgeConfigEntry;
        } else {
            LOG(DEBUG, __FUNCTION__, " updating ifaceType::", request->interface_type(),
                " bridgeId::", request->bridge_id());
            data.stateRootObj[subsystem]["bridgeConfig"][entryIdx]["bridgeId"] =
                request->bridge_id();
        }
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status BridgeServerImpl::GetInterfaceBridge(ServerContext* context,
    const dataStub::GetInterfaceBridgeRequest* request,
    dataStub::GetInterfaceBridgeReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = BRIDGE_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = BRIDGE_MANAGER_STATE_JSON;
    std::string subsystem = "IBridgeManager";
    std::string method = "getInterfaceBridge";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        int entryIdx = 0;
        bool entryExists = isBridgeConfigAvailable(subsystem, data, request, entryIdx);
        if (!entryExists) {
            response->set_bridge_id(0);
        } else {
            response->set_bridge_id(
                data.stateRootObj[subsystem]["bridgeConfig"][entryIdx]["bridgeId"].asInt());
        }
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}