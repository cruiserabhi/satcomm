/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <grpcpp/grpcpp.h>

#include "Cv2xConfigStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace telux {
namespace cv2x {

static const std::string CV2X_CONFIG_FILTER = "cv2x_config";

void ConfigChangedListener::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::cv2xStub::ConfigEventInfo>()) {
        ::cv2xStub::ConfigEventInfo configEvt;
        event.UnpackTo(&configEvt);

        telux::cv2x::ConfigEventInfo config;
        config.source = static_cast<telux::cv2x::ConfigSourceType>(configEvt.source());
        config.event = static_cast<telux::cv2x::ConfigEvent>(configEvt.event());
        onConfigChanged(config);
    }
}

telux::common::Status ConfigChangedListener::registerListener(
    std::weak_ptr<telux::cv2x::ICv2xConfigListener> listener) {
    return listenerMgr_.registerListener(listener);
}

telux::common::Status ConfigChangedListener::deregisterListener(
    std::weak_ptr<telux::cv2x::ICv2xConfigListener> listener) {
    return listenerMgr_.deRegisterListener(listener);
}

void ConfigChangedListener::onConfigChanged(const ConfigEventInfo &info) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICv2xConfigListener>> applisteners;

    listenerMgr_.getAvailableListeners(applisteners);
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onConfigChanged(info);
        }
    }
}

Cv2xConfigStub::Cv2xConfigStub() {
    LOG(DEBUG, __FUNCTION__);
    exiting_ = false;
    taskQ_   = std::make_shared<AsyncTaskQueue<void>>();
    stub_    = CommonUtils::getGrpcStub<::cv2xStub::Cv2xConfigService>();
    configEvtListener_ = std::make_shared<ConfigChangedListener>();
}

Cv2xConfigStub::~Cv2xConfigStub() {
    LOG(DEBUG, __FUNCTION__);
    exiting_ = true;
    cv_.notify_all();

    if (configEvtListener_) {
       std::vector<std::string> filters = {CV2X_CONFIG_FILTER};
        auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
        clientEventManager.deregisterListener(configEvtListener_, filters);
     }
}

telux::common::Status Cv2xConfigStub::init(telux::common::InitResponseCb callback) {
    LOG(INFO, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void Cv2xConfigStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    ::cv2xStub::GetServiceStatusReply response;
    int delay = DEFAULT_DELAY;

    if (configEvtListener_) {
       std::vector<std::string> filters = {CV2X_CONFIG_FILTER};
        auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(configEvtListener_, filters);
     }

    CALL_RPC(stub_->initService, request, status, response, delay);
    {
        std::lock_guard<std::mutex> cvLock(mutex_);
        serviceStatus_ = static_cast<telux::common::ServiceStatus>(response.status());
    }

    if (status == telux::common::Status::FAILED) {
        LOG(DEBUG, __FUNCTION__, "Fail to init Cv2xConfigStub");
    }

    cv_.notify_all();

    if (callback && (delay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        callback(serviceStatus_);
    }
}

bool Cv2xConfigStub::isReady() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return telux::common::ServiceStatus::SERVICE_AVAILABLE == serviceStatus_;
}

std::future<bool> Cv2xConfigStub::onReady() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [this] {
        std::unique_lock<mutex> cvLock(mutex_);
        while (telux::common::ServiceStatus::SERVICE_UNAVAILABLE == serviceStatus_ && !exiting_) {
            cv_.wait(cvLock);
        }
        return telux::common::ServiceStatus::SERVICE_AVAILABLE == serviceStatus_;
    });
    return f;
}

telux::common::Status Cv2xConfigStub::updateConfiguration(
    const std::string &configFilePath, telux::common::ResponseCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::Cv2xCommandReply response;
    ::cv2xStub::Cv2xConfigPath path;
    path.set_path(configFilePath);

    CALL_RPC_AND_RESPOND(stub_->updateConfiguration, path, status, cb, taskQ_);

    return status;
}

telux::common::Status Cv2xConfigStub::retrieveConfiguration(
    const std::string &configFilePath, telux::common::ResponseCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::Cv2xCommandReply response;
    ::cv2xStub::Cv2xConfigPath path;
    path.set_path(configFilePath);

    CALL_RPC_AND_RESPOND(stub_->retrieveConfiguration, path, status, cb, taskQ_);

    return status;
}

telux::common::Status Cv2xConfigStub::registerListener(
    std::weak_ptr<ICv2xConfigListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    auto status = telux::common::Status::FAILED;
    if (configEvtListener_) {
        status = configEvtListener_->registerListener(listener);
    }

    return status;
}

telux::common::Status Cv2xConfigStub::deregisterListener(
    std::weak_ptr<ICv2xConfigListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    auto status = telux::common::Status::FAILED;
    if (configEvtListener_) {
        status = configEvtListener_->deregisterListener(listener);
    }

    return status;
}

telux::common::ServiceStatus Cv2xConfigStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<mutex> cvLock(mutex_);
    return serviceStatus_;
}

}  // namespace cv2x
}  // namespace telux
