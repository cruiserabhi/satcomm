/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <grpcpp/grpcpp.h>

#include "Cv2xRadioManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace telux {
namespace cv2x {

void Cv2xEvtListener::onCv2xStatusChange(telux::cv2x::Cv2xStatus &status) {
    LOG(DEBUG, __FUNCTION__);
    telux::cv2x::Cv2xStatusEx statusEx;
    std::vector<std::weak_ptr<ICv2xListener>> applisteners;

    statusEx.status = status;
    listenerMgr_.getAvailableListeners(applisteners);
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onStatusChanged(status);
            sp->onStatusChanged(statusEx);
        }
    }
}


void Cv2xEvtListener::onSlssRxInfoChange(const ::cv2xStub::SyncRefUeInfo& rcpSlssUe) {
    LOG(DEBUG, __FUNCTION__);
    cv2x::SyncRefUeInfo cv2xUeInfo;
    cv2x::Cv2xRadioHelper::rpcSlssInfoToSlssInfo(rcpSlssUe, cv2xUeInfo);

    cv2x::SlssRxInfo info;
    info.ueInfo.push_back(cv2xUeInfo);
    NOTIFY_LISTENER(listenerMgr_, ICv2xListener, onSlssRxInfoChanged, info);
}

void Cv2xEvtListener::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::cv2xStub::Cv2xStatus>()) {
        ::cv2xStub::Cv2xStatus stubStatus;
        event.UnpackTo(&stubStatus);

        telux::cv2x::Cv2xStatus cv2xStatus;
        RPC_TO_CV2X_STATUS(stubStatus, cv2xStatus);
        onCv2xStatusChange(cv2xStatus);
    } else if (event.Is<::cv2xStub::SyncRefUeInfo>()) {
        ::cv2xStub::SyncRefUeInfo rcpSlssUe;
        event.UnpackTo(&rcpSlssUe);
        onSlssRxInfoChange(rcpSlssUe);
    }
}

telux::common::Status Cv2xEvtListener::registerListener(std::weak_ptr<ICv2xListener> listener) {
    return listenerMgr_.registerListener(listener);
}

telux::common::Status Cv2xEvtListener::deregisterListener(std::weak_ptr<ICv2xListener> listener) {
    return listenerMgr_.deRegisterListener(listener);
}

uint32_t Cv2xEvtListener::listenersSize() {
    std::vector<std::weak_ptr<ICv2xListener>> applisteners;
    listenerMgr_.getAvailableListeners(applisteners);
    return applisteners.size();
}

Cv2xRadioManagerStub::Cv2xRadioManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    Cv2xRadioManagerStub(false);
}

Cv2xRadioManagerStub::Cv2xRadioManagerStub(bool runAsIPCServer) {
    LOG(DEBUG, __FUNCTION__);
    exiting_      = false;
    taskQ_        = std::make_shared<AsyncTaskQueue<void>>();
    stub_         = CommonUtils::getGrpcStub<::cv2xStub::Cv2xManagerService>();
    pEvtListener_ = std::make_shared<Cv2xEvtListener>();
}

Cv2xRadioManagerStub::~Cv2xRadioManagerStub() {
    exiting_ = true;
    initializedCv_.notify_all();

    if (pEvtListener_) {
        std::vector<std::string> filters = {CV2X_EVENT_RADIO_MGR_FILTER};
        auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
        clientEventManager.deregisterListener(pEvtListener_, filters);
    }
    {
        std::unique_lock<std::mutex> lock(radioMutex_);
        while (not cv2xRadioInitCallbacks_.empty()) {
            LOG(DEBUG, __FUNCTION__, " waiting cv2xRadioInitCallbacks complete");
            radioCv_.wait(lock);
        }
    }

    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status Cv2xRadioManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(INFO, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void Cv2xRadioManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    ::cv2xStub::GetServiceStatusReply response;
    int delay = DEFAULT_DELAY;

    if (pEvtListener_) {
        std::vector<std::string> filters = {CV2X_EVENT_RADIO_MGR_FILTER};
        auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(pEvtListener_, filters);
    }

    CALL_RPC(stub_->initService, request, status, response, delay);
    {
        lock_guard<mutex> cvLock(mutex_);
        serviceStatus_ = static_cast<telux::common::ServiceStatus>(response.status());
        initializedCv_.notify_all();
    }
    if (status == telux::common::Status::FAILED) {
        LOG(DEBUG, __FUNCTION__, "Fail to init Cv2xRadioManagerStub");
    }

    if (callback && (delay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        callback(serviceStatus_);
    }
}

bool Cv2xRadioManagerStub::isReady() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return telux::common::ServiceStatus::SERVICE_AVAILABLE == serviceStatus_;
}

std::future<bool> Cv2xRadioManagerStub::onReady() {
    auto f = std::async(std::launch::async, [this] {
        unique_lock<mutex> cvLock(mutex_);
        while (telux::common::ServiceStatus::SERVICE_UNAVAILABLE == serviceStatus_ && !exiting_) {
            initializedCv_.wait(cvLock);
        }
        return telux::common::ServiceStatus::SERVICE_AVAILABLE == serviceStatus_;
    });
    return f;
}

telux::common::ServiceStatus Cv2xRadioManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

telux::common::Status Cv2xRadioManagerStub::startCv2x(StartCv2xCallback cb) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    CALL_RPC_AND_RESPOND(stub_->startCv2x, request, status, cb, taskQ_);
    return status;
}

