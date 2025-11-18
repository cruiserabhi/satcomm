/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef VLAN_MANAGER_SERVER_HPP
#define VLAN_MANAGER_SERVER_HPP

#include <telux/data/net/VlanManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class VlanServerImpl final:
    public dataStub::VlanManager::Service {
public:
    VlanServerImpl();
    ~VlanServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::InitRequest* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status CreateVlan(ServerContext* context,
        const dataStub::CreateVlanRequest* request,
        dataStub::CreateVlanReply* response) override;

    grpc::Status RemoveVlan(ServerContext* context,
        const dataStub::RemoveVlanRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status QueryVlanInfo(ServerContext* context,
        const dataStub::QueryVlanInfoRequest* request,
        dataStub::QueryVlanInfoReply* response) override;

    grpc::Status BindToBackhaul(ServerContext* context,
        const dataStub::BindToBackhaulConfig* request,
        dataStub::DefaultReply* response) override;

    grpc::Status UnbindFromBackhaul(ServerContext* context,
        const dataStub::BindToBackhaulConfig* request,
        dataStub::DefaultReply* response) override;

    grpc::Status QueryVlanMappingList(ServerContext* context,
        const dataStub::QueryVlanMappingListRequest* request,
        dataStub::QueryVlanMappingListReply* response) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    template <typename T>
    bool isConfigAvailable(std::string subsystem, std::string method, const JsonData& data,
        const T* request, int& configIdx = 0) {
        const Json::Value& config = data.stateRootObj[subsystem][method];
        int count = config.size();
        bool isFound = false;
        int idx = 0;
        for (idx=0; idx < count; idx++) {
            if (config[idx]["vlanId"].asInt() != request->vlan_id()) {
                continue;
            }
            if (config[idx]["ifaceType"].asInt() != request->interface_type()) {
                continue;
            }
            isFound = true;
            break;
        }
        if (isFound) {
            configIdx = idx;
        }
        return isFound;
    }

    template <typename T>
    bool isBindingConfigAvailable(std::string subsystem, std::string method,
        const JsonData& data, const T* request, int& configIdx = 0) {
        const Json::Value& config = data.stateRootObj[subsystem][method];
        int count = config.size();
        bool isFound = false;
        int idx = 0;
        for (idx=0; idx < count; idx++) {
            if (config[idx]["vlanId"].asInt() != request->vlan_id()) {
                continue;
            }
            if (config[idx]["slotId"].asInt() != request->slot_id()) {
                continue;
            }
            if (config[idx]["profileId"].asInt() != request->profile_id()) {
                continue;
            }
            isFound = true;
            break;
        }
        if (isFound) {
            configIdx = idx;
        }
        return isFound;
    }

};

#endif //VLAN_MANAGER_SERVER_HPP