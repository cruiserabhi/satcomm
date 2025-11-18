/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "NatManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "DataUtilsStub.hpp"
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {
namespace net {

NatManagerStub::NatManagerStub (telux::data::OperationType oprType)
: oprType_(oprType) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<INatListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

NatManagerStub::~NatManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status NatManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void NatManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::SnatManager>();

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

void NatManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void NatManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void NatManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

std::future<bool> NatManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return NatManagerStub::waitForInitialization(); });
    return future;
}

bool NatManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

void NatManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus NatManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

bool NatManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

telux::common::Status NatManagerStub::addStaticNatEntry(int profileId,
    const NatConfig &snatConfig, telux::common::ResponseCallback callback,
    SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Nat manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::StaticNatRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;
    request.mutable_static_nat_entry()->set_operation_type(::dataStub::OperationType(oprType_));
    request.mutable_static_nat_entry()->set_backhaul_type(
            ::dataStub::BackhaulPreference::PREF_WWAN);
    request.mutable_static_nat_entry()->set_profile_id(profileId);
    request.mutable_static_nat_entry()->set_slot_id(slotId);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_address(snatConfig.addr);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_port(snatConfig.port);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_global_port(
        snatConfig.globalPort);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_ip_protocol(
        DataUtilsStub::protocolToString(snatConfig.proto));

    grpc::Status reqStatus = stub_->AddStaticNatEntry(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " addStaticNatEntry request failed");
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

telux::common::Status NatManagerStub::addStaticNatEntry(const BackhaulInfo &bhInfo, const NatConfig
        &snatConfig, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Nat manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::StaticNatRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.mutable_static_nat_entry()->set_operation_type(::dataStub::OperationType(oprType_));
    if (bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
        request.mutable_static_nat_entry()->set_backhaul_type(
                ::dataStub::BackhaulPreference::PREF_WWAN);
        request.mutable_static_nat_entry()->set_profile_id(bhInfo.profileId);
        request.mutable_static_nat_entry()->set_slot_id(bhInfo.slotId);
    } else if (bhInfo.backhaul == telux::data::BackhaulType::ETH) {
        request.mutable_static_nat_entry()->set_backhaul_type(
                ::dataStub::BackhaulPreference::PREF_ETH);
        request.mutable_static_nat_entry()->set_vlan_id(bhInfo.vlanId);
    } else {
        return telux::common::Status::NOTSUPPORTED;
    }

    request.mutable_static_nat_entry()->mutable_nat_config()->set_address(snatConfig.addr);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_port(snatConfig.port);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_global_port(
        snatConfig.globalPort);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_ip_protocol(
        DataUtilsStub::protocolToString(snatConfig.proto));

    grpc::Status reqStatus = stub_->AddStaticNatEntry(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " addStaticNatEntry request failed");
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

telux::common::Status NatManagerStub::removeStaticNatEntry(int profileId,
    const NatConfig &snatConfig, telux::common::ResponseCallback callback,
    SlotId slotId) {
     LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Nat manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::StaticNatRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.mutable_static_nat_entry()->set_operation_type(::dataStub::OperationType(oprType_));
    request.mutable_static_nat_entry()->set_backhaul_type(
            ::dataStub::BackhaulPreference::PREF_WWAN);
    request.mutable_static_nat_entry()->set_profile_id(profileId);
    request.mutable_static_nat_entry()->set_slot_id(slotId);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_address(snatConfig.addr);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_port(snatConfig.port);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_global_port(
        snatConfig.globalPort);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_ip_protocol(
        DataUtilsStub::protocolToString(snatConfig.proto));

    grpc::Status reqStatus = stub_->RemoveStaticNatEntry(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " removeStaticNatEntry request failed");
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

telux::common::Status NatManagerStub::removeStaticNatEntry(const BackhaulInfo &bhInfo, const
        NatConfig &snatConfig, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Nat manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::StaticNatRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.mutable_static_nat_entry()->set_operation_type(::dataStub::OperationType(oprType_));
    if (bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
        request.mutable_static_nat_entry()->set_backhaul_type(
                ::dataStub::BackhaulPreference::PREF_WWAN);
        request.mutable_static_nat_entry()->set_profile_id(bhInfo.profileId);
        request.mutable_static_nat_entry()->set_slot_id(bhInfo.slotId);
    } else if (bhInfo.backhaul == telux::data::BackhaulType::ETH) {
        request.mutable_static_nat_entry()->set_backhaul_type(
                ::dataStub::BackhaulPreference::PREF_ETH);
        request.mutable_static_nat_entry()->set_vlan_id(bhInfo.vlanId);
    } else {
        return telux::common::Status::NOTSUPPORTED;
    }

    request.mutable_static_nat_entry()->mutable_nat_config()->set_address(snatConfig.addr);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_port(snatConfig.port);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_global_port(
        snatConfig.globalPort);
    request.mutable_static_nat_entry()->mutable_nat_config()->set_ip_protocol(
        DataUtilsStub::protocolToString(snatConfig.proto));

    grpc::Status reqStatus = stub_->RemoveStaticNatEntry(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " removeStaticNatEntry request failed");
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

telux::common::Status NatManagerStub::requestStaticNatEntries(int profileId,
    StaticNatEntriesCb snatEntriesCb, SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Nat manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::RequestStaticNatEntriesRequest request;
    ::dataStub::RequestStaticNatEntriesReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    request.set_backhaul_type(::dataStub::BackhaulPreference::PREF_WWAN);
    request.set_profile_id(profileId);
    request.set_slot_id(slotId);

    grpc::Status reqStatus = stub_->RequestStaticNatEntries(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " requestStaticNatEntries request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        std::vector<NatConfig> snatEntries;
        for (int idx = 0; idx < response.nat_config_size(); idx++) {
            NatConfig obj;
            obj.addr = response.mutable_nat_config(idx)->address();
            obj.port = response.mutable_nat_config(idx)->port();
            obj.globalPort = response.mutable_nat_config(idx)->global_port();
            obj.proto = DataUtilsStub::stringToProtocol(
                response.mutable_nat_config(idx)->ip_protocol());
            snatEntries.push_back(obj);
        }

        if (snatEntriesCb && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, snatEntries, snatEntriesCb, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    snatEntriesCb(snatEntries, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status NatManagerStub::requestStaticNatEntries(const BackhaulInfo &bhInfo,
        StaticNatEntriesCb snatEntriesCb) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Nat manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::RequestStaticNatEntriesRequest request;
    ::dataStub::RequestStaticNatEntriesReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    if (bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
        request.set_backhaul_type(::dataStub::BackhaulPreference::PREF_WWAN);
        request.set_profile_id(bhInfo.profileId);
        request.set_slot_id(bhInfo.slotId);
    } else if (bhInfo.backhaul == telux::data::BackhaulType::ETH) {
        request.set_backhaul_type(::dataStub::BackhaulPreference::PREF_ETH);
        request.set_vlan_id(bhInfo.vlanId);
    } else {
        return telux::common::Status::NOTSUPPORTED;
    }

    grpc::Status reqStatus = stub_->RequestStaticNatEntries(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " removeStaticNatEntry request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        std::vector<NatConfig> snatEntries;
        for (int idx = 0; idx < response.nat_config_size(); idx++) {
            NatConfig obj;
            obj.addr = response.mutable_nat_config(idx)->address();
            obj.port = response.mutable_nat_config(idx)->port();
            obj.globalPort = response.mutable_nat_config(idx)->global_port();
            obj.proto = DataUtilsStub::stringToProtocol(
                response.mutable_nat_config(idx)->ip_protocol());
            snatEntries.push_back(obj);
        }

        if (snatEntriesCb && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, snatEntries, snatEntriesCb, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    snatEntriesCb(snatEntries, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::data::OperationType NatManagerStub::getOperationType() {
    LOG(DEBUG, __FUNCTION__);
    return oprType_;
}

telux::common::Status NatManagerStub::registerListener(
    std::weak_ptr<INatListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status NatManagerStub::deregisterListener(
    std::weak_ptr<INatListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void NatManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INatListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Nat Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

} // end of namespace net
} // end of namespace data
} // end of namespace telux
