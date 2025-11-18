/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_FILTER_SERVER_HPP
#define DATA_FILTER_SERVER_HPP

#include <telux/data/DataFilterManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "data/DataConnectionServerImpl.hpp"
#include "event/ServerEventManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class DataFilterServerImpl final:
    public dataStub::DataFilterManager::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<DataFilterServerImpl> {

public:
    DataFilterServerImpl(std::shared_ptr<DataConnectionServerImpl> dcmServerImpl);
    ~DataFilterServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::SlotInfo* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetDataRestrictMode(ServerContext* context,
        const dataStub::SetDataRestrictModeRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status GetDataRestrictMode(ServerContext* context,
        const dataStub::GetDataRestrictModeRequest* request,
        dataStub::GetDataRestrictModeReply* response) override;

    grpc::Status AddDataRestrictFilter(ServerContext* context,
        const dataStub::AddDataRestrictFilterRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RemoveAllDataRestrictFilter(ServerContext* context,
        const dataStub::RemoveDataRestrictFilterRequest* request,
        dataStub::DefaultReply* response) override;

    void onServerEvent(google::protobuf::Any event) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<DataConnectionServerImpl> dcmServerImpl_;

    ::dataStub::DataRestrictMode::DataRestrictModeType convertFilterStringToEnum(
        std::string status);
    std::string convertFilterEnumToString(
        ::dataStub::DataRestrictMode::DataRestrictModeType status);
    void sendDataRestrictModeEvent(int slot_id, std::string filterMode, std::string autoExitMode);
};

#endif //DATA_FILTER_SERVER_HPP