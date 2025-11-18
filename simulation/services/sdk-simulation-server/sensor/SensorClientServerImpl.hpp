/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorClientServerImpl.hpp
 *
 *
 */

#ifndef SENSOR_CLIENT_SERVER_HPP
#define SENSOR_CLIENT_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <map>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/sensor/SensorDefines.hpp>

#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/sensor/SensorDefinesStub.hpp"
#include "protos/proto-src/sensor_simulation.grpc.pb.h"
#include "common/FileBuffer.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using sensorStub::SensorClientService;

class SensorClientServerImpl final :
    public sensorStub::SensorClientService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<SensorClientServerImpl> {
 public:
    SensorClientServerImpl();
    ~SensorClientServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::GetServiceStatusReply* response);

    grpc::Status GetSensorList(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorInfoResponse* response);

    grpc::Status Configure(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status GetConfiguration(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status GetSensorInfo(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status Activate(ServerContext* context, const sensorStub::ActivateRequest* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status Deactivate(ServerContext* context, const sensorStub::DeactivateRequest* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status SensorUpdateRotationMatrix(ServerContext* context,
        const ::google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status SelfTest (ServerContext* context,
        const sensorStub::SelfTestRequest* request, sensorStub::SelfTestResponse* response);

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    void apiJsonReader(std::string apiName, sensorStub::SensorClientCommandReply* response);
    bool init();
    void updateSensorInfo();
    telux::sensor::SensorType getSensorType(std::string sensorType);
    void startStreaming();
    void updateStreamRequest();
    void triggerStreamingStoppedEvent();
    void handleEvent(std::string token , std::string event);
    void triggerSelfTestFailedEvent(std::string event);
    std::vector<telux::sensor::SensorInfo> sensorInfo_;
    std::shared_ptr<FileBuffer> fileBuffer_ = nullptr;
    std::vector<std::string> requestBuffer_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    bool bufferingInitialized_ = false;
    bool stopStreamingData_ = false;
    bool replayCsv_ = false;
    uint64_t previousTimestamp_ = 0;
    bool lastBatchStreamed_ = false;
    int activeAccelCount_ = 0;
    int activeGyroCount_ = 0;
    std::unordered_map<telux::sensor::SelfTestType, uint64_t> accelSelfTestCache_;
    std::unordered_map<telux::sensor::SelfTestType, uint64_t> gyroSelfTestCache_;
};
#endif  // SENSOR_FEATURE_MANAGER_SERVER_HPP
