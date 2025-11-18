/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataLinkManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DELAY 100
#define DEFAULT_DELIMITER " "
#define DATA_LINK_SSR_FILTER "data_link_ssr"
#define ETH_DATA_LINK_STATE_CHANGE_FILTER "eth_data_link_state_change"

namespace telux {
namespace data {

using telux::common::Status;

DataLinkManagerStub::DataLinkManagerStub()
    : SimulationManagerStub<DataLinkManager>(std::string("IDataLinkManagerStub"))
    , clientEventMgr_(ClientEventManager::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

DataLinkManagerStub::~DataLinkManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void DataLinkManagerStub::createListener() {
    LOG(DEBUG, __FUNCTION__);

    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IDataLinkListener>>();
}

void DataLinkManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void DataLinkManagerStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t DataLinkManagerStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
    return cbDelay_;
}

Status DataLinkManagerStub::init() {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::SUCCESS;

    try {
        createListener();
    } catch (std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ": Invalid listener instance");
        return Status::FAILED;
    }
    status = registerDefaultIndications();
    return status;
}

Status DataLinkManagerStub::registerDefaultIndications() {
    Status status = Status::FAILED;
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    status = clientEventMgr_.registerListener(shared_from_this(), DATA_LINK_SSR_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }

    status = clientEventMgr_.registerListener(shared_from_this(),
            ETH_DATA_LINK_STATE_CHANGE_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering eth datalink state change indications failed");
        return status;
    }
    return Status::SUCCESS;
}

void DataLinkManagerStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        // Deregister optional indications on SSR (if any)
    }
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDataLinkListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Data link Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(srvcStatus);
            }
        }
    }
}

ServiceStatus DataLinkManagerStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status DataLinkManagerStub::initSyncComplete(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::FAILED;
    status = registerDefaultIndications();

    return status;
}

void DataLinkManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    // Execute all events in separate thread
    auto f = std::async(std::launch::deferred, [this, event]() {
            if (event.Is<commonStub::GetServiceStatusReply>()) {
                handleSSREvent(event);
            } else if (event.Is<dataStub::OnEthDataLinkStateChangeReply>()) {
                handleEthDatalinkChangeEvent(event);
            } else {
                LOG(ERROR, __FUNCTION__, ":: Invalid event");
    }}).share();

    taskQ_.add(f);
}

void DataLinkManagerStub::handleEthDatalinkChangeEvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    dataStub::OnEthDataLinkStateChangeReply indication;
    event.UnpackTo(&indication);

    telux::data::LinkState linkState;

    auto ethLinkState = indication.eth_datalink_state().link_state();

    if (ethLinkState == ::dataStub::LinkStateEnum_LinkState_UP) {
        linkState = telux::data::LinkState::UP;
    } else if (ethLinkState == ::dataStub::LinkStateEnum_LinkState_DOWN) {
        linkState = telux::data::LinkState::DOWN;
    } else {
        // Ignore
        LOG(ERROR, __FUNCTION__, ":: INVALID eth link state event");
        return;
    }

    std::vector<std::weak_ptr<IDataLinkListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    LOG(DEBUG, __FUNCTION__, ":: Notifying eth data link state change event ",
            " to listeners: ", applisteners.size());

    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onEthDataLinkStateChange(linkState);
        }
    }

}

void DataLinkManagerStub::handleSSREvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    commonStub::GetServiceStatusReply ssrResp;
    event.UnpackTo(&ssrResp);

    ServiceStatus srvcStatus = ServiceStatus::SERVICE_FAILED;

    if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_AVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_AVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_UNAVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_FAILED) {
        srvcStatus = ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(ERROR, __FUNCTION__, ":: INVALID SSR event");
        return;
    }
    setServiceReady(srvcStatus);
    onServiceStatusChange(srvcStatus);
}

void DataLinkManagerStub::onServiceStatusChange(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__, ":: Service Status: ", static_cast<int>(srvcStatus));

    if (srvcStatus == getServiceStatus()) {
        return;
    }
    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: Datalink Service is UNAVAILABLE");
        setServiceStatus(srvcStatus);
    } else {
        LOG(INFO, __FUNCTION__, ":: Datalink Service is AVAILABLE");
        auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
        taskQ_.add(f);
    }
}

telux::common::Status DataLinkManagerStub::registerListener(
    std::weak_ptr<IDataLinkListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status DataLinkManagerStub::deregisterListener(
    std::weak_ptr<IDataLinkListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

telux::common::ErrorCode DataLinkManagerStub::setEthDataLinkState(telux::data::LinkState linkState)
{
    LOG(DEBUG, __FUNCTION__);

    ::dataStub::SetEthDatalinkStateRequest request;
    ::dataStub::SetEthDatalinkStateReply response;
    ClientContext context;

    if (linkState == telux::data::LinkState::UP) {
        request.mutable_eth_datalink_state()->set_link_state(
                ::dataStub::LinkStateEnum_LinkState_UP);
    } else {
        request.mutable_eth_datalink_state()->set_link_state(
                ::dataStub::LinkStateEnum_LinkState_DOWN);
    }

    grpc::Status reqStatus = stub_->SetEthDataLinkState(&context, request, &response);

    telux::common::ErrorCode error  =
        static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, ", errorcode: ", static_cast<int>(reqStatus.error_code()));
            LOG(ERROR, __FUNCTION__, " seEthDataLinkState request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }

    return error;
}

telux::common::Status DataLinkManagerStub::getEthCapability(telux::data::EthCapability
        &ethCapability) 
{
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataLinkManagerStub::setPeerEthCapability(telux::data::EthCapability
        ethCapability) 
{
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataLinkManagerStub::setLocalEthOperatingMode(telux::data::EthModeType
        ethModeType, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataLinkManagerStub::setPeerModeChangeRequestStatus(LinkModeChangeStatus
        status) {
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::NOTSUPPORTED;
}

} // end of namespace data
} // end of namespace telux
