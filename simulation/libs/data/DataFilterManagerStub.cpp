/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataFilterManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DATA_FILTER "data_filter"
#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {

DataFilterManagerStub::DataFilterManagerStub(SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ =
        std::make_shared<telux::common::ListenerManager<IDataFilterListener>>();
    slotId_ = slotId;
}

DataFilterManagerStub::~DataFilterManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status DataFilterManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

bool DataFilterManagerStub::isReady() {
    std::lock_guard<std::mutex> lock(mutex_);
    return (subSystemStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        ? true : false;
}

std::future<bool> DataFilterManagerStub::onReady() {
    std::future<bool> f;
    f = std::async(std::launch::async, [&] {
        return waitForInitialization(); });
    return f;
}

bool DataFilterManagerStub::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    {
        std::unique_lock<std::mutex> cvLock(mutex_);
        if (subSystemStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            initCV_.wait(cvLock);
        }
    }
    return isReady();
}

void DataFilterManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::DataFilterManager>();

    ::dataStub::SlotInfo request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
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

    std::vector<std::string> filters = {DATA_FILTER};
    auto &clientEventManager = telux::common::ClientEventManager::getInstance();
    clientEventManager.registerListener(shared_from_this(), filters);
}

void DataFilterManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void DataFilterManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void DataFilterManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus DataFilterManagerStub::getServiceStatus() {
    return subSystemStatus_;
}

telux::common::Status DataFilterManagerStub::setDataRestrictMode(DataRestrictMode mode,
    telux::common::ResponseCallback callback) {
    LOG(INFO, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data filter manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetDataRestrictModeRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    request.mutable_filter_mode()->set_filter_mode(
        static_cast<::dataStub::DataRestrictMode::DataRestrictModeType>(mode.filterMode));
    request.mutable_filter_mode()->set_filter_auto_exit(
        static_cast<::dataStub::DataRestrictMode::DataRestrictModeType>(mode.filterAutoExit));
    grpc::Status reqStatus = stub_->SetDataRestrictMode(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setDataRestrictMode request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status DataFilterManagerStub::requestDataRestrictMode(
    DataRestrictModeCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data filter manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::GetDataRestrictModeRequest request;
    ::dataStub::GetDataRestrictModeReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->GetDataRestrictMode(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    DataRestrictMode mode;
    mode.filterMode =
        static_cast<telux::data::DataRestrictModeType>(
            response.filter_mode().filter_mode());
    mode.filterAutoExit =
        static_cast<telux::data::DataRestrictModeType>(
            response.filter_mode().filter_auto_exit());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " requestDataRestrictMode failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, error, mode, callback, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(mode, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataFilterManagerStub::addDataRestrictFilter(
    std::shared_ptr<IIpFilter> &filter,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data filter manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    if (filter == nullptr) {
        return telux::common::Status::INVALIDPARAM;
    }

    ::dataStub::AddDataRestrictFilterRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->AddDataRestrictFilter(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " addDataRestrictFilter request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status DataFilterManagerStub::removeAllDataRestrictFilters(
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data filter manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RemoveDataRestrictFilterRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->RemoveAllDataRestrictFilter(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " RemoveAllDataRestrictFilter request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

SlotId DataFilterManagerStub::getSlotId() {
    LOG(DEBUG, __FUNCTION__);
    return slotId_;
}

telux::common::Status DataFilterManagerStub::registerListener(
    std::weak_ptr<IDataFilterListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status DataFilterManagerStub::deregisterListener(
    std::weak_ptr<IDataFilterListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void DataFilterManagerStub::onDataRestrictModeChange(
    DataRestrictMode mode) {
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDataFilterListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG,
                    "DataFilter Manager: invoking onDataRestrictModeChange");
                sp->onDataRestrictModeChange(mode);
            }
        }
    }
}

void DataFilterManagerStub::onServiceStatusChange(
    telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDataFilterListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "DataFilter Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status DataFilterManagerStub::setDataRestrictMode(DataRestrictMode mode,
    telux::common::ResponseCallback callback, int profileId,
    IpFamilyType ipFamilyType) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataFilterManagerStub::requestDataRestrictMode(
    std::string ifaceName, DataRestrictModeCb callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataFilterManagerStub::addDataRestrictFilter(
    std::shared_ptr<IIpFilter> &filter,
    telux::common::ResponseCallback callback, int profileId,
    IpFamilyType ipFamilyType) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataFilterManagerStub::removeAllDataRestrictFilters(
    telux::common::ResponseCallback callback, int profileId,
    IpFamilyType ipFamilyType) {
    return telux::common::Status::NOTSUPPORTED;
}

void DataFilterManagerStub::onEventUpdate(google::protobuf::Any event) {
    if (event.Is<::dataStub::SetDataRestrictModeRequest>()) {
        ::dataStub::SetDataRestrictModeRequest modeUpdateEvent;
        event.UnpackTo(&modeUpdateEvent);
        if (modeUpdateEvent.slot_id() == slotId_) {
            DataRestrictMode mode;
            mode.filterMode = static_cast<telux::data::DataRestrictModeType>(
                modeUpdateEvent.filter_mode().filter_mode());
            mode.filterAutoExit = static_cast<telux::data::DataRestrictModeType>(
                modeUpdateEvent.filter_mode().filter_auto_exit());
            this->onDataRestrictModeChange(mode);
        }
    }
}

} // end of namespace data
} // end of namespace telux