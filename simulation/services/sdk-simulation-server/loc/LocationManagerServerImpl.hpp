/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file LocationManagerServerImpl.hpp
 *
 *
 */

#ifndef LOC_MANAGER_SERVER_HPP
#define LOC_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/loc/LocationDefines.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/loc_simulation.grpc.pb.h"
#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#include "common/FileBuffer.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using locStub::LocationManagerService;

class LocationManagerServerImpl final : public locStub::LocationManagerService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<LocationManagerServerImpl> {
 public:
    LocationManagerServerImpl();
    ~LocationManagerServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        locStub::GetServiceStatusReply* response);
    grpc::Status StartBasicReports(ServerContext* context, const google::protobuf::Empty* request,
        locStub::LocManagerCommandReply* response);
    grpc::Status StartDetailedReports(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response);
    grpc::Status StartDetailedEngineReports(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response);
    grpc::Status StopReports(ServerContext* context, const google::protobuf::Empty* request,
        google::protobuf::Empty* response);
    grpc::Status RegisterLocationSystemInfo(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response);
    grpc::Status DeregisterLocationSystemInfo(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response);
    grpc::Status RequestEnergyConsumedInfo(ServerContext* context,
        const google::protobuf::Empty* request, locStub::RequestEnergyConsumedInfoReply* response);
    grpc::Status GetYearOfHw(ServerContext* context,
        const google::protobuf::Empty* request, locStub::GetYearOfHwReply* response);
    grpc::Status GetCapabilities(ServerContext* context,
        const google::protobuf::Empty* request, locStub::GetCapabilitiesReply* response);
    grpc::Status GetTerrestrialPosition(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response);
    grpc::Status CancelTerrestrialPosition(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response);
    grpc::Status GetLastLocation(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LastLocationInfo* response);
    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    void apiJsonReader(std::string apiName, locStub::LocManagerCommandReply* response);
    bool init();
    void startStreaming();
    void updateStreamRequest();
    void onEventUpdate(std::string event);
    void handleEvent(std::string token, std::string event);
    void handleCapabilitiesUpdate(std::string event);
    void triggerCapabilitiesUpdateEvent();
    void handleSysInfoUpdateCurrent(std::string event);
    void handleSysInfoUpdateLeapSecond(std::string event);
    void handleDisasterCrisisReport(std::string event);
    void triggerSysinfoUpdateEvent();
    void triggerStreamingStoppedEvent();
    void triggerResetWindowEvent();
    std::shared_ptr<FileBuffer> fileBuffer_ = nullptr;
    std::vector<std::string> requestBuffer_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    bool bufferingInitialized_ = false;
    bool stopStreamingData_ = false;
    bool replayCsv_ = false;
    uint64_t previousTimestamp_ = 0;
    std::string lastLocInfo_ = "";
    telux::loc::LocCapability capabilityMask_ = 0;
    uint32_t sysinfoValidity_ = 0x01;
    uint32_t leapsecondValidity_ = 0x03;
    uint32_t current_ = 18;
    uint32_t leapSecondsBeforeChange_ = 0;
    uint32_t leapSecondsAfterChange_ = 0;
    uint32_t gnssValidity_ = 0x3F;
    uint32_t systemWeek_ = 0;
    uint32_t systemMsec_ = 0;
    float systemClkTimeBias_ = 0.0;
    float systemClkTimeUncMs_ = 0.0;
    uint32_t refFCount_ = 0;
    uint32_t clockResets_ = 0;
};

#endif // LOC_MANAGER_SERVER_HPP
