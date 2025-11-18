/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <grpcpp/grpcpp.h>

#include "DataConnectionManagerStub.hpp"
#include "DataProfileManagerStub.hpp"
#include "DataEventListener.hpp"
#include "DataUtilsStub.hpp"

#include "common/Logger.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define MAX_PROFILE_ID 255
#define MIN_PROFILE_ID 0
#define DEFAULT_NOTIFICATION_DELAY 2000

namespace telux {
namespace data {

DataConnectionManagerStub::DataConnectionManagerStub(SlotId slotId)
{
    LOG(DEBUG, __FUNCTION__, " Initializing DataConnectionManagerStub for slot:",
        static_cast<int>(slotId));
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    slotId_ = slotId;
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

DataConnectionManagerStub::~DataConnectionManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }

    std::vector<std::string> filters = {DATA_CONNECTION_FILTER};
    auto &clientEventManager = telux::common::ClientEventManager::getInstance();
    clientEventManager.deregisterListener(eventListener_, filters);
    cleanup();
}

telux::common::Status DataConnectionManagerStub::init(
    telux::common::InitResponseCb callback) {
    LOG(INFO, __FUNCTION__);

    auto f = std::async(std::launch::async,
             [this, callback]() {
                this->initSync(callback);
            }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void DataConnectionManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::DataConnectionManager>();

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
        eventListener_ = std::make_shared<DataEventListener>(shared_from_this());
        std::vector<std::string> filters = {DATA_CONNECTION_FILTER};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(eventListener_, filters);

    } while (0);

    bool isSubsystemReady = (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
        true : false;
    setSubSystemStatus(cbStatus);
    setSubsystemReady(isSubsystemReady);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        callback(cbStatus);
    }

    //To fetch the datacalls cached on server side
    requestConnectedDataCallLists();
}

void DataConnectionManagerStub::requestConnectedDataCallLists() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    ::dataStub::CachedDataCallsRequest request;
    ::dataStub::CachedDataCalls response;
    std::list<telux::data::IpAddrInfo> ipAddrList;

    request.set_slot_id(slotId_);
    stub_->requestConnectedDataCallLists(&context, request, &response);

    std::shared_ptr<DataCallStub> call;
    for (auto idx = 0; idx < response.datacalls_size(); idx++) {
        auto datacall = response.mutable_datacalls(idx);
        call =
            std::make_shared<telux::data::DataCallStub>(
            datacall->iface_name());
        int profileId = datacall->profile_id();
        call->setProfileId(profileId);
        call->setSlotId(slotId_);

        IpFamilyType ipFamilyType =  static_cast<IpFamilyType>(
            DataUtilsStub::convertIpFamilyStringToEnum(
            datacall->ip_family_type()));
        call->setIpFamilyType(ipFamilyType);

        IpAddrInfo ipv4Addr, ipv6Addr;
        ipv4Addr.ifAddress = datacall->ipv4_address();
        ipv4Addr.gwAddress = datacall->gwv4_address();
        ipv4Addr.primaryDnsAddress = datacall->v4dns_primary_address();
        ipv4Addr.secondaryDnsAddress = datacall->v4dns_secondary_address();
        ipv6Addr.ifAddress = datacall->ipv6_address();
        ipv6Addr.gwAddress = datacall->gwv6_address();
        ipv6Addr.primaryDnsAddress = datacall->v6dns_primary_address();
        ipv6Addr.secondaryDnsAddress = datacall->v6dns_secondary_address();

        // setting to defaults.
        call->setTechPreference(TechPreference::TP_3GPP);
        call->setDataBearerTechnology(DataBearerTechnology::LTE);
        call->setOperationType(OperationType::DATA_LOCAL);

        bool ipv4Supported = (ipv4Addr.ifAddress.length() == 0)? false : true;
        bool ipv6Supported = (ipv6Addr.ifAddress.length() == 0)? false : true;
        ipAddrList.clear();
        if (ipFamilyType == IpFamilyType::IPV4 || ipFamilyType == IpFamilyType::IPV4V6) {
            if (ipv4Supported) {
                ipAddrList.emplace_back(ipv4Addr);
                call->setDataCallStatus(DataCallStatus::NET_CONNECTED, IpFamilyType::IPV4);
            } else {
                call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV4);
            }
        }

        if (ipFamilyType == IpFamilyType::IPV6 || ipFamilyType == IpFamilyType::IPV4V6) {
            if (ipv6Supported) {
                ipAddrList.emplace_back(ipv6Addr);
                call->setDataCallStatus(DataCallStatus::NET_CONNECTED, IpFamilyType::IPV6);
            } else {
                call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV6);
            }
        }
        call->setIpAddrList(ipAddrList);
        cacheDataCalls_.insert({profileId, call});
    }
}

telux::common::Status DataConnectionManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
    setSubSystemStatus(telux::common::ServiceStatus::SERVICE_FAILED);
    setSubsystemReady(false);

    ClientContext context;
    ::dataStub::ClientInfo request;
    ::google::protobuf::Empty response;
    request.set_client_id(getpid());

    stub_->CleanUpService(&context, request, &response);

    dataCalls_.clear();
    cacheDataCalls_.clear();

    return telux::common::Status::SUCCESS;
}

void DataConnectionManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

void DataConnectionManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

std::future<bool> DataConnectionManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return DataConnectionManagerStub::waitForInitialization(); });
    return future;
}

bool DataConnectionManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

telux::common::ServiceStatus DataConnectionManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

bool DataConnectionManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

void DataConnectionManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

telux::common::Status DataConnectionManagerStub::setDefaultProfile(OperationType oprType,
    uint8_t profileId, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (profileId < MIN_PROFILE_ID && profileId > MAX_PROFILE_ID) {
        LOG(ERROR, __FUNCTION__, " Invalid profile id");
        return telux::common::Status::INVALIDPARAM;
    }

    if (OperationType::DATA_REMOTE == oprType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetDefaultProfileRequest request;
    ::dataStub::DefaultReply response;

    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_operation_type((::dataStub::OperationType)oprType);
    request.set_profile_id(profileId);

    grpc::Status reqStatus = stub_->SetDefaultProfile(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " SetDefaultProfile request failed");
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

telux::common::Status DataConnectionManagerStub::getDefaultProfile(OperationType oprType,
    DefaultProfileIdResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (OperationType::DATA_REMOTE == oprType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::GetDefaultProfileReply response;
    ::dataStub::GetDefaultProfileRequest request;
    ClientContext context;

    SlotId slotID = slotId_;

    request.set_slot_id(slotID);
    request.set_operation_type((::dataStub::OperationType)oprType);

    grpc::Status reqStatus = stub_->GetDefaultProfile(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " GetDefaultProfile request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        uint8_t profileId = response.profile_id();
        LOG(DEBUG, __FUNCTION__, " profileId:", profileId);
        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
             [this, error, profileId, slotID, delay,callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(profileId, slotID, error);
               }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataConnectionManagerStub::setRoamingMode(bool enable,
    uint8_t profileId, OperationType operationType,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (profileId < MIN_PROFILE_ID && profileId > MAX_PROFILE_ID) {
        LOG(ERROR, __FUNCTION__, " Invalid profile id");
        return telux::common::Status::INVALIDPARAM;
    }

    if (OperationType::DATA_REMOTE == operationType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetRoamingModeRequest request;
    ::dataStub::DefaultReply response;

    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_operation_type((::dataStub::OperationType)operationType);
    request.set_profile_id(profileId);
    request.set_roaming_mode(enable);

    grpc::Status reqStatus = stub_->SetRoamingMode(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " SetRoamingMode request failed");
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

telux::common::Status DataConnectionManagerStub::requestRoamingMode(uint8_t profileId,
    OperationType operationType, requestRoamingModeResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (profileId < MIN_PROFILE_ID && profileId > MAX_PROFILE_ID) {
        LOG(ERROR, __FUNCTION__, " Invalid profile id");
        return telux::common::Status::INVALIDPARAM;
    }

    if (OperationType::DATA_REMOTE == operationType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RequestRoamingModeReply response;
    ::dataStub::RequestRoamingModeRequest request;
    ClientContext context;

    SlotId slotID = slotId_;

    request.set_slot_id(slotID);
    request.set_operation_type((::dataStub::OperationType)operationType);
    request.set_profile_id(profileId);

    grpc::Status reqStatus = stub_->RequestRoamingMode(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " RequestRoamingMode request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        bool mode = response.roaming_mode();
        uint8_t profile_Id = response.profile_id();
        LOG(DEBUG, __FUNCTION__, " profile_Id:", profile_Id, " mode:", mode);
        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
             [this, mode, profile_Id, error, delay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(mode, profile_Id, error);
               }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

void DataConnectionManagerStub::invokeDataConnectionListener(
    std::shared_ptr<IDataCall> call) {
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onDataCallInfoChanged(call);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void DataConnectionManagerStub::handleStartDataCallEvent(
    ::dataStub::StartDataCallEvent startEvent) {
    std::lock_guard<std::mutex> lck(mtx_);
    int profileId = startEvent.profile_id();
    SlotId slotId = static_cast<SlotId>(startEvent.slot_id());
    std::string ifaceName = startEvent.iface_name();
    IpFamilyType ipFamilyType =  static_cast<IpFamilyType>(
        DataUtilsStub::convertIpFamilyStringToEnum(startEvent.ip_family_type()));
    std::string ipv4Address = startEvent.ipv4_address();
    std::string gwv4Address = startEvent.gwv4_address();
    std::string v4dnsPrimaryAddress = startEvent.v4dns_primary_address();
    std::string v4dnsSecondaryAddress = startEvent.v4dns_secondary_address();
    std::string ipv6Address = startEvent.ipv6_address();
    std::string gwv6Address = startEvent.gwv6_address();
    std::string v6dnsPrimaryAddress = startEvent.v6dns_primary_address();
    std::string v6dnsSecondaryAddress = startEvent.v6dns_secondary_address();
    std::list<telux::data::IpAddrInfo> ipAddrList;

    if (slotId != slotId_)
        return;

    LOG(DEBUG, __FUNCTION__, " connecting datacall for profileId:", profileId);

    bool ipv4Supported = (ipv4Address.length() == 0)? false : true;
    bool ipv6Supported = (ipv6Address.length() == 0)? false : true;
    std::shared_ptr<DataCallStub> call;

    if (dataCalls_.find(profileId) != dataCalls_.end()) {
        call = dataCalls_[profileId];
    } else if (cacheDataCalls_.find(profileId) != cacheDataCalls_.end()) {
        call = cacheDataCalls_[profileId];
    } else {
        call =
            std::make_shared<telux::data::DataCallStub>(ifaceName);
        call->setProfileId(profileId);
        call->setSlotId(slotId_);
        call->setIpFamilyType(ipFamilyType);
        // setting to defaults.
        call->setTechPreference(TechPreference::TP_3GPP);
        call->setDataBearerTechnology(DataBearerTechnology::LTE);
        call->setOperationType(OperationType::DATA_LOCAL);

        cacheDataCalls_.insert({profileId, call});
    }

    if (call) {
        auto currentFamily = call->getIpFamilyType();
        if (ipFamilyType != currentFamily) {
            call->setIpFamilyType(ipFamilyType);
        }
        call->setInterfaceName(ifaceName);
    }

    std::shared_ptr<IDataCall> baseCallPtr =
        std::static_pointer_cast<IDataCall>(call);

    if (ipv4Supported || ipv6Supported) {
        IpAddrInfo ipv4Addr, ipv6Addr;
        ipv4Addr.ifAddress = ipv4Address;
        ipv4Addr.gwAddress = gwv4Address;
        ipv4Addr.primaryDnsAddress = v4dnsPrimaryAddress;
        ipv4Addr.secondaryDnsAddress = v4dnsSecondaryAddress;

        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_NOTIFICATION_DELAY));
        if (ipFamilyType == IpFamilyType::IPV4 || ipFamilyType == IpFamilyType::IPV4V6) {
            if (ipv4Supported) {
                call->setDataCallStatus(DataCallStatus::NET_CONNECTED, IpFamilyType::IPV4);
            } else {
                call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV4);
            }
            ipAddrList.emplace_back(ipv4Addr);
            ipAddrList.emplace_back(ipv6Addr);
            call->setIpAddrList(ipAddrList);
            auto f = std::async(std::launch::async,
                [this, baseCallPtr]() {
                    invokeDataConnectionListener(baseCallPtr);
                }).share();
            f.wait();
        } else {
            //if it is only IPV6 call, to match the device o/p
            ipv4Addr.ifAddress = "0.0.0.0";
            ipv4Addr.gwAddress = "0.0.0.0";
            ipv4Addr.primaryDnsAddress = "0.0.0.0";
            ipv4Addr.secondaryDnsAddress = "0.0.0.0";
        }

        ipv6Addr.ifAddress = ipv6Address;
        ipv6Addr.gwAddress = gwv6Address;
        ipv6Addr.primaryDnsAddress = v6dnsPrimaryAddress;
        ipv6Addr.secondaryDnsAddress = v6dnsSecondaryAddress;
        ipAddrList.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_NOTIFICATION_DELAY));
        if (ipFamilyType == IpFamilyType::IPV6 || ipFamilyType == IpFamilyType::IPV4V6) {
            if (ipv6Supported) {
                call->setDataCallStatus(DataCallStatus::NET_CONNECTED, IpFamilyType::IPV6);
            } else {
                call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV6);
            }
            ipAddrList.emplace_back(ipv4Addr);
            ipAddrList.emplace_back(ipv6Addr);
            call->setIpAddrList(ipAddrList);
            auto f = std::async(std::launch::async,
                [this, baseCallPtr]() {
                    invokeDataConnectionListener(baseCallPtr);
                }).share();
            f.wait();
        }
        LOG(DEBUG, __FUNCTION__, " datacall connected for profileId:", profileId,
            " ipFamilyType:", static_cast<int>(ipFamilyType));
    } else {
        DataCallEndReason reason;
        reason.type = EndReasonType::CE_3GPP_SPEC_DEFINED;
        reason.specCode = SpecReasonCode::CE_UNKNOWN_APN;
        call->setDataCallEndReason(reason);

        if (ipFamilyType == IpFamilyType::IPV4 || ipFamilyType == IpFamilyType::IPV4V6) {
            call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV4);
            auto f = std::async(std::launch::async,
                [this, baseCallPtr]() {
                    invokeDataConnectionListener(baseCallPtr);
                }).share();
            taskQ_->add(f);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_NOTIFICATION_DELAY));
        if (ipFamilyType == IpFamilyType::IPV6 || ipFamilyType == IpFamilyType::IPV4V6) {
            call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV6);
            auto f = std::async(std::launch::async,
                    [this, baseCallPtr]() {
                        invokeDataConnectionListener(baseCallPtr);
                    }).share();
            taskQ_->add(f);
        }

        dataCalls_.erase(profileId);
        cacheDataCalls_.erase(profileId);
        LOG(DEBUG, __FUNCTION__, " failed to connect datacall");
    }
}

telux::common::Status DataConnectionManagerStub::startDataCall(
    const DataCallParams &dataCallParams, DataCallResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (dataCallParams.profileId < MIN_PROFILE_ID &&
        dataCallParams.profileId > MAX_PROFILE_ID) {
        LOG(ERROR, __FUNCTION__, " Invalid profile id");
        return telux::common::Status::INVALIDPARAM;
    }

    if (dataCallParams.ipFamilyType != IpFamilyType::IPV4 &&
        dataCallParams.ipFamilyType != IpFamilyType::IPV6
        && dataCallParams.ipFamilyType != IpFamilyType::IPV4V6) {
        LOG(ERROR, __FUNCTION__, " Invalid ip family type");
        return telux::common::Status::INVALIDPARAM;
    }

    if (OperationType::DATA_REMOTE == dataCallParams.operationType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::DefaultReply response;
    ::dataStub::DataCallInputParams request;
    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_profile_id(dataCallParams.profileId);
    request.mutable_ip_family_type()->set_ip_family_type(
        (::dataStub::IpFamilyType::Type)dataCallParams.ipFamilyType);
    request.set_operation_type(
        (::dataStub::OperationType)dataCallParams.operationType);
    request.set_client_id(getpid());
    request.set_iface_name(dataCallParams.interfaceName);

    grpc::Status reqStatus = stub_->StartDatacall(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    std::shared_ptr<DataCallStub> call;
    std::shared_ptr<IDataCall> baseCallPtr = nullptr;

    do {
        if (status == telux::common::Status::SUCCESS &&
            error == telux::common::ErrorCode::SUCCESS) {
            if (!reqStatus.ok()) {
                LOG(ERROR, __FUNCTION__, " StartDatacall request failed");
                error = telux::common::ErrorCode::INTERNAL_ERROR;
                break;
            }

            std::lock_guard<std::mutex> lck(mtx_);
            if (dataCalls_.find(dataCallParams.profileId) != dataCalls_.end()) {
                LOG(DEBUG, __FUNCTION__, " datacall already exists");
                baseCallPtr = std::static_pointer_cast<IDataCall>(
                    dataCalls_[dataCallParams.profileId]);
                break;
            } else if (cacheDataCalls_.find(dataCallParams.profileId)
                != cacheDataCalls_.end()) {
                LOG(DEBUG, __FUNCTION__, " datacall already exists");
                baseCallPtr = std::static_pointer_cast<IDataCall>(
                    cacheDataCalls_[dataCallParams.profileId]);
                //To handle scenario of datacall ownership, current client would also become owner
                dataCalls_[dataCallParams.profileId] =
                    cacheDataCalls_[dataCallParams.profileId];
                cacheDataCalls_.erase(dataCallParams.profileId);
                break;
            }

            LOG(DEBUG, __FUNCTION__, " creating new datacall for profile:",
                dataCallParams.profileId);
            call =
                std::make_shared<telux::data::DataCallStub>("");
            baseCallPtr =
                std::static_pointer_cast<IDataCall>(call);

            call->setProfileId(dataCallParams.profileId);
            call->setSlotId(slotId_);
            call->setIpFamilyType(dataCallParams.ipFamilyType);
            // setting to defaults.
            call->setTechPreference(TechPreference::TP_3GPP);
            call->setDataBearerTechnology(DataBearerTechnology::LTE);
            call->setOperationType(dataCallParams.operationType);
            if (dataCallParams.ipFamilyType == IpFamilyType::IPV4 ||
                dataCallParams.ipFamilyType == IpFamilyType::IPV4V6) {
                call->setDataCallStatus(DataCallStatus::NET_CONNECTING, IpFamilyType::IPV4);
            }
            if (dataCallParams.ipFamilyType == IpFamilyType::IPV6 ||
                dataCallParams.ipFamilyType == IpFamilyType::IPV4V6) {
                call->setDataCallStatus(DataCallStatus::NET_CONNECTING, IpFamilyType::IPV6);
            }

            dataCalls_[dataCallParams.profileId] = call;
        }
    } while(0);

    if (callback && (delay != SKIP_CALLBACK)) {
        auto f = std::async(std::launch::async,
        [this, baseCallPtr, error, delay, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(baseCallPtr, error);
        }).share();
        taskQ_->add(f);
    }

    return status;
}

telux::common::Status DataConnectionManagerStub::startDataCall(int profileId,
    IpFamilyType ipFamilyType, DataCallResponseCb callback, OperationType operationType,
    std::string apn) {
    LOG(DEBUG, __FUNCTION__);

    if (profileId < MIN_PROFILE_ID && profileId > MAX_PROFILE_ID) {
        LOG(ERROR, __FUNCTION__, " Invalid profile id");
        return telux::common::Status::INVALIDPARAM;
    }

    if (ipFamilyType != IpFamilyType::IPV4 && ipFamilyType != IpFamilyType::IPV6
        && ipFamilyType != IpFamilyType::IPV4V6) {
        LOG(ERROR, __FUNCTION__, " Invalid ip family type");
        return telux::common::Status::INVALIDPARAM;
    }

    if (OperationType::DATA_REMOTE == operationType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::DefaultReply response;
    ::dataStub::DataCallInputParams request;
    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_profile_id(profileId);
    request.mutable_ip_family_type()->set_ip_family_type(
        (::dataStub::IpFamilyType::Type)ipFamilyType);
    request.set_operation_type((::dataStub::OperationType)operationType);
    request.set_client_id(getpid());

    grpc::Status reqStatus = stub_->StartDatacall(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    std::shared_ptr<DataCallStub> call;
    std::shared_ptr<IDataCall> baseCallPtr = nullptr;

    do {
        if (status == telux::common::Status::SUCCESS &&
            error == telux::common::ErrorCode::SUCCESS) {
            if (!reqStatus.ok()) {
                LOG(ERROR, __FUNCTION__, " StartDatacall request failed");
                error = telux::common::ErrorCode::INTERNAL_ERROR;
                break;
            }

            std::lock_guard<std::mutex> lck(mtx_);
            if (dataCalls_.find(profileId) != dataCalls_.end()) {
                LOG(DEBUG, __FUNCTION__, " datacall already exists");
                baseCallPtr = std::static_pointer_cast<IDataCall>(dataCalls_[profileId]);
                break;
            } else if (cacheDataCalls_.find(profileId) != cacheDataCalls_.end()) {
                LOG(DEBUG, __FUNCTION__, " datacall already exists");
                baseCallPtr = std::static_pointer_cast<IDataCall>(cacheDataCalls_[profileId]);
                //To handle scenario of datacall ownership, current client would also become owner
                dataCalls_[profileId] = cacheDataCalls_[profileId];
                cacheDataCalls_.erase(profileId);
                break;
            }

            LOG(DEBUG, __FUNCTION__, " creating new datacall for profile:", profileId);
            call =
                std::make_shared<telux::data::DataCallStub>("");
            baseCallPtr =
                std::static_pointer_cast<IDataCall>(call);

            call->setProfileId(profileId);
            call->setSlotId(slotId_);
            call->setIpFamilyType(ipFamilyType);
            // setting to defaults.
            call->setTechPreference(TechPreference::TP_3GPP);
            call->setDataBearerTechnology(DataBearerTechnology::LTE);
            call->setOperationType(operationType);
            if (ipFamilyType == IpFamilyType::IPV4 || ipFamilyType == IpFamilyType::IPV4V6) {
                call->setDataCallStatus(DataCallStatus::NET_CONNECTING, IpFamilyType::IPV4);
            }
            if (ipFamilyType == IpFamilyType::IPV6 || ipFamilyType == IpFamilyType::IPV4V6) {
                call->setDataCallStatus(DataCallStatus::NET_CONNECTING, IpFamilyType::IPV6);
            }

            dataCalls_[profileId] = call;
        }
    } while(0);

    if (callback && (delay != SKIP_CALLBACK)) {
        auto f = std::async(std::launch::async,
        [this, baseCallPtr, error, delay, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(baseCallPtr, error);
        }).share();
        taskQ_->add(f);
    }

    return status;
}

telux::common::Status DataConnectionManagerStub::stopDataCall(
    const DataCallParams &dataCallParams, DataCallResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (dataCallParams.profileId < MIN_PROFILE_ID &&
        dataCallParams.profileId > MAX_PROFILE_ID) {
        LOG(ERROR, __FUNCTION__, " Invalid profile id");
        return telux::common::Status::INVALIDPARAM;
    }

    if (dataCallParams.ipFamilyType != IpFamilyType::IPV4
        && dataCallParams.ipFamilyType != IpFamilyType::IPV6
        && dataCallParams.ipFamilyType != IpFamilyType::IPV4V6) {
        LOG(ERROR, __FUNCTION__, " Invalid ip family type");
        return telux::common::Status::INVALIDPARAM;
    }

    if (OperationType::DATA_REMOTE == dataCallParams.operationType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay = DEFAULT_DELAY;

    ::dataStub::DefaultReply response;
    ::dataStub::DataCallInputParams request;
    ClientContext context;
    std::shared_ptr<DataCallStub> call = nullptr;
    std::shared_ptr<IDataCall> baseCallPtr = nullptr;

    std::lock_guard<std::mutex> lck(mtx_);
    do {
        if (dataCalls_.find(dataCallParams.profileId) != dataCalls_.end()) {
            call = dataCalls_[dataCallParams.profileId];
        }

        if (call == nullptr) {
            LOG(DEBUG, __FUNCTION__, " datacall doesn't exist");
            error = telux::common::ErrorCode::INVALID_OPERATION;
            break;
        }

        request.set_slot_id(slotId_);
        request.set_profile_id(dataCallParams.profileId);
        request.mutable_ip_family_type()->set_ip_family_type(
            (::dataStub::IpFamilyType::Type)dataCallParams.ipFamilyType);
        request.set_operation_type(
            (::dataStub::OperationType)dataCallParams.operationType);
        request.set_client_id(getpid());

        grpc::Status reqStatus = stub_->StopDatacall(&context, request, &response);

        error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        delay = static_cast<int>(response.delay());

        if (status == telux::common::Status::SUCCESS) {
            if (!reqStatus.ok()) {
                LOG(ERROR, __FUNCTION__, " StopDatacall request failed");
                error = telux::common::ErrorCode::INTERNAL_ERROR;
                break;
            }
            IpAddrInfo ipv4Addr, ipv6Addr;
            baseCallPtr =
                    std::static_pointer_cast<IDataCall>(call);

            if (call) {
                DataCallEndReason endReason, emptyDataCallEndReason;
                endReason.type = EndReasonType::CE_CALL_MANAGER_DEFINED;
                endReason.cmCode = CallManagerReasonCode::CE_CLIENT_END;
                call->setInterfaceName("");
                call->setTechPreference(TechPreference::TP_3GPP);
                call->setDataBearerTechnology(DataBearerTechnology::UNKNOWN);
                call->setOperationType(dataCallParams.operationType);
                if ((dataCallParams.ipFamilyType == IpFamilyType::IPV4) ||
                    (dataCallParams.ipFamilyType == IpFamilyType::IPV4V6)) {
                    call->setDataCallStatus(DataCallStatus::NET_DISCONNECTING, IpFamilyType::IPV4);
                }

                if ((dataCallParams.ipFamilyType == IpFamilyType::IPV6) ||
                    (dataCallParams.ipFamilyType == IpFamilyType::IPV4V6)) {
                    call->setDataCallStatus(DataCallStatus::NET_DISCONNECTING, IpFamilyType::IPV6);
                }
                call->setDataCallEndReason(emptyDataCallEndReason);
            }
        }
    } while (0);

    if (callback) {
        auto f = std::async(std::launch::async,
                [this, baseCallPtr, error, delay, callback]() {
                    if (callback && (delay != SKIP_CALLBACK)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(baseCallPtr, error);
                    }
                }).share();
        f.wait();
    }

    return status;
}

void DataConnectionManagerStub::handleStopDataCallEvent(
    ::dataStub::StopDataCallEvent stopEvent) {
    LOG(DEBUG, __FUNCTION__);

    int profileId = stopEvent.profile_id();
    SlotId slotId = static_cast<SlotId>(stopEvent.slot_id());
    IpFamilyType ipFamilyType =
        static_cast<IpFamilyType>(
        DataUtilsStub::convertIpFamilyStringToEnum(stopEvent.ip_family_type()));
    std::list<telux::data::IpAddrInfo> ipAddrList;
    std::lock_guard<std::mutex> lck(mtx_);
    if (slotId != slotId_)
        return;

    LOG(DEBUG, __FUNCTION__, " disconnecting datacall");

    std::shared_ptr<DataCallStub> call;

    if (dataCalls_.find(profileId) != dataCalls_.end()) {
        call = dataCalls_[profileId];
    } else {
        call = cacheDataCalls_[profileId];

        if (call == nullptr) {
            return;
        }

        DataCallEndReason endReason;
        endReason.type = EndReasonType::CE_CALL_MANAGER_DEFINED;
        endReason.cmCode = CallManagerReasonCode::CE_CLIENT_END;
        call->setTechPreference(TechPreference::TP_3GPP);
        call->setDataBearerTechnology(DataBearerTechnology::UNKNOWN);
        call->setOperationType(OperationType::DATA_LOCAL);
        call->setDataCallEndReason(endReason);
    }

    std::shared_ptr<IDataCall> baseCallPtr =
        std::static_pointer_cast<IDataCall>(call);

    if (call->getIpFamilyType() == ipFamilyType) {
        DataCallEndReason reason;
        reason.type = EndReasonType::CE_CALL_MANAGER_DEFINED;
        reason.cmCode = CallManagerReasonCode::CE_CLIENT_END;
        call->setDataCallEndReason(reason);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_NOTIFICATION_DELAY));

    call->setInterfaceName("");
    call->setIpAddrList(ipAddrList);
    if ((ipFamilyType == IpFamilyType::IPV4) || (ipFamilyType == IpFamilyType::IPV4V6)) {
        call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV4);
        auto f = std::async(std::launch::async,
            [this, baseCallPtr]() {
                    invokeDataConnectionListener(baseCallPtr);
            }).share();
        f.wait();
    }

    if ((ipFamilyType == IpFamilyType::IPV6) || (ipFamilyType == IpFamilyType::IPV4V6)) {
        call->setDataCallStatus(DataCallStatus::NET_NO_NET, IpFamilyType::IPV6);
        auto f = std::async(std::launch::async,
            [this, baseCallPtr]() {
                    invokeDataConnectionListener(baseCallPtr);
            }).share();
        f.wait();
    }
    if (call->getDataCallStatus() == DataCallStatus::NET_NO_NET) {
        dataCalls_.erase(profileId);
        cacheDataCalls_.erase(profileId);
    }

    LOG(DEBUG, __FUNCTION__, " datacall disconnected for profileId:", profileId,
            " ipFamilyType:", static_cast<int>(ipFamilyType));
}

telux::common::Status DataConnectionManagerStub::stopDataCall(int profileId,
    IpFamilyType ipFamilyType, DataCallResponseCb callback, OperationType operationType,
    std::string apn) {
    LOG(DEBUG, __FUNCTION__);

    if (profileId < MIN_PROFILE_ID && profileId > MAX_PROFILE_ID) {
        LOG(ERROR, __FUNCTION__, " Invalid profile id");
        return telux::common::Status::INVALIDPARAM;
    }

    if (ipFamilyType != IpFamilyType::IPV4 && ipFamilyType != IpFamilyType::IPV6
        && ipFamilyType != IpFamilyType::IPV4V6) {
        LOG(ERROR, __FUNCTION__, " Invalid ip family type");
        return telux::common::Status::INVALIDPARAM;
    }

    if (OperationType::DATA_REMOTE == operationType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay = DEFAULT_DELAY;

    ::dataStub::DefaultReply response;
    ::dataStub::DataCallInputParams request;
    ClientContext context;
    std::shared_ptr<DataCallStub> call = nullptr;
    std::shared_ptr<IDataCall> baseCallPtr = nullptr;

    std::lock_guard<std::mutex> lck(mtx_);
    do {
        if (dataCalls_.find(profileId) != dataCalls_.end()) {
            call = dataCalls_[profileId];
        }

        if (call == nullptr) {
            LOG(DEBUG, __FUNCTION__, " datacall doesn't exist");
            error = telux::common::ErrorCode::INVALID_OPERATION;
            break;
        }

        request.set_slot_id(slotId_);
        request.set_profile_id(profileId);
        request.mutable_ip_family_type()->set_ip_family_type(
            (::dataStub::IpFamilyType::Type)ipFamilyType);
        request.set_operation_type((::dataStub::OperationType)operationType);
        request.set_client_id(getpid());

        grpc::Status reqStatus = stub_->StopDatacall(&context, request, &response);

        error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        delay = static_cast<int>(response.delay());

        if (status == telux::common::Status::SUCCESS) {
            if (!reqStatus.ok()) {
                LOG(ERROR, __FUNCTION__, " StopDatacall request failed");
                error = telux::common::ErrorCode::INTERNAL_ERROR;
                break;
            }
            IpAddrInfo ipv4Addr, ipv6Addr;
            baseCallPtr =
                    std::static_pointer_cast<IDataCall>(call);

            if (call) {
                DataCallEndReason endReason, emptyDataCallEndReason;
                endReason.type = EndReasonType::CE_CALL_MANAGER_DEFINED;
                endReason.cmCode = CallManagerReasonCode::CE_CLIENT_END;
                call->setInterfaceName("");
                call->setTechPreference(TechPreference::TP_3GPP);
                call->setDataBearerTechnology(DataBearerTechnology::UNKNOWN);
                call->setOperationType(operationType);
                if ((ipFamilyType == IpFamilyType::IPV4) || (ipFamilyType == IpFamilyType::IPV4V6)) {
                    call->setDataCallStatus(DataCallStatus::NET_DISCONNECTING, IpFamilyType::IPV4);
                }

                if ((ipFamilyType == IpFamilyType::IPV6) || (ipFamilyType == IpFamilyType::IPV4V6)) {
                    call->setDataCallStatus(DataCallStatus::NET_DISCONNECTING, IpFamilyType::IPV6);
                }
                call->setDataCallEndReason(emptyDataCallEndReason);
            }
        }
    } while (0);

    if (callback) {
        auto f = std::async(std::launch::async,
                [this, baseCallPtr, error, delay, callback]() {
                    if (callback && (delay != SKIP_CALLBACK)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(baseCallPtr, error);
                    }
                }).share();
        f.wait();
    }

    return status;
}

telux::common::Status DataConnectionManagerStub::registerListener(
    std::weak_ptr<IDataConnectionListener> listener) {

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

telux::common::Status DataConnectionManagerStub::deregisterListener(
    std::weak_ptr<telux::data::IDataConnectionListener> listener) {

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

telux::common::Status DataConnectionManagerStub::requestDataCallList(
    OperationType operationType, DataCallListResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (OperationType::DATA_REMOTE == operationType) {
        LOG(ERROR, __FUNCTION__, " Remote operation not supported");
        return telux::common::Status::NOTSUPPORTED;
    }

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RequestDataCallListReply response;
    ::dataStub::DataCallInputParams request;
    ClientContext context;

    request.set_slot_id(slotId_);
    request.set_operation_type((::dataStub::OperationType)operationType);

    grpc::Status reqStatus = stub_->RequestDatacallList(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " RequestDatacallList request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        std::lock_guard<std::mutex> lck(mtx_);
        std::vector<std::shared_ptr<IDataCall>> dataCalls;
        for (auto it: dataCalls_) {
            dataCalls.push_back(std::static_pointer_cast<IDataCall>(it.second));
        }

        for (auto it: cacheDataCalls_) {
            dataCalls.push_back(std::static_pointer_cast<IDataCall>(it.second));
        }
        LOG(DEBUG, __FUNCTION__, " found ", dataCalls.size(), " datacall");
        auto f = std::async(std::launch::async,
                [this, dataCalls, error, delay, callback]() {
                    if (callback && (delay != SKIP_CALLBACK)){
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(delay));
                        callback(dataCalls, error);
                    }
                }).share();
        taskQ_->add(f);
    }

    return status;
}

telux::common::Status DataConnectionManagerStub::requestThrottledApnInfo(ThrottleInfoCb callback)
{
    LOG(DEBUG, __FUNCTION__);

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data subsystem is not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::Status status = telux::common::Status::SUCCESS;
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    int delay;

    ::dataStub::ThrottleInfoReply response;
    ::dataStub::SlotInfo request;
    ClientContext context;

    request.set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->RequestThrottledApnInfo(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(0);
    status = static_cast<telux::common::Status>(0);
    delay = static_cast<int>(response.reply().delay());

    LOG(ERROR, __FUNCTION__, " Trigger stub_->RequestThrottledApnInfo has delay of ", delay,
    " with status: ", response.reply().status());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " RequestThrottledApnInfo request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        std::vector<APNThrottleInfo> apnThrottleInfo;
        LOG(DEBUG, __FUNCTION__, " throttled_apn_size: ",
        response.mutable_apn_throttle_info_list()->rep_apn_throttle_info_size());
        int size = response.mutable_apn_throttle_info_list()->rep_apn_throttle_info_size();
        for (auto idx = 0; idx < size; idx++) {
            APNThrottleInfo throttleInfo;
            auto apn_throttle_info = response.mutable_apn_throttle_info_list()->
                                     mutable_rep_apn_throttle_info(idx);
            throttleInfo.apn = apn_throttle_info->apn_name();
            for (auto pidx = 0; pidx < apn_throttle_info->profile_ids_size(); pidx++) {
                throttleInfo.profileIds.push_back(apn_throttle_info->profile_ids(pidx));
            }
            throttleInfo.ipv4Time  = apn_throttle_info->ipv4time();
            throttleInfo.ipv6Time  = apn_throttle_info->ipv6time();
            throttleInfo.isBlocked = apn_throttle_info->is_blocked();
            throttleInfo.mcc       = apn_throttle_info->mcc();
            throttleInfo.mnc       = apn_throttle_info->mnc();
            apnThrottleInfo.push_back(throttleInfo);
        }


        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
             [this, apnThrottleInfo, error, delay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(apnThrottleInfo, error);
               }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

void DataConnectionManagerStub::handleThrottledApnInfoChangedEvent
(::dataStub::APNThrottleInfoList throttleInfoList)
{
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<IDataConnectionListener>> applisteners;
    this->getAvailableListeners(applisteners);

    std::vector<APNThrottleInfo> apnThrottleInfo;

    LOG(DEBUG, __FUNCTION__, " throttled_apn_info_list size: ",
    throttleInfoList.rep_apn_throttle_info_size());
    for (auto idx = 0; idx < throttleInfoList.rep_apn_throttle_info_size(); idx++) {
        APNThrottleInfo throttleInfo;
        auto apn_throttle_info = throttleInfoList.mutable_rep_apn_throttle_info(idx);
        throttleInfo.apn = apn_throttle_info->apn_name();
        for (auto pidx = 0; pidx < apn_throttle_info->profile_ids_size(); pidx++) {
            throttleInfo.profileIds.push_back(apn_throttle_info->profile_ids(pidx));
        }
        throttleInfo.ipv4Time  = apn_throttle_info->ipv4time();
        throttleInfo.ipv6Time  = apn_throttle_info->ipv6time();
        throttleInfo.isBlocked = apn_throttle_info->is_blocked();
        throttleInfo.mcc       = apn_throttle_info->mcc();
        throttleInfo.mnc       = apn_throttle_info->mnc();
        apnThrottleInfo.push_back(throttleInfo);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_NOTIFICATION_DELAY));
    for (auto &listener : applisteners) {
        listener->onThrottledApnInfoChanged(apnThrottleInfo);
    }

    return;
}

int DataConnectionManagerStub::getSlotId() {
    LOG(DEBUG, __FUNCTION__);
    return slotId_;
}

void DataConnectionManagerStub::getAvailableListeners(
    std::vector<std::shared_ptr<IDataConnectionListener>> &listeners) {
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

void DataConnectionManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::vector<std::shared_ptr<IDataConnectionListener>> applisteners;
    this->getAvailableListeners(applisteners);
    for (auto &listener : applisteners) {
        listener->onServiceStatusChange(status);
    }
}

} // end of namespace data
} // end of namespace telux
