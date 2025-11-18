/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "NetworkSelectionManagerStub.hpp"
#include "TelDefinesStub.hpp"

using namespace telux::common;
using namespace telux::tel;

NetworkSelectionManagerStub::NetworkSelectionManagerStub(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    phoneId_ = phoneId;
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

void NetworkSelectionManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(mtx_);
        subSystemStatus_ = status;
    }
    if(initCb_) {
        auto f1 = std::async(std::launch::async,
        [this, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay_));
                initCb_(status);
        }).share();
        taskQ_->add(f1);
    } else {
        LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

telux::common::Status NetworkSelectionManagerStub::init(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<INetworkSelectionListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<::telStub::NetworkSelectionService>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate network selection service");
        return telux::common::Status::FAILED;
    }
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if(!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    initCb_ = callback;
    auto f = std::async(std::launch::async,
        [this]() {
            this->initSync();
        }).share();
    auto status = taskQ_->add(f);
    return status;
}

void NetworkSelectionManagerStub::initSync() {
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " InitService request failed");
    } else {
        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
    }
    LOG(DEBUG, __FUNCTION__, " callback delay ", cbDelay_,
        " callback status ", static_cast<int>(cbStatus));
    bool isSubsystemReady = (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
        true : false;
    setSubsystemReady(isSubsystemReady);
    setServiceStatus(cbStatus);
}

NetworkSelectionManagerStub::~NetworkSelectionManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void NetworkSelectionManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus NetworkSelectionManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

void NetworkSelectionManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

bool NetworkSelectionManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

bool NetworkSelectionManagerStub::waitForInitialization() {
    std::unique_lock<std::mutex> cvLock(mtx_);
    while (!isSubsystemReady()) {
        cv_.wait(cvLock);
    }
    return isSubsystemReady();
}

std::future<bool> NetworkSelectionManagerStub::onSubsystemReady() {
    auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return f;
}

