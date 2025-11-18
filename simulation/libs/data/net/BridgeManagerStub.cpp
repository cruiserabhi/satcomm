/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "BridgeManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include <thread>
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {
namespace net {

BridgeManagerStub::BridgeManagerStub () {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IBridgeListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

BridgeManagerStub::~BridgeManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status BridgeManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void BridgeManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::BridgeManager>();

    google::protobuf::Empty request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

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

    bool isSubsystemReady = (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
        true : false;
    setSubSystemStatus(cbStatus);
    setSubsystemReady(isSubsystemReady);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void BridgeManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void BridgeManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void BridgeManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

std::future<bool> BridgeManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return BridgeManagerStub::waitForInitialization(); });
    return future;
}

bool BridgeManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

void BridgeManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus BridgeManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

bool BridgeManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

telux::common::Status BridgeManagerStub::registerListener(
    std::weak_ptr<IBridgeListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status BridgeManagerStub::deregisterListener(
    std::weak_ptr<IBridgeListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void BridgeManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IBridgeListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Bridge Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::ErrorCode BridgeManagerStub::setInterfaceBridge(InterfaceType ifaceType,
    uint32_t bridgeId) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " bridge manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (ifaceType < InterfaceType::AP_PRIMARY ||
        ifaceType > InterfaceType::AP_QUATERNARY) {
        return telux::common::ErrorCode::NOT_SUPPORTED;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    ::dataStub::SetInterfaceBridgeRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_interface_type(::dataStub::InterfaceType(ifaceType));
    request.set_bridge_id(bridgeId);
    grpc::Status reqStatus = stub_->SetInterfaceBridge(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());

    return error;
}

telux::common::ErrorCode BridgeManagerStub::getInterfaceBridge(InterfaceType ifaceType,
    uint32_t& bridgeId) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " bridge manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (ifaceType < InterfaceType::AP_PRIMARY ||
        ifaceType > InterfaceType::AP_QUATERNARY) {
        return telux::common::ErrorCode::NOT_SUPPORTED;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    ::dataStub::GetInterfaceBridgeRequest request;
    ::dataStub::GetInterfaceBridgeReply response;
    ClientContext context;

    request.set_interface_type(::dataStub::InterfaceType(ifaceType));
    grpc::Status reqStatus = stub_->GetInterfaceBridge(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.mutable_reply()->error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        bridgeId = response.bridge_id();
    }

    return error;
}

telux::common::Status BridgeManagerStub::enableBridge(bool enable,
    telux::common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status BridgeManagerStub::addBridge( BridgeInfo config,
    telux::common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status BridgeManagerStub::requestBridgeInfo(
    BridgeInfoResponseCb callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status BridgeManagerStub::removeBridge(std::string ifaceName,
    telux::common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

} // end of namespace net
} // end of namespace data
} // end of namespace telux
