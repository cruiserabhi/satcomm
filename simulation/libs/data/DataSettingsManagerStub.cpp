/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataSettingsManagerStub.hpp"
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

DataSettingsManagerStub::DataSettingsManagerStub(
    OperationType oprType)
    : oprType_(oprType){
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

DataSettingsManagerStub::~DataSettingsManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status DataSettingsManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(INFO, __FUNCTION__);
    initCb_ = callback;
    auto f = std::async(std::launch::async,
            [this, callback]() {
                this->initSync(callback);
            }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void DataSettingsManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::DataSettingsManager>();

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

    setSubSystemStatus(cbStatus);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

telux::common::ServiceStatus DataSettingsManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

void DataSettingsManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

void DataSettingsManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void DataSettingsManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

telux::common::Status DataSettingsManagerStub::requestDdsSwitch(
    DdsInfo info, telux::common::ResponseCallback callback) {
    LOG(INFO, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetDdsSwitchRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(info.slotId);
    request.set_switch_type(static_cast<int>(info.type));
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->SetDdsSwitch(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " DdsSwitch request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }

        if (error == telux::common::ErrorCode::SUCCESS) {
            this->onDdsChange(info);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::requestCurrentDds(
    RequestCurrentDdsResponseCb callback) {
    LOG(INFO, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::CurrentDdsSwitchRequest request;
    ::dataStub::CurrentDdsSwitchResponse response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->RequestCurrentDdsSwitch(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    DdsInfo ddsResponse;
    ddsResponse.slotId = static_cast<SlotId>(response.slot_id());
    ddsResponse.type = static_cast<DdsType>(response.current_switch());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " Request DDS failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, ddsResponse, callback]() {
                   callback(ddsResponse, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::restoreFactorySettings(
    OperationType operationType, telux::common::ResponseCallback callback,
    bool isRebootNeeded) {
    LOG(INFO, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataSettingsManagerStub::setBackhaulPreference(
    std::vector<BackhaulType> backhaulPref, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::setBackhaulPreferenceRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    for(size_t i=0; i<backhaulPref.size(); ++i) {
        request.add_backhaul_pref(
            static_cast<::dataStub::BackhaulPreference>(
            backhaulPref[i]));
    }
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->setBackhaulPreference(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setBackhaulPreference request failed");
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

telux::common::Status DataSettingsManagerStub::requestBackhaulPreference(
    RequestBackhaulPrefResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RequestBackhaulPreference request;
    ::dataStub::BackhaulPreferenceReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->requestBackhaulPreference(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    std::vector<BackhaulType> backhaulPref;
    for (auto& pref: response.backhaul_pref()) {
        backhaulPref.emplace_back(static_cast<BackhaulType>(pref));
    }

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request BackhaulPreference failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, backhaulPref, callback]() {
                   callback(backhaulPref, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::setBandInterferenceConfig(bool enable,
    std::shared_ptr<BandInterferenceConfig> config, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::BandInterferenceConfig request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_enable(enable);
    if(enable) {
        request.set_priority(static_cast<int>(config->priority));
        request.set_wlan_wait_time_in_sec(config->wlanWaitTimeInSec);
        request.set_n79_wait_time_in_sec(config->n79WaitTimeInSec);
        request.set_operation_type(::dataStub::OperationType(oprType_));
    }

    grpc::Status reqStatus = stub_->setBandInterferenceConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setBandInterferenceConfig request failed");
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

telux::common::Status DataSettingsManagerStub::requestBandInterferenceConfig(
    RequestBandInterferenceConfigResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::BandInterferenceRequest request;
    ::dataStub::BandInterferenceReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus =
        stub_->requestBandInterferenceConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());


    bool enabled = response.config().enable();
    std::shared_ptr<BandInterferenceConfig> config = nullptr;
    if (enabled) {
        config = std::make_shared<BandInterferenceConfig>();
        config->priority = static_cast<BandPriority>(response.config().priority());
        config->n79WaitTimeInSec = response.config().wlan_wait_time_in_sec();
        config->wlanWaitTimeInSec = response.config().n79_wait_time_in_sec();
    }

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request BandInterferenceConfig failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, enabled, config, callback]() {
                   callback(enabled, config, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::setWwanConnectivityConfig(SlotId slotId,
    bool allow, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetWwanConnectivityConfigRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(slotId);
    request.set_is_wwan_connectivity_allowed(allow);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->SetWwanConnectivityConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setWwanConnectivityConfig failed");
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

    if (error == telux::common::ErrorCode::SUCCESS) {
        this->onWwanConnectivityConfigChange(slotId, allow);
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::requestWwanConnectivityConfig(SlotId slotId,
    requestWwanConnectivityConfigResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::WwanConnectivityConfigRequest request;
    ::dataStub::WwanConnectivityConfigReply response;
    ClientContext context;

    request.set_slot_id(slotId);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->RequestWwanConnectivityConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    bool isallowed = response.is_wwan_connectivity_allowed();

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " Request WwanConnectivity failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, slotId, isallowed, callback]() {
                   callback(slotId, isallowed, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::switchBackHaul(BackhaulInfo source,
    BackhaulInfo dest, bool applyToAll, telux::common::ResponseCallback callback) {
    LOG(DEBUG,__FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::switchBackHaulRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(dest.backhaul));
    request.set_slot_id(dest.slotId);
    request.set_profile_id(dest.profileId);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->switchBackHaul(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " switchBackHaul request failed");
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

telux::common::ErrorCode DataSettingsManagerStub::getIpPassThroughConfig(const IpptParams
        &ipptParms, IpptConfig &config) {
    LOG(DEBUG,__FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::dataStub::getIpptConfigRequest request;
    ::dataStub::getIpptConfigReply response;
    ClientContext context;
    request.set_profile_id(ipptParms.profileId);
    request.set_vlan_id(ipptParms.vlanId);
    request.set_slot_id(ipptParms.slotId);

    grpc::Status reqStatus = stub_->getIpPassThroughConfig(&context, request, &response);
    auto error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " getIpPassThrough request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        config.ipptOpr = DataUtilsStub::convertIpptOprToStruct(response.ippt_opr());
        config.devConfig.nwInterface =
            DataUtilsStub::convertInterfaceTypeToStruct(response.interface_type());
        config.devConfig.macAddr = response.mac_address();
    }

    return error;
}

telux::common::ErrorCode DataSettingsManagerStub::setIpPassThroughConfig(const IpptParams
        &ipptParms, const IpptConfig &config) {
    LOG(DEBUG,__FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::dataStub::setIpptConfigRequest request;
    ::dataStub::setIpptConfigReply response;
    ClientContext context;
    request.set_profile_id(ipptParms.profileId);
    request.set_vlan_id(ipptParms.vlanId);
    request.set_slot_id(ipptParms.slotId);
    request.set_interface_type(DataUtilsStub::convertInterfaceTypeToGrpc(
                config.devConfig.nwInterface));
    request.mutable_ippt_opr()->set_ippt_opr(DataUtilsStub::convertIpptOprToGrpc(config.ipptOpr));
    request.set_mac_address(config.devConfig.macAddr);

    grpc::Status reqStatus = stub_->setIpPassThroughConfig(&context, request, &response);
    auto error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setIpPassThrough request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }
    return error;
}

telux::common::ErrorCode DataSettingsManagerStub::getIpPassThroughNatConfig(bool &isNatEnabled) {
    LOG(DEBUG,__FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::google::protobuf::Empty request;
    ::dataStub::getIpptNatConfigReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->GetIpPassThroughNatConfig(&context, request, &response);
    auto error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " getIpPassThroughNatConfig request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        isNatEnabled = response.enable_nat();
    }
    return error;
}

telux::common::ErrorCode DataSettingsManagerStub::setIpPassThroughNatConfig(bool enableNat) {
    LOG(DEBUG,__FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::dataStub::setIpptNatConfigRequest request;
    ::dataStub::setIpptNatConfigReply response;
    ClientContext context;

    request.set_enable_nat(enableNat);

    grpc::Status reqStatus = stub_->SetIpPassThroughNatConfig(&context, request, &response);
    auto error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setIpPassThroughNatConfig request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }
    return error;
}

telux::common::ErrorCode DataSettingsManagerStub::getIpConfig(const IpConfigParams &ipConfigParams,
        IpConfig &ipConfig) {
    LOG(DEBUG,__FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::dataStub::getIpConfigRequest request;
    ::dataStub::getIpConfigReply response;
    ClientContext context;
    request.set_interface_type(DataUtilsStub::convertInterfaceTypeToGrpc(ipConfigParams.ifType));
    request.mutable_ip_family_type()->set_ip_family_type(
            DataUtilsStub::convertIpFamilyTypeToGrpc(ipConfigParams.ipFamilyType));
    request.set_vlan_id(ipConfigParams.vlanId);

    grpc::Status reqStatus = stub_->getIpConfig(&context, request, &response);
    auto error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " getIpConfig request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        ipConfig.ipType = DataUtilsStub::convertIpTypeToStruct(response.ip_type());
        ipConfig.ipOpr  = DataUtilsStub::convertIpAssignToStruct(response.ip_assign());
        DataUtilsStub::convertIpAddrInfoToStruct(response.ip_addr_info(), ipConfig.ipAddr);
    }

    return error;
}

telux::common::ErrorCode DataSettingsManagerStub::setIpConfig(const IpConfigParams &ipConfigParams,
        const IpConfig &ipConfig) {
    LOG(DEBUG,__FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::dataStub::setIpConfigRequest request;
    ::dataStub::setIpConfigReply response;
    ClientContext context;
    request.set_interface_type(DataUtilsStub::convertInterfaceTypeToGrpc(ipConfigParams.ifType));
    request.set_vlan_id(ipConfigParams.vlanId);
    request.mutable_ip_type()->set_ip_type(
            DataUtilsStub::convertIpTypeToGrpc(ipConfig.ipType));
    request.mutable_ip_assign()->set_ip_assign(
            DataUtilsStub::convertIpAssignToGrpc(ipConfig.ipOpr));
    request.mutable_ip_family_type()->set_ip_family_type(
            DataUtilsStub::convertIpFamilyTypeToGrpc(ipConfigParams.ipFamilyType));
    if ((ipConfig.ipType == telux::data::IpAssignType::STATIC_IP) && (ipConfig.ipOpr
                == telux::data::IpAssignOperation::DISABLE)) {
        request.mutable_ip_family_type()->set_ip_family_type(::dataStub::IpFamilyType_Type_IPV4);
    }
    auto *ipAddrInfo = request.mutable_ip_addr_info();
    DataUtilsStub::convertIpAddrInfoToGrpc(ipConfig.ipAddr, ipAddrInfo);

    grpc::Status reqStatus = stub_->setIpConfig(&context, request, &response);
    auto error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setIpConfig request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }
    return error;
}

bool DataSettingsManagerStub::isDeviceDataUsageMonitoringEnabled() {
    LOG(ERROR, __FUNCTION__, " TBD");
    return false;
}

telux::common::Status DataSettingsManagerStub::registerListener(
    std::weak_ptr<IDataSettingsListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> listenerLock(mutex_);
    telux::common::Status status = telux::common::Status::SUCCESS;
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, "Register Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::deregisterListener(
    std::weak_ptr<IDataSettingsListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                retVal=telux::common::Status::SUCCESS;
                break;
            }
        }
    }

    return (retVal);
}

telux::common::Status DataSettingsManagerStub::setMacSecState(
    bool enable, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetMacSecStateRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_enabled(enable);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->SetMacSecState(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setMacSecState request failed");
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

telux::common::Status DataSettingsManagerStub::requestMacSecState(
    RequestMacSecSateResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::MacSecStateRequest request;
    ::dataStub::MacSecStateReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->RequestMacSecState(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    bool isenabled = static_cast<SlotId>(response.enabled());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " Request MacSecState failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, isenabled, callback]() {
                   callback(isenabled, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

void DataSettingsManagerStub::getAvailableListeners(
    std::vector<std::shared_ptr<IDataSettingsListener>> &listeners) {
    LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners_.size());
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto sp = (*it).lock();
        if (sp) {
            listeners.emplace_back(sp);
            ++it;
        } else {
            LOG(DEBUG, "erased obsolete weak pointer from DataConnectionManagerImpl's listeners");
            it = listeners_.erase(it);
        }
    }
}

void DataSettingsManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);

    setSubSystemStatus(status);
    invokeInitCallback(status);

    std::vector<std::shared_ptr<IDataSettingsListener>> applisteners;
    this->getAvailableListeners(applisteners);
    for (auto& listener : applisteners) {
        listener->onServiceStatusChange(status);
    }
}

void DataSettingsManagerStub::onWwanConnectivityConfigChange(
    SlotId slotId, bool isConnectivityAllowed) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::shared_ptr<IDataSettingsListener>> listeners;
    this->getAvailableListeners(listeners);
    for(auto& listener : listeners) {
        listener->onWwanConnectivityConfigChange(slotId, isConnectivityAllowed);
    }
}

void DataSettingsManagerStub::onDdsChange(DdsInfo currentState) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::shared_ptr<IDataSettingsListener>> listeners;
    this->getAvailableListeners(listeners);
    for(auto& listener : listeners) {
        listener->onDdsChange(currentState);
    }
}

}  // namespace data
}  // namespace telux

