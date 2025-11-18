/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerServerImpl.hpp
 *
 *
 */

#ifndef SENSOR_FEATURE_MANAGER_SERVER_HPP
#define SENSOR_FEATURE_MANAGER_SERVER_HPP

#include <memory>
#include <string>
#include <map>

#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"

#include "protos/proto-src/sensor_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using sensorStub::SensorFeatureManagerService;

class SensorFeatureManagerServerImpl final :
    public sensorStub::SensorFeatureManagerService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<SensorFeatureManagerServerImpl> {
 public:
    SensorFeatureManagerServerImpl();
    ~SensorFeatureManagerServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::GetServiceStatusReply* response);

    grpc::Status GetFeatureList(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::GetFeatureListReply* response);

    grpc::Status EnableFeature(ServerContext* context,
        const sensorStub::SensorEnableFeature* request,
        sensorStub::SensorFeatureManagerCommandReply* response);

    grpc::Status DisableFeature(ServerContext* context,
        const sensorStub::SensorEnableFeature* request,
        sensorStub::SensorFeatureManagerCommandReply* response);

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    void apiJsonReader(std::string apiName, sensorStub::SensorFeatureManagerCommandReply* response);
    void handleEvent(std::string token , std::string event);
    void handleFeatureEvent(std::string eventParams);
    void triggerFeatureEvent(std::string featureName, int id, std::string events);
    std::string readBufferedEventStringFromFile(std::string filename,int eventId);
    void onEventUpdate(std::string event);
    std::map<std::string, bool> featureStatusMap_;
    std::mutex mtx_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};
#endif  // SENSOR_FEATURE_MANAGER_SERVER_HPP