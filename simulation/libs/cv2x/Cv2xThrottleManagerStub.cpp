/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <grpcpp/grpcpp.h>

#include "Cv2xRadioHelperStub.hpp"
#include "Cv2xThrottleManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100

namespace telux {
namespace cv2x {

static const std::string CV2X_THROTTLE_FILTER = "throttle_mgr";

Cv2xThrottleEventListener::Cv2xThrottleEventListener(
    std::shared_ptr<telux::common::ListenerManager<
                        telux::cv2x::ICv2xThrottleManagerListener>> mgr) {
    listenerMgr_ = mgr;
}

void Cv2xThrottleEventListener::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICv2xThrottleManagerListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    if (event.Is<::cv2xStub::SanityEvent>()) {
        ::cv2xStub::SanityEvent sanityEvent;
        event.UnpackTo(&sanityEvent);
        for (auto iter = listeners.begin(); iter != listeners.end(); ++iter) {
            auto it = (*iter).lock();
            if (nullptr != it) {
                it->onSanityStateUpdate(sanityEvent.state() == 1 ? true : false);
            }
        }
    } else if (event.Is<::cv2xStub::FilterEvent>()) {
        ::cv2xStub::FilterEvent filterEvent;
        event.UnpackTo(&filterEvent);

        for (auto iter = listeners.begin(); iter != listeners.end(); ++iter) {
            auto it = (*iter).lock();
            if (nullptr != it) {
                it->onFilterRateAdjustment(filterEvent.filter());
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " unknown event");
    }
}


Cv2xThrottleManagerStub::Cv2xThrottleManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<::cv2xStub::Cv2xThrottleManagerService>();
    listenerMgr_ =
        std::make_shared<telux::common::ListenerManager<
            telux::cv2x::ICv2xThrottleManagerListener>>();
    throttleEvtListener_ = std::make_shared<Cv2xThrottleEventListener>(listenerMgr_);
}

Cv2xThrottleManagerStub::~Cv2xThrottleManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status Cv2xThrottleManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();
    taskQ_.add(f);

    return telux::common::Status::SUCCESS;
}

void Cv2xThrottleManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::string> filters = {CV2X_THROTTLE_FILTER};
    auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
    clientEventManager.registerListener(throttleEvtListener_, filters);

    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    ::cv2xStub::GetServiceStatusReply response;
    int delay = DEFAULT_DELAY;

    CALL_RPC(stub_->initService, request, status, response, delay);

    serviceStatus_ = static_cast<telux::common::ServiceStatus>(response.status());
    if (status == telux::common::Status::FAILED) {
        LOG(DEBUG, __FUNCTION__, "Fail to init Cv2xThrottleManagerStub");
    }

    if (callback && (delay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        callback(serviceStatus_);
    }
}

telux::common::ServiceStatus Cv2xThrottleManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return serviceStatus_;
}

telux::common::Status Cv2xThrottleManagerStub::registerListener(
    std::weak_ptr<ICv2xThrottleManagerListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status Cv2xThrottleManagerStub::deregisterListener(
    std::weak_ptr<ICv2xThrottleManagerListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

telux::common::Status Cv2xThrottleManagerStub::setVerificationLoad(
    int load, setVerificationLoadCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async,
                     [cb]() { cb(telux::common::ErrorCode::SUCCESS); }).share();

    taskQ_.add(f);

    return telux::common::Status::SUCCESS;
}

}  // namespace cv2x
}  // namespace telux
