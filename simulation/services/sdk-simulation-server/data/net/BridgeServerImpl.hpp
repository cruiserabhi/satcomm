/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef BRIDGE_MANAGER_SERVER_HPP
#define BRIDGE_MANAGER_SERVER_HPP

#include <telux/data/net/BridgeManager.hpp>

#include "libs/common/CommonUtils.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class BridgeServerImpl final:
    public dataStub::BridgeManager::Service {
public:
    BridgeServerImpl();
    ~BridgeServerImpl();

    grpc::Status InitService(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetInterfaceBridge(ServerContext* context,
        const dataStub::SetInterfaceBridgeRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status GetInterfaceBridge(ServerContext* context,
        const dataStub::GetInterfaceBridgeRequest* request,
        dataStub::GetInterfaceBridgeReply* response) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    template <typename T>
    bool isBridgeConfigAvailable(std::string subsystem, const JsonData& data,
        const T* request, int& entryIdx = 0) {
        LOG(DEBUG, __FUNCTION__);
        bool entryExists = false;

        int currentEntryCount =
            data.stateRootObj[subsystem]["bridgeConfig"].size();
        auto ifaceType = request->interface_type();

        int index = 0;
        for (; index < currentEntryCount; index++) {
            Json::Value currentEntry =
                data.stateRootObj[subsystem]
                ["bridgeConfig"][index];

            if (currentEntry["ifaceType"].asInt() != ifaceType) {
                continue;
            }

            entryIdx = index;
            entryExists = true;
            break;
        }
        return entryExists;
    }
};

#endif //BRIDGE_MANAGER_SERVER_HPP