/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file      ModemManagerImpl.hpp
 *
 */

#ifndef TELUX_TEL_MODEMMANAGERIMPL_HPP
#define TELUX_TEL_MODEMMANAGERIMPL_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "event/EventService.hpp"
#include "libs/common/ModemManager.hpp"
#include "tel/OperatingModeTransitionManager.hpp"

using grpc::ServerContext;

namespace telux {
namespace common {

class ModemManagerImpl : public IServerEventListener,
                         public telux::common::IModemManager,
                         public std::enable_shared_from_this<ModemManagerImpl> {
 public:
    ModemManagerImpl();
    ~ModemManagerImpl();
    grpc::Status init();
    grpc::Status setOperatingMode(const telStub::SetOperatingModeRequest* request,
        telStub::SetOperatingModeReply* response);
    grpc::Status getOperatingMode(const google::protobuf::Empty* request,
        telStub::GetOperatingModeReply* response);
    grpc::Status resetWwan(ServerContext* context,
        const google::protobuf::Empty* request, telStub::ResetWwanReply* response);
    grpc::Status setRadioPower(ServerContext* context,
        const telStub::SetRadioPowerRequest* request,
        telStub::SetRadioPowerReply* response);
    grpc::Status setCellInfoListRate(ServerContext* context,
        const telStub::SetCellInfoListRateRequest* request,
        telStub::SetCellInfoListRateReply* response);
    grpc::Status setECallOperatingMode(ServerContext* context,
        const telStub::SetECallOperatingModeRequest* request,
        telStub::SetECallOperatingModeReply* response);
    grpc::Status getECallOperatingMode(ServerContext* context,
        const telStub::GetECallOperatingModeRequest* request,
        telStub::GetECallOperatingModeReply* response);
    void onEventUpdate(::eventService::UnsolicitedEvent message);
    void updateSignalStrength(int slotId);
    void updateOperatingModeState(int operatingMode);
    telux::common::ErrorCode getVoiceServiceState(int slotId, telStub::VoiceServiceStateInfo&
        serviceInfo);
    telux::common::ErrorCode getSystemInfo(int slotId, telStub::RadioTechnology &servingRat,
        telStub::ServiceDomainInfo_Domain &servingDomain);
    telux::common::ErrorCode getEcallOperatingMode(int slotId, telStub::ECallMode& mode);
private:
    std::shared_ptr<telux::tel::OperatingModeTransitionManager> operatingModeMgr_;
};

}  // namespace common
}  // namespace telux

#endif // TELUX_TEL_MODEMMANAGERIMPL_HPP
