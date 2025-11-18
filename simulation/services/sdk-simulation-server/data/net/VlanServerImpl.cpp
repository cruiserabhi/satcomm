/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "VlanServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/data/DataUtilsStub.hpp"

#define VLAN_MANAGER_API_LOCAL_JSON "api/data/IVlanManagerLocal.json"
#define VLAN_MANAGER_STATE_JSON "system-state/data/IVlanManagerState.json"

#define REMOTE 1

VlanServerImpl::VlanServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

VlanServerImpl::~VlanServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status VlanServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = VLAN_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IVlanManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IVlanManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status VlanServerImpl::CreateVlan(ServerContext* context,
    const dataStub::CreateVlanRequest* request, dataStub::CreateVlanReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = VLAN_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = VLAN_MANAGER_STATE_JSON;
    std::string subsystem = "IVlanManager";
    std::string method = "createVlan";
    JsonData data = {0};
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    std::string nwType = DataUtilsStub::convertNetworkTypeToString(request->nw_type());
    bool createBridge = request->create_bridge();
    auto ifType = request->interface_type();

    if ( (ifType == ::dataStub::InterfaceType::WLAN)  ||
         (ifType == ::dataStub::InterfaceType::RNDIS) ||
         (ifType == ::dataStub::InterfaceType::MHI)) {
        data.error = telux::common::ErrorCode::NOT_SUPPORTED;
    }

    if (nwType == "WAN" && createBridge) {
        data.error = telux::common::ErrorCode::INVALID_ARG;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        int idx = 0;
        bool isFound = isConfigAvailable(subsystem, "vlanConfig", data, request, idx);

        if (isFound) {
            data.error = telux::common::ErrorCode::NO_EFFECT;
        } else {
            const Json::Value& config = data.stateRootObj[subsystem]["vlanConfig"];
            int count = config.size();
            Json::Value newConfig;
            newConfig["ifaceType"] = request->interface_type();
            newConfig["vlanId"] = request->vlan_id();
            newConfig["isAccelerated"] = request->is_accelerated();
            newConfig["priority"] = request->priority();
            newConfig["createBridge"] = createBridge;
            newConfig["networkType"] = nwType;
            data.stateRootObj[subsystem]["vlanConfig"][count] = newConfig;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_is_accelerated(request->is_accelerated());
    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status VlanServerImpl::RemoveVlan(ServerContext* context,
    const dataStub::RemoveVlanRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = VLAN_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = VLAN_MANAGER_STATE_JSON;
    std::string subsystem = "IVlanManager";
    std::string method = "removeVlan";
    JsonData data = {0};
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

        int idx = 0;
        bool isFound = isConfigAvailable(subsystem, "vlanConfig", data, request, idx);

        if (isFound) {
            int currentCount =
                data.stateRootObj[subsystem]["vlanConfig"].size();
            int newCount = 0;
            int index = 0;
            Json::Value newRoot;
            for (; index < currentCount; ++index) {
                //skipping to add entry in new array for the matched index.
                if (idx == index ) {
                    continue;
                }
                newRoot[subsystem]["vlanConfig"][newCount]
                    = data.stateRootObj[subsystem]["vlanConfig"][index];
                newCount++;
            }
            data.stateRootObj[subsystem]["vlanConfig"]
                = newRoot[subsystem]["vlanConfig"];
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            data.error = telux::common::ErrorCode::NO_EFFECT;
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status VlanServerImpl::QueryVlanInfo(ServerContext* context,
    const dataStub::QueryVlanInfoRequest* request,
    dataStub::QueryVlanInfoReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = VLAN_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = VLAN_MANAGER_STATE_JSON;
    std::string subsystem = "IVlanManager";
    std::string method = "queryVlanInfo";
    JsonData data = {0};
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

        int currentCount =
            data.stateRootObj[subsystem]["vlanConfig"].size();
        int configIdx = 0;
        for (; configIdx < currentCount; ++configIdx) {
            Json::Value requestedConfig =
                data.stateRootObj[subsystem]["vlanConfig"][configIdx];
            dataStub::VlanConfig *config = response->add_vlan_config();
            config->set_interface_type(
                (::dataStub::InterfaceType)requestedConfig["ifaceType"].asInt());
            config->set_vlan_id(requestedConfig["vlanId"].asInt());
            config->set_is_accelerated(requestedConfig["isAccelerated"].asBool());
            config->set_priority(requestedConfig["priority"].asInt());
            config->set_create_bridge(requestedConfig["createBridge"].asBool());
            config->mutable_nw_type()->set_nw_type(DataUtilsStub::convertNetworkTypeToGrpc(
                        requestedConfig["networkType"].asString()));
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status VlanServerImpl::BindToBackhaul(ServerContext* context,
    const dataStub::BindToBackhaulConfig* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = VLAN_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = VLAN_MANAGER_STATE_JSON;
    std::string subsystem = "IVlanManager";
    std::string method = "bindToBackhaul";
    JsonData data = {0};
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

        int idx = 0;
        bool isFound = isBindingConfigAvailable(
            subsystem, "vlanBindConfig", data, request, idx);

        if (isFound) {
            data.error = telux::common::ErrorCode::NO_EFFECT;
        } else {
            const Json::Value& config = data.stateRootObj[subsystem]["vlanBindConfig"];
            int count = config.size();
            Json::Value newConfig;
            newConfig["backhaul"] = DataUtilsStub::convertEnumToBackhaulPrefString(
                    static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
            newConfig["vlanId"] = request->vlan_id();
            newConfig["slotId"] = request->slot_id();
            newConfig["profileId"] = request->profile_id();
            newConfig["backhaul_vlanId"] = request->backhaul_vlan_id();
            data.stateRootObj[subsystem]["vlanBindConfig"][count] = newConfig;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status VlanServerImpl::UnbindFromBackhaul(ServerContext* context,
    const dataStub::BindToBackhaulConfig* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = VLAN_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = VLAN_MANAGER_STATE_JSON;
    std::string subsystem = "IVlanManager";
    std::string method = "unbindFromBackhaul";
    JsonData data = {0};
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

        int idx = 0;
        bool isFound = isBindingConfigAvailable(
            subsystem, "vlanBindConfig", data, request, idx);

        if (isFound) {
            int currentCount =
                data.stateRootObj[subsystem]["vlanBindConfig"].size();
            int newCount = 0;
            int index = 0;
            Json::Value newRoot;
            for (; index < currentCount; ++index) {
                //skipping to add entry in new array for the matched index.
                if (idx == index ) {
                    continue;
                }
                newRoot[subsystem]["vlanBindConfig"][newCount]
                    = data.stateRootObj[subsystem]["vlanBindConfig"][index];
                newCount++;
            }
            data.stateRootObj[subsystem]["vlanBindConfig"]
                = newRoot[subsystem]["vlanBindConfig"];
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            data.error = telux::common::ErrorCode::NO_EFFECT;
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status VlanServerImpl::QueryVlanMappingList(ServerContext* context,
    const dataStub::QueryVlanMappingListRequest* request,
    dataStub::QueryVlanMappingListReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = VLAN_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = VLAN_MANAGER_STATE_JSON;
    std::string subsystem = "IVlanManager";
    std::string method = "queryVlanToBackhaulBindings";
    JsonData data = {0};
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

        int currentCount =
            data.stateRootObj[subsystem]["vlanBindConfig"].size();
        int bindIdx = 0;
        for (; bindIdx < currentCount; ++bindIdx) {
            Json::Value requestedBinding =
                data.stateRootObj[subsystem]["vlanBindConfig"][bindIdx];
            auto slotId = requestedBinding["slotId"].asInt();
            std::string reqBackhaul = DataUtilsStub::convertEnumToBackhaulPrefString(
                static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
            auto backhaul = requestedBinding["backhaul"].asString();
            if ((slotId == request->slot_id()) && (reqBackhaul == backhaul)) {
                dataStub::VlanMapping *config = response->add_vlan_mapping();
                config->set_vlan_id(requestedBinding["vlanId"].asInt());
                config->set_profile_id(requestedBinding["profileId"].asInt());
                config->set_backhaul_vlan_id(requestedBinding["backhaul_vlanId"].asInt());
            }
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}
