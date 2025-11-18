/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_CONTROL_MANAGER_SERVER_HPP
#define DATA_CONTROL_MANAGER_SERVER_HPP

#include <telux/data/DataControlManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include "event/ServerEventManager.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class DataControlServerImpl final:
    public dataStub::DualDataManager::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<DataControlServerImpl> {

public:
    DataControlServerImpl();
    ~DataControlServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::InitRequest* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetDataStallParams(ServerContext* context,
            dataStub::SetDataStallParamsReply request, dataStub::SetDataStallParamsReply* response)
private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

};

#endif //DATA_CONTROL_MANAGER_SERVER_HPP
