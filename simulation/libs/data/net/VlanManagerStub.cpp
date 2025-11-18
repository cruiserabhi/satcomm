/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "VlanManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "libs/data/DataUtilsStub.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {
namespace net {

VlanManagerStub::VlanManagerStub (telux::data::OperationType oprType)
: oprType_(oprType) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IVlanListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

VlanManagerStub::~VlanManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status VlanManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void VlanManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::VlanManager>();

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

void VlanManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void VlanManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void VlanManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

std::future<bool> VlanManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return VlanManagerStub::waitForInitialization(); });
    return future;
}

bool VlanManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

void VlanManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus VlanManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

bool VlanManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

telux::data::OperationType VlanManagerStub::getOperationType() {
    LOG(DEBUG, __FUNCTION__);
    return oprType_;
}

telux::common::Status VlanManagerStub::registerListener(
    std::weak_ptr<IVlanListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status VlanManagerStub::deregisterListener(
    std::weak_ptr<IVlanListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void VlanManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IVlanListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Vlan Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status VlanManagerStub::createVlan(
    const VlanConfig &vlanConfig, CreateVlanCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " vlan manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::CreateVlanRequest request;
    ::dataStub::CreateVlanReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    request.set_vlan_id(vlanConfig.vlanId);
    request.set_is_accelerated(vlanConfig.isAccelerated);
    request.set_priority(vlanConfig.priority);
    request.set_interface_type(::dataStub::InterfaceType(vlanConfig.iface));
    request.set_create_bridge(vlanConfig.createBridge);
    request.mutable_nw_type()->set_nw_type(DataUtilsStub::convertNetworkTypeToGrpc(
                vlanConfig.nwType));
    grpc::Status reqStatus = stub_->CreateVlan(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;
    bool isAccelerated = false;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());
    isAccelerated = response.is_accelerated();

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " createVlan request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay, isAccelerated]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(isAccelerated, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status VlanManagerStub::removeVlan(int16_t vlanId,
    InterfaceType ifaceType, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " vlan manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::RemoveVlanRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    request.set_vlan_id(vlanId);
    request.set_interface_type(::dataStub::InterfaceType(ifaceType));
    grpc::Status reqStatus = stub_->RemoveVlan(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " removeVlan request failed");
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

telux::common::Status VlanManagerStub::queryVlanInfo(
    QueryVlanResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " vlan manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::QueryVlanInfoRequest request;
    ::dataStub::QueryVlanInfoReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->QueryVlanInfo(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " queryVlanInfo request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::vector<VlanConfig> configs;

            for (auto config : response.vlan_config()) {
                VlanConfig vlanConfig;
                vlanConfig.iface =
                    (telux::data::InterfaceType)config.interface_type();
                vlanConfig.vlanId = config.vlan_id();
                vlanConfig.isAccelerated = config.is_accelerated();
                vlanConfig.priority = config.priority();
                vlanConfig.createBridge = config.create_bridge();
                vlanConfig.nwType = DataUtilsStub::convertNetworkTypeToEnum(config.nw_type());
                configs.push_back(vlanConfig);
            }
            auto f1 = std::async(std::launch::async,
                [this, error, callback, configs, delay]() {
                    callback(configs, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status VlanManagerStub::bindToBackhaul(
    VlanBindConfig vlanBindConfig, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " vlan manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::BindToBackhaulConfig request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_vlan_id(vlanBindConfig.vlanId);
    request.set_slot_id(vlanBindConfig.bhInfo.slotId);
    request.set_profile_id(vlanBindConfig.bhInfo.profileId);
    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        vlanBindConfig.bhInfo.backhaul));
    request.set_operation_type(::dataStub::OperationType(oprType_));
    request.set_backhaul_vlan_id(vlanBindConfig.bhInfo.vlanId);
    grpc::Status reqStatus = stub_->BindToBackhaul(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " bindToBackhaul request failed");
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

telux::common::Status VlanManagerStub::unbindFromBackhaul(
    VlanBindConfig vlanBindConfig, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " vlan manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::BindToBackhaulConfig request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_vlan_id(vlanBindConfig.vlanId);
    request.set_slot_id(vlanBindConfig.bhInfo.slotId);
    request.set_profile_id(vlanBindConfig.bhInfo.profileId);
    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(
        vlanBindConfig.bhInfo.backhaul));
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->UnbindFromBackhaul(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " unbindFromBackhaul request failed");
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

telux::common::Status VlanManagerStub::queryVlanToBackhaulBindings(
    BackhaulType backhaulType,
    VlanBindingsResponseCb callback, SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " vlan manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::dataStub::QueryVlanMappingListRequest request;
    ::dataStub::QueryVlanMappingListReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    request.set_slot_id(slotId);
    request.set_backhaul_type((::dataStub::BackhaulPreference)backhaulType);
    grpc::Status reqStatus = stub_->QueryVlanMappingList(&context, request, &response);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " queryVlanToBackhaulBindings request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::vector<VlanBindConfig> configs;

            for (auto config : response.vlan_mapping()) {
                VlanBindConfig vlanConfig;
                vlanConfig.bhInfo.backhaul = backhaulType;
                vlanConfig.vlanId = config.vlan_id();
                vlanConfig.bhInfo.slotId = slotId;
                vlanConfig.bhInfo.profileId = config.profile_id();
                vlanConfig.bhInfo.vlanId = config.backhaul_vlan_id();
                configs.push_back(vlanConfig);
            }
            auto f1 = std::async(std::launch::async,
                [this, error, callback, configs, delay]() {
                    callback(configs, error);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status VlanManagerStub::bindWithProfile(int profileId, int vlanId,
    telux::common::ResponseCallback callback , SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status VlanManagerStub::unbindFromProfile(int profileId, int vlanId,
    telux::common::ResponseCallback callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status VlanManagerStub::queryVlanMappingList(
    VlanMappingResponseCb callback, SlotId slotId) {
    return telux::common::Status::NOTSUPPORTED;
}

} // end of namespace net
} // end of namespace data
} // end of namespace telux
