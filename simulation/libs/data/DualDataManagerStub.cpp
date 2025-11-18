/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DualDataManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define DUAL_DATA_FILTER "dual_data"

namespace telux {
namespace data {

DualDataManagerStub::DualDataManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IDualDataListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

DualDataManagerStub::~DualDataManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status DualDataManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void DualDataManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::DualDataManager>();

    ::dataStub::InitRequest request;
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

    setSubSystemStatus(cbStatus);
    if(cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {DUAL_DATA_FILTER};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(shared_from_this(), filters);
    }

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void DualDataManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void DualDataManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus DualDataManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status DualDataManagerStub::registerListener(
    std::weak_ptr<IDualDataListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status DualDataManagerStub::deregisterListener(
    std::weak_ptr<IDualDataListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void DualDataManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDualDataListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "DualData Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::ErrorCode DualDataManagerStub::getDualDataCapability(bool &isCapable) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " DualData manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::dataStub::GetDualDataCapabilityReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->GetDualDataCapability(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " getDualDataCapability request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        } else {
            isCapable = response.capability();
        }
    }

    return error;
}

telux::common::ErrorCode DualDataManagerStub::getDualDataUsageRecommendation(
    DualDataUsageRecommendation &recommendation) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " DualData manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::dataStub::GetDualDataUsageRecommendationReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->GetDualDataUsageRecommendation(&context, request, &response);

    telux::common::ErrorCode error  =
        static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " getDualDataUsageRecommendation request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        } else {
            recommendation =
                (DualDataUsageRecommendation)response.usage_recommendation().recommendation();
        }
    }

    return error;
}

void DualDataManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::dataStub::DualDataCapabilityEvent>()) {
        ::dataStub::DualDataCapabilityEvent capabilityEvent;
        event.UnpackTo(&capabilityEvent);
        this->handleCapabilityChangeEvent(capabilityEvent);
    } else if (event.Is<::dataStub::DualDataUsageRecommendationEvent>()) {
        ::dataStub::DualDataUsageRecommendationEvent recommendationEvent;
        event.UnpackTo(&recommendationEvent);
        this->handleRecommendationChangeEvent(recommendationEvent);
    }
}

void DualDataManagerStub::handleCapabilityChangeEvent(
    ::dataStub::DualDataCapabilityEvent capabilityEvent) {
    LOG(DEBUG, __FUNCTION__);
    bool capability = capabilityEvent.capability();

    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDualDataListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG,
                    "DualData Manager: invoking onDualDataCapabilityChange");
                sp->onDualDataCapabilityChange(capability);
            }
        }
    }
}

void DualDataManagerStub::handleRecommendationChangeEvent(
    ::dataStub::DualDataUsageRecommendationEvent recommendationEvent) {
    LOG(DEBUG, __FUNCTION__);
    std::string recommendation = recommendationEvent.recommendation();
    DualDataUsageRecommendation dppdRecommendation;
    if (recommendation == "ALLOWED") {
        dppdRecommendation = DualDataUsageRecommendation::ALLOWED;
    } else if (recommendation == "NOT_ALLOWED") {
        dppdRecommendation = DualDataUsageRecommendation::NOT_ALLOWED;
    } else if (recommendation == "NOT_RECOMMENDED") {
        dppdRecommendation = DualDataUsageRecommendation::NOT_RECOMMENDED;
    }
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDualDataListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG,
                    "DualData Manager: invoking onDualDataUsageRecommendationChange");
                sp->onDualDataUsageRecommendationChange(dppdRecommendation);
            }
        }
    }
}

telux::common::Status DualDataManagerStub::requestDdsSwitch(DdsInfo request,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DualDataManagerStub::requestCurrentDds(RequestCurrentDdsRespCb callback) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode DualDataManagerStub::configureDdsSwitchRecommendation(
    const DdsSwitchRecommendationConfig recommendationConfig) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode DualDataManagerStub::getDdsSwitchRecommendation(
    DdsSwitchRecommendation &ddsSwitchRecommendation) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

} // end of namespace data
} // end of namespace telux