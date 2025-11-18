/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "KeepAliveManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {

KeepAliveManagerStub::KeepAliveManagerStub (SlotId slotId) : slotId_(slotId) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IKeepAliveListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

KeepAliveManagerStub::~KeepAliveManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status KeepAliveManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void KeepAliveManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::KeepAliveManager>();

    ::dataStub::InitRequest request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
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

void KeepAliveManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void KeepAliveManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus KeepAliveManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

void KeepAliveManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IKeepAliveListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "KeepAlive Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status KeepAliveManagerStub::registerListener(
    std::weak_ptr<IKeepAliveListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status KeepAliveManagerStub::deregisterListener(
    std::weak_ptr<IKeepAliveListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

telux::common::ErrorCode  KeepAliveManagerStub::enableTCPMonitor(
    const TCPKAParams &tcpKaParams, MonitorHandleType &monHandle) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode  KeepAliveManagerStub::disableTCPMonitor(
    const MonitorHandleType monHandle) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode  KeepAliveManagerStub::startTCPKeepAliveOffload(
    const TCPKAParams &tcpKaParams, const TCPSessionParams &tcpSessionParams,
    const uint32_t interval, TCPKAOffloadHandle &handle) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode  KeepAliveManagerStub::startTCPKeepAliveOffload(
    const MonitorHandleType monHandle, const uint32_t interval, TCPKAOffloadHandle &handle) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode  KeepAliveManagerStub::stopTCPKeepAliveOffload(const TCPKAOffloadHandle handle) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

} // end of namespace data
} // end of namespace telux