telux::common::Status Cv2xRadioManagerStub::stopCv2x(StopCv2xCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    CALL_RPC_AND_RESPOND(stub_->stopCv2x, request, status, cb, taskQ_);
    return status;
}

telux::common::Status Cv2xRadioManagerStub::requestCv2xStatus(RequestCv2xStatusCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    ::cv2xStub::Cv2xRequestStatusReply response;
    int delay = DEFAULT_DELAY;
    CALL_RPC(stub_->requestCv2xStatus, request, status, response, delay);
    if (cb) {
        auto f = std::async(std::launch::async, [=]() {
            if (delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            telux::cv2x::Cv2xStatus cv2xStatus;
            if (status == telux::common::Status::SUCCESS) {
                RPC_TO_CV2X_STATUS(response.cv2xstatus(), cv2xStatus);
            }
            cb(cv2xStatus, static_cast<telux::common::ErrorCode>(response.error()));
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status Cv2xRadioManagerStub::requestCv2xStatus(RequestCv2xStatusCallbackEx cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    ::cv2xStub::Cv2xRequestStatusReply response;
    int delay = DEFAULT_DELAY;
    CALL_RPC(stub_->requestCv2xStatus, request, status, response, delay);
    if (cb) {
        auto f = std::async(std::launch::async, [=]() {
            if (delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            telux::cv2x::Cv2xStatusEx cv2xStatusEx;
            if (status == telux::common::Status::SUCCESS) {
                RPC_TO_CV2X_STATUS(response.cv2xstatus(), cv2xStatusEx.status);
            }
            cb(cv2xStatusEx, static_cast<telux::common::ErrorCode>(response.error()));
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status Cv2xRadioManagerStub::registerListener(
    std::weak_ptr<ICv2xListener> listener) {
    auto status = telux::common::Status::FAILED;
    if (pEvtListener_) {
        status = pEvtListener_->registerListener(listener);
    }
    return status;
}

telux::common::Status Cv2xRadioManagerStub::deregisterListener(
    std::weak_ptr<ICv2xListener> listener) {
    auto status = telux::common::Status::FAILED;
    if (pEvtListener_) {
        status = pEvtListener_->deregisterListener(listener);
    }
    return status;
}

telux::common::Status Cv2xRadioManagerStub::setPeakTxPower(
    int8_t txPower, common::ResponseCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::Cv2xPeakTxPower request;

    request.set_txpower(static_cast<int32_t>(txPower));
    CALL_RPC_AND_RESPOND(stub_->setPeakTxPower, request, status, cb, taskQ_);
    return status;
}

telux::common::Status Cv2xRadioManagerStub::setL2Filters(
    const std::vector<L2FilterInfo> &filterList, common::ResponseCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::L2FilterInfos infos;
    for (auto filter : filterList) {
        ::cv2xStub::L2FilterInfo *info = infos.add_info();
        info->set_srcl2id(filter.srcL2Id);
        info->set_durationms(filter.durationMs);
        info->set_pppp(filter.pppp);
    }
    CALL_RPC_AND_RESPOND(stub_->setL2Filters, infos, status, cb, taskQ_);

    return status;
}

telux::common::Status Cv2xRadioManagerStub::removeL2Filters(
    const std::vector<uint32_t> &l2IdList, common::ResponseCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::L2Ids ids;
    for (auto i : l2IdList) {
        ids.add_id(i);
    }
    CALL_RPC_AND_RESPOND(stub_->removeL2Filters, ids, status, cb, taskQ_);
    return status;
}

telux::common::Status Cv2xRadioManagerStub::getSlssRxInfo(GetSlssRxInfoCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    ::cv2xStub::SlssRxInfoReply response;
    int delay                    = DEFAULT_DELAY;
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;
    CALL_RPC(stub_->getSlssRxInfo, request, status, response, delay);
    if (cb) {
        auto f = std::async(std::launch::async, [=]() {
            SlssRxInfo info;

            if (delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            if (status != telux::common::Status::SUCCESS) {
                LOG(ERROR, __FUNCTION__, "Fail to get slss Rx info");
            } else {
                SyncRefUeInfo refInfo;

                for (auto ueInfo : response.info()) {
                    cv2x::Cv2xRadioHelper::rpcSlssInfoToSlssInfo(ueInfo, refInfo);
                    info.ueInfo.push_back(refInfo);
                }
            }
            cb(info, static_cast<telux::common::ErrorCode>(response.error()));
        }).share();
        taskQ_->add(f);
    }

    return status;
}

telux::common::Status Cv2xRadioManagerStub::injectCoarseUtcTime(
    uint64_t utc, common::ResponseCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::Cv2xCommandReply response;
    ::cv2xStub::CoarseUtcTime t;
    t.set_utc(utc);

    CALL_RPC_AND_RESPOND(stub_->injectCoarseUtcTime, t, status, cb, taskQ_);

    return status;
}

void Cv2xRadioManagerStub::onGetCv2xRadioResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::unique_lock<std::mutex> lock(radioMutex_);
        cv2xRadioInitStatus_ = status;
        if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(ERROR, "Fail to initialize Cv2xRadio");
            radio_.reset();  // FAILED, radio_ reset to nullptr
        }
        cbs = cv2xRadioInitCallbacks_;
        cv2xRadioInitCallbacks_.clear();
        radioCv_.notify_all();
    }
    for (auto &cb : cbs) {
        if (cb) {
            cb(status);
        }
    }
}

std::shared_ptr<ICv2xRadio> Cv2xRadioManagerStub::getCv2xRadio(
    TrafficCategory category, telux::common::InitResponseCb cb) {
    LOG(DEBUG, __FUNCTION__);
    // Traffic Category is currently unused.

    std::shared_ptr<Cv2xRadioSimulation> radio = nullptr;

    {
        lock_guard<mutex> lock(radioMutex_);
        radio = std::dynamic_pointer_cast<Cv2xRadioSimulation>(radio_.lock());
        if (radio && telux::common::ServiceStatus::SERVICE_FAILED != radio->getServiceStatus()) {
            // Radio has completed initialization and success, or has not complete
            // initialization, but not FAILED.
            if (cv2xRadioInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
                // SERVICE_UNAVAILABLE is initial status
                LOG(DEBUG, __FUNCTION__, " Cv2xRadio status is SERVICE_UNAVAILABLE");
                // radio initialization is triggered, wait for init result
                cv2xRadioInitCallbacks_.push_back(cb);
            } else {
                if (cb) {
                    LOG(DEBUG, __FUNCTION__, " Cv2xRadio status is SERVICE_AVAILABLE");
                    auto f = std::async(std::launch::async, [cb]() {
                        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                    }).share();
                    // Add to Async queue to unblock calling thread
                    taskQ_->add(f);
                }
            }
            LOG(DEBUG, __FUNCTION__, " returns existing radio");
            return radio;
        }

        /*no radio, or existing radio in bad state , make a new instance*/
        LOG(DEBUG, __FUNCTION__, " Creating new cv2x radio");
        try {
            radio = make_shared<Cv2xRadioSimulation>();
        } catch (std::bad_alloc &e) {
            LOG(ERROR, "Fail to get Cv2xRadio due to ", e.what());
            return nullptr;
        }

        cv2xRadioInitCallbacks_.push_back(cb);
        radio_ = radio;
    }
    // not necessary nullptr check, but still a good practice
    if (radio) {
        radio->init(
            [this](telux::common::ServiceStatus status) { onGetCv2xRadioResponse(status); });
    }

    return radio;
}

}  // namespace cv2x
}  // namespace telux
