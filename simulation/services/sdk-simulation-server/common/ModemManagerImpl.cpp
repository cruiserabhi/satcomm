/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/ServerEventManager.hpp"
#include "ModemManagerImpl.hpp"
#include "tel/TelUtil.hpp"

namespace telux {
namespace common {

ModemManagerImpl::ModemManagerImpl() {
    LOG(DEBUG, __FUNCTION__);
    operatingModeMgr_ = std::make_shared<telux::tel::OperatingModeTransitionManager>();
}

grpc::Status ModemManagerImpl::init() {
    //This needs to be called only once.
    grpc::Status status = operatingModeMgr_->init();
    operatingModeMgr_->start();
    return status;
}

ModemManagerImpl::~ModemManagerImpl() {
    LOG(DEBUG, __FUNCTION__, " Destructor called");
    if (operatingModeMgr_ != nullptr) {
        operatingModeMgr_->stop();
        operatingModeMgr_ = nullptr;
    }
}

grpc::Status ModemManagerImpl::setOperatingMode(const telStub::SetOperatingModeRequest* request,
    telStub::SetOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telStub::OperatingMode mode = request->operating_mode();
    JsonData data = telux::tel::TelUtil::writeOperatingModeToJsonFileAndReply(mode, response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in setting operating mode");
    }
    data.error = operatingModeMgr_->updateOperatingMode(mode);
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    //Send common indication to other managers on server side
    telStub::OperatingModeEvent opModeEvent = telux::tel::TelUtil::createOperatingModeEvent(mode);
    ::eventService::ServerEvent event;
    event.set_filter(MODEM_FILTER);
    event.mutable_any()->PackFrom(opModeEvent);
    auto& serverEventManager = ServerEventManager::getInstance();
    serverEventManager.sendServerEvent(event);
    return grpc::Status::OK;
}

grpc::Status ModemManagerImpl::getOperatingMode(const google::protobuf::Empty* request,
    telStub::GetOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data = telux::tel::TelUtil::readOperatingModeRespFromJsonFile(response);
    if (data.status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " Error in getting operating mode");
    }
    return grpc::Status::OK;
}

grpc::Status ModemManagerImpl::resetWwan(ServerContext* context,
    const google::protobuf::Empty* request, telStub::ResetWwanReply* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, " Not Supported");
}

grpc::Status ModemManagerImpl::setRadioPower(ServerContext* context,
    const telStub::SetRadioPowerRequest* request,
    telStub::SetRadioPowerReply* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, " Not Supported");
}

grpc::Status ModemManagerImpl::setCellInfoListRate(ServerContext* context,
    const telStub::SetCellInfoListRateRequest* request,
    telStub::SetCellInfoListRateReply* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, " Not Supported");
}

grpc::Status ModemManagerImpl::setECallOperatingMode(ServerContext* context,
    const telStub::SetECallOperatingModeRequest* request,
    telStub::SetECallOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, " Not Supported");
}

grpc::Status ModemManagerImpl::getECallOperatingMode(ServerContext* context,
    const telStub::GetECallOperatingModeRequest* request,
    telStub::GetECallOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, " Not Supported");
}

void ModemManagerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    LOG(DEBUG, __FUNCTION__);
}

void ModemManagerImpl::updateSignalStrength(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    operatingModeMgr_->updateCachedSignalStrength(slotId);
}

void ModemManagerImpl::updateOperatingModeState(int mode) {
    LOG(DEBUG, __FUNCTION__);
    operatingModeMgr_->updateOperatingMode(static_cast<telStub::OperatingMode>(mode));
}

telux::common::ErrorCode ModemManagerImpl::getVoiceServiceState(int slotId,
    telStub::VoiceServiceStateInfo & serviceInfo) {
    telStub::VoiceServiceStateEvent event;
    telux::common::ErrorCode error = telux::tel::TelUtil::readVoiceServiceStateEventFromJsonFile(slotId,
        event);
    serviceInfo = event.voice_service_state_info();
    return error;
}

telux::common::ErrorCode ModemManagerImpl::getSystemInfo(int slotId,
    telStub::RadioTechnology &servingRat, telStub::ServiceDomainInfo_Domain &servingDomain) {
    telux::common::ErrorCode error = telux::tel::TelUtil::readSystemInfoFromJsonFile(slotId,
        servingRat, servingDomain);
    return error;
}

telux::common::ErrorCode ModemManagerImpl::getEcallOperatingMode(int slotId,
    telStub::ECallMode& mode){
    telStub::ECallModeInfoChangeEvent event;
    telux::common::ErrorCode error = telux::tel::TelUtil::readEcallOperatingModeEventFromJsonFile(slotId,
        event);
    mode = event.ecall_mode();
    return error;
}

}  // namespace common
}  // namespace telux