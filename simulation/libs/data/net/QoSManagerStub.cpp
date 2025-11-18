/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "QoSManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "data/TrafficFilterImpl.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {
namespace net {

QoSManagerStub::QoSManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IQoSListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

QoSManagerStub::~QoSManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status QoSManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void QoSManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::QoSManager>();

    ::dataStub::InitRequest request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(
        telux::data::OperationType::DATA_LOCAL));
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

void QoSManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void QoSManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus QoSManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status QoSManagerStub::registerListener(
    std::weak_ptr<IQoSListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status QoSManagerStub::deregisterListener(
    std::weak_ptr<IQoSListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void QoSManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IQoSListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "QoS Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::ErrorCode QoSManagerStub::addQoSFilter(
    QoSFilterConfig qosFilterConfig, QoSFilterHandle &filterHandle,
    QoSFilterErrorCode &QoSFilterErrorCode) {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode QoSManagerStub::getQosFilter(
    QoSFilterHandle filterHandle, std::shared_ptr<IQoSFilter> &qosFilter) {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode QoSManagerStub::getQosFilters(
    std::vector<std::shared_ptr<IQoSFilter>> &qosFilter) {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode QoSManagerStub::deleteQosFilter(
    uint32_t policyHandle) {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode QoSManagerStub::deleteAllQosConfigs() {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode QoSManagerStub::createTrafficClass(
    std::shared_ptr<ITcConfig> tcConfig,
    TcConfigErrorCode &tcConfigErrorCode) {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode QoSManagerStub::getAllTrafficClasses(
    std::vector<std::shared_ptr<ITcConfig>> &tcConfigs) {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode QoSManagerStub::deleteTrafficClass(
    std::shared_ptr<ITcConfig> tcConfig) {
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

uint32_t QoSFilterImpl::getHandle() {
    return handle_;
}
TrafficClass QoSFilterImpl::getTrafficClass() {
    return trafficClass_;
}
std::shared_ptr<ITrafficFilter> QoSFilterImpl::getTrafficFilter() {
    return trafficFilter_;
}
QoSFilterStatus QoSFilterImpl::getStatus() {
    return status_;
}
std::string QoSFilterImpl::toString() {
    std::stringstream outStr;
    outStr << " handle: " << static_cast<int>(handle_) << std::endl
           << " status: " << std::endl
           << "   ethStatus: " << filterInstallationStatusToString(status_.ethStatus) << std::endl
           << "   modemStatus: " << filterInstallationStatusToString(status_.modemStatus)
           << std::endl
           << "   ipaStatus: " << filterInstallationStatusToString(status_.ipaStatus) << std::endl
           << " traffic Class: " << +trafficClass_ << std::endl
           << " TrafficFilter: " << std::endl
           << trafficFilter_->toString() << std::endl;
    return outStr.str();
}

std::string QoSFilterImpl::filterInstallationStatusToString(
    FilterInstallationStatus filterInstallationStatus) {
    std::string status = "";
    switch (filterInstallationStatus) {
        case FilterInstallationStatus::SUCCESS:
            status = "SUCCESS";
            break;
        case FilterInstallationStatus::FAILED:
            status = "FAILED";
            break;
        case FilterInstallationStatus::PENDING:
            status = "PENDING";
            break;
        case FilterInstallationStatus::NOT_APPLICABLE:
            status = "NOT_APPLICABLE";
            break;
        default:
            LOG(ERROR, __FUNCTION__, " status is unexpected");
    }
    return status;
}

void QoSFilterImpl::setHandle(uint32_t handle) {
    handle_ = handle;
}
void QoSFilterImpl::setTrafficClass(TrafficClass trafficClass) {
    trafficClass_ = trafficClass;
}
void QoSFilterImpl::setTrafficFilter(std::shared_ptr<ITrafficFilter> trafficFilter) {
    trafficFilter_ = trafficFilter;
}
void QoSFilterImpl::setStatus(QoSFilterStatus status) {
    status_ = status;
}

TrafficClass TcConfigImpl::getTrafficClass() {
    return trafficClass_;
}
Direction TcConfigImpl::getDirection() {
    return direction_;
}
DataPath TcConfigImpl::getDataPath() {
    return dataPath_;
}
BandwidthConfig TcConfigImpl::getBandwidthConfig() {
    return bandwidthConfig_;
}
std::string TcConfigImpl::toString() {
    std::stringstream outStr;
    outStr  << " Traffic class: "   << +trafficClass_
            << ", Data path: "      << telux::data::TrafficFilterImpl::dataPathToString(dataPath_)
            << ", direction : "     << telux::data::TrafficFilterImpl::directionToString(direction_);
    if (validityMask_ & TcConfigValidField::TC_BANDWIDTH_CONFIG_VALID) {
        outStr << ", Min bandwidth config : "
               << +bandwidthConfig_.dlBandwidthValue.bandwidthRange.minBandwidth
               << ", Max bandwidth config : "
               << +bandwidthConfig_.dlBandwidthValue.bandwidthRange.maxBandwidth;
    }
    outStr << std::endl;
    return outStr.str();
}

TcConfigValidFields TcConfigImpl::getTcConfigValidFields() {
    return validityMask_;
}

void TcConfigImpl::setTrafficClass(TrafficClass trafficClass) {
    validityMask_ = validityMask_ | TcConfigValidField::TC_TRAFFIC_CLASS_VALID;
    trafficClass_ = trafficClass;
}
void TcConfigImpl::setDirection(Direction direction) {
    validityMask_ = validityMask_ | TcConfigValidField::TC_DIRECTION_VALID;
    direction_ = direction;
}
void TcConfigImpl::setDataPath(DataPath dataPath) {
    validityMask_ = validityMask_ | TcConfigValidField::TC_DATA_PATH_VALID;
    dataPath_ = dataPath;
}
void TcConfigImpl::setBandwidthConfig(BandwidthConfig bandwidthConfig) {
    validityMask_ = validityMask_ | TcConfigValidField::TC_BANDWIDTH_CONFIG_VALID;
    bandwidthConfig_ = bandwidthConfig;
}

TcConfigBuilder &TcConfigBuilder::setTrafficClass(TrafficClass trafficClass) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setTrafficClass(trafficClass);
    return *this;
}

TcConfigBuilder &TcConfigBuilder::setDirection(Direction direction) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setDirection(direction);
    return *this;
}

TcConfigBuilder &TcConfigBuilder::setBandwidthConfig(BandwidthConfig bandwidthConfig) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setBandwidthConfig(bandwidthConfig);
    return *this;
}

TcConfigBuilder &TcConfigBuilder::setDataPath(DataPath dataPath) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setDataPath(dataPath);
    return *this;
}

std::shared_ptr<ITcConfig> TcConfigBuilder::build() {
    return tcConfig_;
}

} // end of namespace net
} // end of namespace data
} // end of namespace telux
