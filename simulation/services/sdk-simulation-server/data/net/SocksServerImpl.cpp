/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "SocksServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define SOCKS_MANAGER_API_LOCAL_JSON "api/data/ISocksManagerLocal.json"
#define SOCKS_MANAGER_STATE_JSON "system-state/data/ISocksManagerState.json"

#define REMOTE 1

SocksServerImpl::SocksServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

SocksServerImpl::~SocksServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status SocksServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = SOCKS_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["ISocksManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["ISocksManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status SocksServerImpl::enableSocks(ServerContext* context,
    const dataStub::EnableSocksRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SOCKS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = SOCKS_MANAGER_STATE_JSON;
    std::string subsystem = "ISocksManager";
    std::string method = "enableSocks";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        if (data.stateRootObj[subsystem]["sockConfig"]["enabled"].asBool() !=
             request->enable()) {
                data.stateRootObj[subsystem]["sockConfig"]["enabled"]
                    = request->enable();
        } else {
            data.error = telux::common::ErrorCode::NO_EFFECT;
        }
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}