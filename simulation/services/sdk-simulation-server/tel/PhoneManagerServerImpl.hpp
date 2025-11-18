/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file      PhoneManagerServerImpl.hpp
 *
 */

#ifndef TELUX_TEL_PHONEMANAGERSERVERIMPL_HPP
#define TELUX_TEL_PHONEMANAGERSERVERIMPL_HPP

#include <memory>
#include <string>
#include <telux/common/CommonDefines.hpp>
#include "event/ServerEventManager.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "event/EventService.hpp"
#include "common/ModemManagerImpl.hpp"

using grpc::ServerContext;

class PhoneManagerServerImpl final : public telStub::PhoneService::Service,
                                     public IServerEventListener,
                                     public std::enable_shared_from_this<PhoneManagerServerImpl> {
 public:
    PhoneManagerServerImpl();
    ~PhoneManagerServerImpl();
    grpc::Status GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty *request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status IsSubsystemReady(ServerContext* context,
        const google::protobuf::Empty *request,
        commonStub::IsSubsystemReadyReply* response) override;
    grpc::Status GetPhoneIds(ServerContext *context,
        const google::protobuf::Empty *request,
        telStub::GetPhoneIdsReply* response) override;
    grpc::Status GetPhoneId(ServerContext *context,
        const google::protobuf::Empty *request,
        telStub::GetPhoneIdReply* response) override;
    grpc::Status GetCellularCapabilities(ServerContext *context,
        const google::protobuf::Empty *request,
        telStub::CellularCapabilityInfoReply* response) override;
    grpc::Status SetOperatingMode(ServerContext* context,
        const telStub::SetOperatingModeRequest* request,
        telStub::SetOperatingModeReply* response) override;
    grpc::Status GetOperatingMode(ServerContext* context, const google::protobuf::Empty* request,
        telStub::GetOperatingModeReply* response) override;
    grpc::Status ResetWwan(ServerContext* context,
        const google::protobuf::Empty* request, telStub::ResetWwanReply* response) override;
    grpc::Status RequestVoiceServiceState(ServerContext* context,
        const telStub::RequestVoiceServiceStateRequest* request,
        telStub::RequestVoiceServiceStateReply* response) override;
    grpc::Status SetRadioPower(ServerContext* context,
        const telStub::SetRadioPowerRequest* request,
        telStub::SetRadioPowerReply* response) override;
    grpc::Status RequestCellInfoList(ServerContext* context,
        const telStub::RequestCellInfoListRequest* request,
        telStub::RequestCellInfoListReply* response) override;
    grpc::Status SetCellInfoListRate(ServerContext* context,
        const telStub::SetCellInfoListRateRequest* request,
        telStub::SetCellInfoListRateReply* response) override;
    grpc::Status GetSignalStrength(ServerContext* context,
        const telStub::GetSignalStrengthRequest* request,
        telStub::GetSignalStrengthReply* response) override;
    grpc::Status SetECallOperatingMode(ServerContext* context,
        const telStub::SetECallOperatingModeRequest* request,
        telStub::SetECallOperatingModeReply* response) override;
    grpc::Status GetECallOperatingMode(ServerContext* context,
        const telStub::GetECallOperatingModeRequest* request,
        telStub::GetECallOperatingModeReply* response) override;
    grpc::Status RequestOperatorInfo(ServerContext* context,
        const telStub::RequestOperatorInfoRequest* request,
        telStub::RequestOperatorInfoReply* response) override;
    grpc::Status ConfigureSignalStrength(ServerContext* context,
        const telStub::ConfigureSignalStrengthRequest* request,
        telStub::ConfigureSignalStrengthReply* response) override;
    grpc::Status ConfigureSignalStrengthEx(ServerContext* context,
        const telStub::ConfigureSignalStrengthExRequest* request,
        telStub::ConfigureSignalStrengthExReply* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message);

private:
    grpc::Status InitService(ServerContext *context, const ::google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override ;
    //Read only API JSON Data
    telux::common::ServiceStatus readSubsystemStatus(int slotId);
    telux::common::ServiceStatus readSubsystemStatus(int slotId, int &cbDelay);
    void onEventUpdate(std::string event);
    void handleSignalStrengthChanged(std::string eventParams);
    void handleCellInfoChanged(std::string eventParams);
    void handleVoiceServiceStateChanged(std::string eventParams);
    void handleOperatingModeChanged(std::string eventParams);
    void notifyAndUpdateOperatingMode(int operatingMode);
    void handleECallOperatingModeChanged(std::string eventParams);
    void handleOperatorInfoChanged(std::string eventParams);
    void triggerChangeEvent(::eventService::EventResponse anyResponse);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ModemManagerImpl> modemMgr_;
};

#endif // TELUX_TEL_PHONEMANAGERSERVERIMPL_HPP
