/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SERVING_SYSTEM_SERVER_HPP
#define SERVING_SYSTEM_SERVER_HPP

#include <telux/data/ServingSystemManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ServingSystemServerImpl final:
    public dataStub::DataServingSystemManager::Service {
public:
    ServingSystemServerImpl();
    ~ServingSystemServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::SlotInfo* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status GetDrbStatus(ServerContext* context,
        const dataStub::GetDrbStatusRequest* request,
        dataStub::GetDrbStatusReply* response) override;

    grpc::Status RequestServiceStatus(ServerContext* context,
        const dataStub::ServingStatusRequest* request,
        dataStub::ServiceStatusReply* response) override;

    grpc::Status RequestRoamingStatus(ServerContext* context,
        const dataStub::RoamingStatusRequest* request,
        dataStub::RomingStatusReply* response) override;

    grpc::Status RequestNrIconType(ServerContext* context,
        const dataStub::NrIconTypeRequest* request,
        dataStub::NrIconTypeReply* response) override;

    grpc::Status MakeDormant(ServerContext* context,
        const dataStub::MakeDormantStatusRequest* request,
        dataStub::DefaultReply* response) override;

private:
    dataStub::DrbStatus::Status convertDrbStatusStringToEnum(std::string status);
    dataStub::DataServiceState::ServiceState convertServiceStateStringToEnum(
        std::string ServiceState);
    dataStub::NetworkRat::Rat convertNetworkRatStringToEnum(
        std::string nwRat);
    dataStub::RoamingType::Type convertRoamingTypeStringToEnum(
        std::string type);
    dataStub::NrIconType::Type convertNrIconTypeStringToEnum(
        std::string type);
};

#endif //SERVING_SYSTEM_SERVER_HPP