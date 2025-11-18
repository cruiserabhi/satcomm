/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "NtnManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace satcom {

NtnManagerStub::NtnManagerStub()
{
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<INtnListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

NtnManagerStub::~NtnManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status NtnManagerStub::init(telux::common::InitResponseCb callback)
{
    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

void NtnManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::satcomStub::NtnManager>();
    ::satcomStub::GetServiceStatusReply response;
    ClientContext context;
    ::google::protobuf::Empty request{};
    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay = DEFAULT_DELAY;
    do {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " InitService request failed");
            break;
        }
        cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());
        this->onServiceStatusChange(cbStatus);
        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
    } while (0);
    setSubSystemStatus(cbStatus);
    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void NtnManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}
void NtnManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus NtnManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

void NtnManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::ErrorCode NtnManagerStub::isNtnSupported(bool &isSupported) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode NtnManagerStub::enableNtn(
    bool enable, bool isEmergency, const std::string &iccid) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode NtnManagerStub::abortData() {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode NtnManagerStub::getNtnCapabilities(NtnCapabilities &capabilities) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode NtnManagerStub::getSignalStrength(SignalStrength &signalStrength) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode NtnManagerStub::updateSystemSelectionSpecifiers(
    std::vector<SystemSelectionSpecifier> &params) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

NtnState NtnManagerStub::getNtnState() {
    LOG(DEBUG, __FUNCTION__);
    return NtnState::DISABLED;
}

telux::common::Status NtnManagerStub::sendData(uint8_t *data, uint32_t size, bool isEmergency,
    TransactionId &TransactionId)
{
    LOG(DEBUG, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode NtnManagerStub::enableCellularScan(bool enable) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

void NtnManagerStub::onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) {
    LOG(DEBUG, __FUNCTION__);
}

void NtnManagerStub::onDataAck(telux::common::ErrorCode err, telux::satcom::TransactionId id) {
    LOG(DEBUG, __FUNCTION__);
}

void NtnManagerStub::onSignalStrengthChange(telux::satcom::SignalStrength newStrength) {
    LOG(DEBUG, __FUNCTION__);
}

void NtnManagerStub::onCapabilitiesChange(telux::satcom::NtnCapabilities capabilities) {
    LOG(DEBUG, __FUNCTION__);
}

void NtnManagerStub::onNtnStateChange(telux::satcom::NtnState state) {
    LOG(DEBUG, __FUNCTION__);
}

void NtnManagerStub::onCellularCoverageAvailable(bool isCellularCoverageAvailable) {
    LOG(DEBUG, __FUNCTION__);
}

// Registration APIs
telux::common::Status NtnManagerStub::registerListener(std::weak_ptr<INtnListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status NtnManagerStub::deregisterListener(std::weak_ptr<INtnListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

}  // namespace satcom
}  // namespace telux