telux::common::Status NetworkSelectionManagerStub::registerListener(
    std::weak_ptr<INetworkSelectionListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_NETWORK_SELECTION_FILTER};
        std::vector<std::weak_ptr<INetworkSelectionListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 1) {
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(shared_from_this(), filters);
        } else {
            LOG(DEBUG, __FUNCTION__, " Not registering to client event manager already registered");
        }
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::deregisterListener(
    std::weak_ptr<INetworkSelectionListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INetworkSelectionListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_NETWORK_SELECTION_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::requestNetworkSelectionMode
    (SelectionModeInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestNetworkSelectionModeRequest request;
    ::telStub::RequestNetworkSelectionModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestNetworkSelectionMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    NetworkModeInfo info = {};
    info.mode = static_cast<telux::tel::NetworkSelectionMode>(response.mode());
    info.mnc = response.mnc();
    info.mcc = response.mcc();
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, info, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(info, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::setNetworkSelectionMode
    (NetworkSelectionMode selectMode, std::string mcc, std::string mnc,
    common::ResponseCallback callback ) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetNetworkSelectionModeRequest request;
    ::telStub::SetNetworkSelectionModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_mode(static_cast<telStub::NetworkSelectionMode_Mode>(selectMode));
    request.set_mcc(mcc);
    request.set_mnc(mnc);
    grpc::Status reqstatus = stub_->SetNetworkSelectionMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                }
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::requestPreferredNetworks(
    PreferredNetworksCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestPreferredNetworksRequest request;
    ::telStub::RequestPreferredNetworksReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestPreferredNetworks(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo;
    for (int i = 0; i < response.preferred_size(); i++) {
        telux::tel::PreferredNetworkInfo info;
        info.mcc = static_cast<int>(response.mutable_preferred(i)->mcc());
        info.mnc = static_cast<int>(response.mutable_preferred(i)->mnc());
        for (auto &r : response.mutable_preferred(i)->types()) {
            int tmp =  static_cast<int>(r);
            info.ratMask.set(tmp);
        }
        preferredNetworks3gppInfo.emplace_back(info);
    }
    std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo;
    for (int i = 0; i < response.static_preferred_size(); i++) {
        telux::tel::PreferredNetworkInfo info;
        info.mcc = static_cast<int>(response.mutable_static_preferred(i)->mcc());
        info.mnc = static_cast<int>(response.mutable_static_preferred(i)->mnc());
        for (auto &r : response.mutable_static_preferred(i)->types()) {
            info.ratMask.set(static_cast<int>(r));
        }
        staticPreferredNetworksInfo.emplace_back(info);
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, preferredNetworks3gppInfo, staticPreferredNetworksInfo, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(preferredNetworks3gppInfo, staticPreferredNetworksInfo, error);
            }
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::setPreferredNetworks(
    std::vector<PreferredNetworkInfo> preferredNetworksInfo, bool clearPrevious,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetPreferredNetworksRequest request;
    ::telStub::SetPreferredNetworksReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_clear_previous(clearPrevious);
    int size = preferredNetworksInfo.size();
    for (int i = 0; i < size; i++) {
        telStub::PreferredNetworkInfo* info = request.add_preferred_networks_info();
        info->set_mcc(preferredNetworksInfo[i].mcc);
        info->set_mnc(preferredNetworksInfo[i].mnc);
        int dataSize = (preferredNetworksInfo[i].ratMask).size();
        for (int j = 0; j < dataSize; j++)
        {
            if ((preferredNetworksInfo[i].ratMask).test(j)) {
                info->add_types(static_cast<telStub::RatType_Type>(j));
            }
        }
    }
    grpc::Status reqstatus = stub_->SetPreferredNetworks(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(error);
            }
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::performNetworkScan(NetworkScanInfo info,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::PerformNetworkScanRequest request;
    ::telStub::PerformNetworkScanReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_scan_type(static_cast<::telStub::NetworkScanType>(info.scanType));
    int size = (info.ratMask).size();
    for (int j = 0; j < size ; j++)
    {
        if((info.ratMask).test(j)) {
            request.add_rat_types(static_cast<::telStub::RatType_Type>(j));
        }
    }

    grpc::Status reqstatus = stub_->PerformNetworkScan(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::requestNetworkSelectionMode
    (SelectionModeResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestNetworkSelectionModeRequest request;
    ::telStub::RequestNetworkSelectionModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestNetworkSelectionMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    NetworkSelectionMode mode = static_cast<telux::tel::NetworkSelectionMode>(response.mode());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, mode, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(mode, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::ErrorCode NetworkSelectionManagerStub::setLteDubiousCell(
        const std::vector<LteDubiousCell> &lteDbCellList) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::telStub::SetLteDubiousCellRequest request;
    ::telStub::SetLteDubiousCellReply response;
    telux::common::ErrorCode err = telux::common::ErrorCode::GENERIC_FAILURE;

    ClientContext context;
    request.set_slot_id(phoneId_);

    grpc::Status reqstatus = stub_->SetLteDubiousCell(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }

    err = static_cast<telux::common::ErrorCode>(response.error());
    return err;
}

telux::common::ErrorCode NetworkSelectionManagerStub::setNrDubiousCell(
        const std::vector<NrDubiousCell> &nrDbCellList) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " NetworkSelection Manager is not ready");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::telStub::SetNrDubiousCellRequest request;
    ::telStub::SetNrDubiousCellReply response;
    telux::common::ErrorCode err = telux::common::ErrorCode::GENERIC_FAILURE;

    ClientContext context;
    request.set_slot_id(phoneId_);

    grpc::Status reqstatus = stub_->SetNrDubiousCell(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }

    err = static_cast<telux::common::ErrorCode>(response.error());
    return err;
}

void NetworkSelectionManagerStub::onEventUpdate(google::protobuf::Any event) {
    if (event.Is<::telStub::SelectionModeChangeEvent>()) {
        ::telStub::SelectionModeChangeEvent selectionModeChangeEvent;
        event.UnpackTo(&selectionModeChangeEvent);
        handleSelectionModeChanged(selectionModeChangeEvent);
    } else if(event.Is<::telStub::NetworkScanResultsChangeEvent>()) {
        ::telStub::NetworkScanResultsChangeEvent networkScanResultsChangeEvent;
        event.UnpackTo(&networkScanResultsChangeEvent);
        handleNetworkScanResultsChanged(networkScanResultsChangeEvent);
    }
}

telux::common::Status NetworkSelectionManagerStub::performNetworkScan(
    NetworkScanCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

void NetworkSelectionManagerStub::handleSelectionModeChanged
    (::telStub::SelectionModeChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    NetworkModeInfo info = {};
    info.mode = static_cast<telux::tel::NetworkSelectionMode>(event.mode());
    info.mnc = event.mnc();
    info.mcc = event.mcc();
    std::vector<std::weak_ptr<INetworkSelectionListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onSelectionModeChanged(info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void NetworkSelectionManagerStub::handleNetworkScanResultsChanged
    (::telStub::NetworkScanResultsChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    telux::tel::NetworkScanStatus status =
        static_cast<telux::tel::NetworkScanStatus>(event.status());
    // update OperatorInfo
    std::vector<OperatorInfo> infos = {};
    for (int i = 0; i < event.operator_infos_size(); i++) {
        OperatorStatus operatorStatus = {};
        operatorStatus.inUse =
            static_cast<InUseStatus>(
            event.mutable_operator_infos(i)->mutable_operator_status()->inuse());
        operatorStatus.roaming =
            static_cast<RoamingStatus>(
            event.mutable_operator_infos(i)->mutable_operator_status()->roaming());
        operatorStatus.forbidden =
            static_cast<ForbiddenStatus>(
            event.mutable_operator_infos(i)->mutable_operator_status()->forbidden());
        operatorStatus.preferred =
            static_cast<PreferredStatus>(
            event.mutable_operator_infos(i)->mutable_operator_status()->preferred());
        OperatorInfo info(event.operator_infos(i).name(),
            event.operator_infos(i).mcc(),
            event.operator_infos(i).mnc(),
            static_cast<RadioTechnology>(event.operator_infos(i).rat()),
            operatorStatus);
        infos.emplace_back(info);
    }
    std::vector<std::weak_ptr<INetworkSelectionListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onNetworkScanResults(status, infos);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

OperatorInfo::OperatorInfo(std::string networkName, std::string mcc, std::string mnc,
                           OperatorStatus operatorStatus)
   : networkName_(networkName)
   , mcc_(mcc)
   , mnc_(mnc)
   , rat_(telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN)
   , operatorStatus_(operatorStatus) {
   LOG(DEBUG, "Operator Info");
}

OperatorInfo::OperatorInfo(std::string networkName, std::string mcc, std::string mnc,
   telux::tel::RadioTechnology rat, OperatorStatus operatorStatus)
   : networkName_(networkName)
   , mcc_(mcc)
   , mnc_(mnc)
   , rat_(rat)
   , operatorStatus_(operatorStatus) {
}

std::string OperatorInfo::getName() {
   return networkName_;
}

std::string OperatorInfo::getMcc() {
   return mcc_;
}

std::string OperatorInfo::getMnc() {
   return mnc_;
}

OperatorStatus OperatorInfo::getStatus() {
   return operatorStatus_;
}

RadioTechnology OperatorInfo::getRat() {
   return rat_;
}

