/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "ServingSystemManagerStub.hpp"
#include "TelDefinesStub.hpp"

using namespace telux::common;
using namespace telux::tel;

ServingSystemManagerStub::ServingSystemManagerStub(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    phoneId_ = phoneId;
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

void ServingSystemManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
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

telux::common::Status ServingSystemManagerStub::init(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IServingSystemListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<::telStub::ServingSystemService>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate serving system service");
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

void ServingSystemManagerStub::initSync() {
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

ServingSystemManagerStub::~ServingSystemManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void ServingSystemManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

void ServingSystemManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

bool ServingSystemManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

bool ServingSystemManagerStub::waitForInitialization() {
    std::unique_lock<std::mutex> cvLock(mtx_);
    while (!isSubsystemReady()) {
        cv_.wait(cvLock);
    }
    return isSubsystemReady();
}

std::future<bool> ServingSystemManagerStub::onSubsystemReady() {
    auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return f;
}

telux::common::ServiceStatus ServingSystemManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status ServingSystemManagerStub::registerListener(
    std::weak_ptr<IServingSystemListener> listener, ServingSystemNotificationMask mask) {
    LOG(DEBUG, __FUNCTION__, " mask - ", mask.to_string());
    telux::common::Status status = telux::common::Status::FAILED;
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
        return telux::common::Status::FAILED;
    }
    do {
        // Check if client is trying to register all optional and default indications. If so, reset
        // the invalid bits in bitset. Invalid bits are those that doesn't represent any
        // notification. This is done to avoid holding listener objects for those invalid bits.
        if(mask == ALL_NOTIFICATIONS) {
            mask.reset();
            // Keep setting the bits for any new notifications here
            mask.set(ServingSystemNotificationType::SYSTEM_INFO);
            mask.set(ServingSystemNotificationType::RF_BAND_INFO);
            mask.set(ServingSystemNotificationType::NETWORK_REJ_INFO);
            mask.set(ServingSystemNotificationType::LTE_SIB16_NETWORK_TIME);
            mask.set(ServingSystemNotificationType::NR5G_RRC_UTC_TIME);
        }
        // TODO: Update client mask for post SSR
        // Register for default notifications
        status = listenerMgr_->registerListener(listener);
        // If no optional indications are chosen, return with registration status of default
        // indications
        if(mask.none()) {
            break;
        }
        // Ignore status == telux::common::Status::ALREADY for default notifications, since app can
        // call this function multiple times for registering a different indication each time.
        if(status != telux::common::Status::SUCCESS && status != telux::common::Status::ALREADY) {
            LOG(ERROR, __FUNCTION__, " Failed to register for default notifications, error: ",
            static_cast<int>(status));
            break;
        }
        // Register for default indications
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        status = clientEventManager.registerListener(shared_from_this(),
                { TEL_SERVING_SYSTEM_SELECTION_PREF, TEL_SERVING_SYSTEM_NETWORK_TIME });
        if ((status != telux::common::Status::SUCCESS) &&
                (status != telux::common::Status::ALREADY)) {
            LOG(ERROR, __FUNCTION__, ":: Registering for default notifications failed");
            return status;
        }

        ServingSystemNotificationMask firstReg = {};
        // Register for chosen optional notifications
        status = listenerMgr_->registerListener(listener, mask, firstReg);
        if(status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to register for notification mask - ",
            mask.to_string(), ", error: ", static_cast<int>(status));
            break;
        }
        if(firstReg.test(ServingSystemNotificationType::SYSTEM_INFO)) {
            status = clientEventManager.registerListener(shared_from_this(),
                { telux::tel::TEL_SERVING_SYSTEM_INFO });
            if ((status != telux::common::Status::SUCCESS) &&
                    (status != telux::common::Status::ALREADY)) {
                LOG(ERROR, __FUNCTION__, ":: Registering system info change event failed");
                return status;
            }
        }
        if (firstReg.test(ServingSystemNotificationType::RF_BAND_INFO)) {
             status = clientEventManager.registerListener(shared_from_this(),
                { telux::tel::TEL_SERVING_SYSTEM_RF_BAND_INFO });
            if ((status != telux::common::Status::SUCCESS) &&
                    (status != telux::common::Status::ALREADY)) {
                LOG(ERROR, __FUNCTION__, ":: Registering rf band info event failed");
                return status;
            }
        }
        if (firstReg.test(ServingSystemNotificationType::NETWORK_REJ_INFO)) {
             status = clientEventManager.registerListener(shared_from_this(),
                { telux::tel::TEL_SERVING_SYSTEM_NETWORK_REJ_INFO });
            if ((status != telux::common::Status::SUCCESS) &&
                    (status != telux::common::Status::ALREADY)) {
                LOG(ERROR, __FUNCTION__, ":: Registering network reject info event failed");
                return status;
            }
        }
        /* In simulation, TEL_SERVING_SYSTEM_NETWORK_TIME is considered for
           ServingSystemNotificationType::LTE_SIB16_NETWORK_TIME or
           ServingSystemNotificationType::NR5G_RRC_UTC_TIME, hence registration is not required. */
    } while(0);
    return status;
}

telux::common::Status ServingSystemManagerStub::deregisterListener(
    std::weak_ptr<IServingSystemListener> listener, ServingSystemNotificationMask mask) {
    LOG(DEBUG, __FUNCTION__, " mask - ", mask.to_string());
    telux::common::Status status = telux::common::Status::FAILED;
    bool deregisteredMainListener = false;
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
        return telux::common::Status::FAILED;
    }
    do {
        // Empty mask is an invalid input for de-registration
        if(mask.none()) {
            LOG(ERROR, __FUNCTION__, " Empty mask");
            status = telux::common::Status::INVALIDPARAM;
            break;
        }
        // De-register all optional and default indications
        if(mask == ALL_NOTIFICATIONS) {
            status = listenerMgr_->deRegisterListener(listener);
            if(status != telux::common::Status::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " Failed to de-register for default notifications,error ",
                    static_cast<int>(status));
                break;
            }
            deregisteredMainListener = true;
            // Reset all invalid bits in bitmask. Invalid bits are those that doesn't represent any
            // notification. This is done because no invalid bits are set during registration.
            mask.reset();
            // Keep setting the bits for any new notifications here
            mask.set(ServingSystemNotificationType::SYSTEM_INFO);
            mask.set(ServingSystemNotificationType::RF_BAND_INFO);
            mask.set(ServingSystemNotificationType::NETWORK_REJ_INFO);
            mask.set(ServingSystemNotificationType::LTE_SIB16_NETWORK_TIME);
            mask.set(ServingSystemNotificationType::NR5G_RRC_UTC_TIME);
        }
        // TODO: Update client mask for SSR
        // De-register optional indications
        ServingSystemNotificationMask lastReg;
        status = listenerMgr_->deRegisterListener(listener, mask, lastReg);
        if(deregisteredMainListener && status == telux::common::Status::NOSUCH) {
            // If no optional indications were registered earlier, the app might have called this
            // function just to de-register the default indications. So considering it as SUCCESS
            return telux::common::Status::SUCCESS;
        }
        if(status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to de-register for notification mask - ",
                mask.to_string(), ", error: ", static_cast<int>(status));
            break;
        }

        // Deregister for default indications.
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
                status = clientEventManager.deregisterListener(shared_from_this(),
                    { telux::tel::TEL_SERVING_SYSTEM_SELECTION_PREF,
                      telux::tel::TEL_SERVING_SYSTEM_NETWORK_TIME });
        if ((status != telux::common::Status::SUCCESS) &&
                (status != telux::common::Status::ALREADY)) {
                LOG(ERROR, __FUNCTION__, " DeRegistering default events failed");
                return status;
        }

        if(lastReg.test(ServingSystemNotificationType::SYSTEM_INFO)) {
            status = clientEventManager.deregisterListener(shared_from_this(),
                        { telux::tel::TEL_SERVING_SYSTEM_INFO });
            if ((status != telux::common::Status::SUCCESS) &&
                (status != telux::common::Status::ALREADY)) {
                LOG(ERROR, __FUNCTION__, " DeRegistering system info event failed");
                return status;
            }
        }

        if(lastReg.test(ServingSystemNotificationType::RF_BAND_INFO)) {
            status = clientEventManager.deregisterListener(shared_from_this(),
                        { telux::tel::TEL_SERVING_SYSTEM_RF_BAND_INFO });
            if ((status != telux::common::Status::SUCCESS) &&
                (status != telux::common::Status::ALREADY)) {
                LOG(ERROR, __FUNCTION__, " DeRegistering rf band info event failed");
                return status;
            }
        }

        if(lastReg.test(ServingSystemNotificationType::NETWORK_REJ_INFO)) {
            status = clientEventManager.deregisterListener(shared_from_this(),
                        { telux::tel::TEL_SERVING_SYSTEM_NETWORK_REJ_INFO });
            if ((status != telux::common::Status::SUCCESS) &&
                (status != telux::common::Status::ALREADY)) {
                LOG(ERROR, __FUNCTION__, " DeRegistering network reject info event failed");
                return status;
            }
        }
        /* In simulation, TEL_SERVING_SYSTEM_NETWORK_TIME is considered for
           ServingSystemNotificationType::LTE_SIB16_NETWORK_TIME or
           ServingSystemNotificationType::NR5G_RRC_UTC_TIME, hence de-registration is not
           required. */
    } while(0);
    return status;
}

telux::tel::DcStatus ServingSystemManagerStub::getDcStatus() {
    LOG(DEBUG, __FUNCTION__);
    DcStatus dcStatus = {EndcAvailability::UNKNOWN, DcnrRestriction::UNKNOWN};
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return dcStatus;
    }
    ::telStub::GetDcStatusRequest request;
    ::telStub::GetDcStatusReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetDcStatus(&context, request, &response);
    if (reqstatus.ok()) {
        dcStatus.endcAvailability =
        static_cast<telux::tel::EndcAvailability>(response.endc_availability());
        dcStatus.dcnrRestriction =
            static_cast<telux::tel::DcnrRestriction>(response.dcnr_restriction());
        LOG(DEBUG, __FUNCTION__, " endcAvailability is ",
            static_cast<int>(dcStatus.endcAvailability) , " dcnrRestriction is ",
            static_cast<int>(dcStatus.dcnrRestriction));
    }
    return dcStatus;
}

telux::common::Status ServingSystemManagerStub::setRatPreference(RatPreference ratPref,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetRATPreferenceRequest request;
    ::telStub::SetRATPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    int size = ratPref.size();
    for (int j = 0; j < size ; j++)
    {
        if(ratPref.test(j)) {
            request.add_rat_pref_types(static_cast<telStub::RatPrefType>(j));
        }
    }
    grpc::Status reqstatus = stub_->SetRATPreference(&context, request, &response);
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
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status
    ServingSystemManagerStub::requestRatPreference(RatPreferenceCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestRATPreferenceRequest request;
    ::telStub::RequestRATPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRATPreference(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    RatPreference preference;
    for (auto &r : response.rat_pref_types()) {
        preference.set(static_cast<int>(r));
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, preference, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(preference, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::setServiceDomainPreference(
    ServiceDomainPreference serviceDomain, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetServiceDomainPreferenceRequest request;
    ::telStub::SetServiceDomainPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_service_domain_pref
        (static_cast<telStub::ServiceDomainPreference_Pref>(serviceDomain));
    grpc::Status reqstatus = stub_->SetServiceDomainPreference(&context, request, &response);
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
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::requestServiceDomainPreference(
    ServiceDomainPreferenceCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestServiceDomainPreferenceRequest request;
    ::telStub::RequestServiceDomainPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestServiceDomainPreference(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ServiceDomainPreference preference =
        (static_cast<telux::tel::ServiceDomainPreference>(response.service_domain_pref()));
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, preference, error, callback]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(preference, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::getSystemInfo(ServingSystemInfo &sysInfo) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::GetSystemInfoRequest request;
    ::telStub::GetSystemInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetSystemInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    sysInfo.domain = static_cast<telux::tel::ServiceDomain>(response.current_domain());
    sysInfo.rat = static_cast<telux::tel::RadioTechnology>(response.current_rat());
    sysInfo.state = static_cast<telux::tel::ServiceRegistrationState>(response.current_state());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

telux::common::Status ServingSystemManagerStub::requestNetworkTime(
    NetworkTimeResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestNetworkTimeRequest request;
    ::telStub::RequestNetworkTimeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestNetworkTime(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    NetworkTimeInfo info;
    info.year = ((response.network_time_info()).year());
    info.month = ((response.network_time_info()).month());
    info.day = ((response.network_time_info()).day());
    info.hour = ((response.network_time_info()).hour());
    info.minute = ((response.network_time_info()).minute());
    info.second = ((response.network_time_info()).second());
    info.dayOfWeek = ((response.network_time_info()).day_of_week());
    info.timeZone = ((response.network_time_info()).time_zone());
    info.dstAdj = ((response.network_time_info()).dst_adj());
    info.nitzTime = ((response.network_time_info()).nitz_time());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, cbDelay, info, error, callback]() {
                if (callback) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(info, error);
                }
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::requestLteSib16NetworkTime(
    NetworkTimeResponseCallback callback) {
    return requestNetworkTime(callback);
}

telux::common::Status ServingSystemManagerStub::requestNr5gRrcUtcTime(
    NetworkTimeResponseCallback callback) {
    return requestNetworkTime(callback);
}

telux::common::Status ServingSystemManagerStub::requestRFBandInfo(RFBandInfoCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestRFBandInfoRequest request;
    ::telStub::RequestRFBandInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRFBandInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    RFBandInfo info;
    info.band = static_cast<telux::tel::RFBand>(response.band());
    info.channel = response.channel();
    info.bandWidth = static_cast<telux::tel::RFBandWidth>(response.band_width());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, info, error, callback]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(info, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::getNetworkRejectInfo
    (NetworkRejectInfo &rejectInfo) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::GetNetworkRejectInfoRequest request;
    ::telStub::GetNetworkRejectInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetNetworkRejectInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    rejectInfo.rejectSrvInfo.domain =
        static_cast<telux::tel::ServiceDomain>(response.reject_domain());
    rejectInfo.rejectSrvInfo.rat = static_cast<telux::tel::RadioTechnology>(response.reject_rat());
    rejectInfo.rejectCause = response.reject_cause();
    rejectInfo.mcc = response.mcc();
    rejectInfo.mnc = response.mnc();
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

telux::common::Status ServingSystemManagerStub::getCallBarringInfo
    (std::vector<CallBarringInfo> &barringInfo) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::GetCallBarringInfoRequest request;
    ::telStub::GetCallBarringInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetCallBarringInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    for (int i = 0; i < response.barring_infos_size(); i++) {
        CallBarringInfo info;
        info.rat = static_cast<telux::tel::RadioTechnology>(
            response.barring_infos(i).rat());
        info.domain = static_cast<telux::tel::ServiceDomain>(
            response.barring_infos(i).domain());
        info.callType = static_cast<telux::tel::CallsAllowedInCell>(
            response.barring_infos(i).call_type());
        barringInfo.emplace_back(info);
    }
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

telux::common::Status ServingSystemManagerStub::getSmsCapabilityOverNetwork
    (SmsCapability &smsCapability) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::GetSmsCapabilityOverNetworkRequest request;
    ::telStub::GetSmsCapabilityOverNetworkReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetSmsCapabilityOverNetwork(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    smsCapability.domain =
        static_cast<telux::tel::SmsDomain>(response.domain());
    smsCapability.rat = static_cast<telux::tel::RadioTechnology>(response.rat());
    smsCapability.smsStatus = static_cast<telux::tel::NtnSmsStatus>(response.sms_status());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}


telux::common::Status ServingSystemManagerStub::getLteCsCapability
    (LteCsCapability &lteCapability) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::GetLteCsCapabilityRequest request;
    ::telStub::GetLteCsCapabilityReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetLteCsCapability(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    lteCapability =
        static_cast<telux::tel::LteCsCapability>(response.capability());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

telux::common::Status ServingSystemManagerStub::requestRFBandPreferences
    (RFBandPrefCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestRFBandPreferencesRequest request;
    ::telStub::RequestRFBandPreferencesReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRFBandPreferences(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    std::vector<GsmRFBand> gsmBands     = {};
    std::vector<WcdmaRFBand> wcdmaBands = {};
    std::vector<LteRFBand> lteBands     = {};
    std::vector<NrRFBand> saBands       = {};
    std::vector<NrRFBand> nsaBands      = {};
    for (auto &g : response.gsm_pref_bands()) {
        gsmBands.push_back(static_cast<GsmRFBand>(g));
    }
    for (auto &w : response.wcdma_pref_bands()) {
        wcdmaBands.push_back(static_cast<WcdmaRFBand>(w));
    }
    for (auto &l : response.lte_pref_bands()) {
        lteBands.push_back(static_cast<LteRFBand>(l));
    }
    for (auto &n : response.nsa_pref_bands()) {
        nsaBands.push_back(static_cast<NrRFBand>(n));
    }
    for (auto &s : response.sa_pref_bands()) {
        saBands.push_back(static_cast<NrRFBand>(s));
    }
    auto builder                           = std::make_shared<RFBandListBuilder>();
    telux::common::ErrorCode errCode       = telux::common::ErrorCode::UNKNOWN;
    std::shared_ptr<IRFBandList> prefBands = builder->addGsmRFBands(gsmBands)
                                                 .addWcdmaRFBands(wcdmaBands)
                                                 .addLteRFBands(lteBands)
                                                 .addNrRFBands(NrType::SA, saBands)
                                                 .addNrRFBands(NrType::NSA, nsaBands)
                                                 .build(errCode);
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, prefBands, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(prefBands, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::setRFBandPreferences
    (std::shared_ptr<IRFBandList> prefList, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetRFBandPreferencesRequest request;
    ::telStub::SetRFBandPreferencesReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    for(auto &g : prefList->getGsmBands()) {
         request.add_gsm_pref_bands(static_cast<telStub::GsmRFBand>(g));
    }
    for(auto &w : prefList->getWcdmaBands()) {
         request.add_wcdma_pref_bands(static_cast<telStub::WcdmaRFBand>(w));
    }
    for(auto &l : prefList->getLteBands()) {
         request.add_lte_pref_bands(static_cast<telStub::LteRFBand>(l));
    }
    for(auto &n : prefList->getNrBands(NrType::NSA)) {
         request.add_nsa_pref_bands(static_cast<telStub::NrRFBand>(n));
    }
    for(auto &s: prefList->getNrBands(NrType::SA)) {
         request.add_sa_pref_bands(static_cast<telStub::NrRFBand>(s));
    }
    grpc::Status reqstatus = stub_->SetRFBandPreferences(&context, request, &response);
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
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::requestRFBandCapability
    (RFBandCapabilityCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Service Status is UNAVAILABLE");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestRFBandCapabilityRequest request;
    ::telStub::RequestRFBandCapabilityReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRFBandCapability(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    std::vector<GsmRFBand> gsmBands     = {};
    std::vector<WcdmaRFBand> wcdmaBands = {};
    std::vector<LteRFBand> lteBands     = {};
    std::vector<NrRFBand> nrBands       = {};
    for (auto &g : response.gsm_capability_bands()) {
        gsmBands.push_back(static_cast<GsmRFBand>(g));
    }
    for (auto &w : response.wcdma_capability_bands()) {
        wcdmaBands.push_back(static_cast<WcdmaRFBand>(w));
    }
    for (auto &l : response.lte_capability_bands()) {
        lteBands.push_back(static_cast<LteRFBand>(l));
    }
    for (auto &n : response.nr_capability_bands()) {
        nrBands.push_back(static_cast<NrRFBand>(n));
    }
    auto builder                           = std::make_shared<RFBandListBuilder>();
    telux::common::ErrorCode errCode       = telux::common::ErrorCode::UNKNOWN;
    std::shared_ptr<IRFBandList> capabilityBands = builder->addGsmRFBands(gsmBands)
                                                 .addWcdmaRFBands(wcdmaBands)
                                                 .addLteRFBands(lteBands)
                                                 .addNrRFBands(NrType::COMBINED, nrBands)
                                                 .build(errCode);
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, capabilityBands, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(capabilityBands, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

void ServingSystemManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::SystemSelectionPreferenceEvent>()) {
        ::telStub::SystemSelectionPreferenceEvent systemSelectionPreferenceEvent;
        event.UnpackTo(&systemSelectionPreferenceEvent);
        handleSystemSelectionPreferenceChanged(systemSelectionPreferenceEvent);
    } else if (event.Is<::telStub::SystemInfoEvent>()) {
        ::telStub::SystemInfoEvent systemInfoEvent;
        event.UnpackTo(&systemInfoEvent);
        handleSystemInfoChanged(systemInfoEvent);
    } else if (event.Is<::telStub::NetworkTimeInfoEvent>()) {
        ::telStub::NetworkTimeInfoEvent networkTimeInfoEvent;
        event.UnpackTo(&networkTimeInfoEvent);
        handleNetworkTimeChange(networkTimeInfoEvent);
    } else if (event.Is<::telStub::NetworkRejectInfoEvent>()) {
        ::telStub::NetworkRejectInfoEvent networkRejectInfoEvent;
        event.UnpackTo(&networkRejectInfoEvent);
        handleNetworkRejection(networkRejectInfoEvent);
    } else if (event.Is<::telStub::RFBandInfoEvent>()) {
        ::telStub::RFBandInfoEvent rFBandInfoEvent;
        event.UnpackTo(&rFBandInfoEvent);
        handleRfBandInfoUpdateEvent(rFBandInfoEvent);
    }
}

void ServingSystemManagerStub::handleRfBandInfoUpdateEvent(::telStub::RFBandInfoEvent event) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    RFBandInfo info;
    info.band = static_cast<RFBand>(event.band());
    info.channel = event.channel();
    info.bandWidth = static_cast<RFBandWidth>(event.band_width());

    std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(
        ServingSystemNotificationType::RF_BAND_INFO, applisteners);
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onRFBandInfoChanged(info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ServingSystemManagerStub::handleSystemInfoChanged(::telStub::SystemInfoEvent event) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    DcStatus dcStatus;
    dcStatus.endcAvailability = static_cast<EndcAvailability>(event.endc_availability());
    dcStatus.dcnrRestriction = static_cast<DcnrRestriction>(event.dcnr_restriction());

    ServingSystemInfo info;
    info.rat = static_cast<RadioTechnology>(event.current_rat());
    info.domain = static_cast<ServiceDomain>(event.current_domain());
    info.state = static_cast<ServiceRegistrationState>(event.current_state());

    SmsCapability smsCapability;
    smsCapability.rat = static_cast<RadioTechnology>(event.sms_rat());
    smsCapability.domain = static_cast<SmsDomain>(event.sms_domain());
    smsCapability.smsStatus = static_cast<NtnSmsStatus>(event.sms_status());

    LteCsCapability lteCapability = static_cast<LteCsCapability>(event.lte_capability());;

    std::vector<CallBarringInfo> infos = {};
    for (int i = 0; i < event.barring_infos_size(); i++) {
        CallBarringInfo info;
        info.rat = static_cast<telux::tel::RadioTechnology>(
            event.barring_infos(i).rat());
        info.domain = static_cast<telux::tel::ServiceDomain>(
            event.barring_infos(i).domain());
        info.callType = static_cast<telux::tel::CallsAllowedInCell>(
            event.barring_infos(i).call_type());
        infos.emplace_back(info);
    }

    std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(
        ServingSystemNotificationType::SYSTEM_INFO, applisteners);
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onDcStatusChanged(dcStatus);
            }
        }
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onSystemInfoChanged(info);
            }
        }
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onSmsCapabilityChanged(smsCapability);
            }
        }
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onLteCsCapabilityChanged(lteCapability);
            }
        }
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onCallBarringInfoChanged(infos);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ServingSystemManagerStub::handleSystemSelectionPreferenceChanged
    (::telStub::SystemSelectionPreferenceEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    RatPreference preference;
    for (auto &r : event.rat_pref_types()) {
        preference.set(static_cast<int>(r));
    }

    ServiceDomainPreference domain =
        static_cast<telux::tel::ServiceDomainPreference>(event.service_domain_pref());

    std::vector<GsmRFBand> gsmBands     = {};
    std::vector<WcdmaRFBand> wcdmaBands = {};
    std::vector<LteRFBand> lteBands     = {};
    std::vector<NrRFBand> saBands       = {};
    std::vector<NrRFBand> nsaBands      = {};
    for (auto &g : event.gsm_pref_bands()) {
        gsmBands.push_back(static_cast<GsmRFBand>(g));
    }
    for (auto &w : event.wcdma_pref_bands()) {
        wcdmaBands.push_back(static_cast<WcdmaRFBand>(w));
    }
    for (auto &l : event.lte_pref_bands()) {
        lteBands.push_back(static_cast<LteRFBand>(l));
    }
    for (auto &n : event.nsa_pref_bands()) {
        nsaBands.push_back(static_cast<NrRFBand>(n));
    }
    for (auto &s : event.sa_pref_bands()) {
        saBands.push_back(static_cast<NrRFBand>(s));
    }
    auto builder                           = std::make_shared<RFBandListBuilder>();
    telux::common::ErrorCode errCode       = telux::common::ErrorCode::UNKNOWN;
    std::shared_ptr<IRFBandList> prefBands = builder->addGsmRFBands(gsmBands)
                                                 .addWcdmaRFBands(wcdmaBands)
                                                 .addLteRFBands(lteBands)
                                                 .addNrRFBands(NrType::SA, saBands)
                                                 .addNrRFBands(NrType::NSA, nsaBands)
                                                 .build(errCode);

    LOG(DEBUG, __FUNCTION__, " ServiceDomainPreference is  ", static_cast<int>(domain));

    std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onRatPreferenceChanged(preference);
            }
        }

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onServiceDomainPreferenceChanged(domain);
            }
        }

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onRFBandPreferenceChanged(prefBands);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }

}

void ServingSystemManagerStub::handleNetworkTimeChange(::telStub::NetworkTimeInfoEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        NetworkTimeInfo info;
        info.year = event.year();
        info.month = event.month();
        info.day = event.day();
        info.hour = event.hour();
        info.minute = event.minute();
        info.second = event.second();
        info.dayOfWeek = event.day_of_week();
        info.timeZone = event.time_zone();
        info.dstAdj = event.dst_adj();
        info.nitzTime = event.nitz_time();

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onNetworkTimeChanged(info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ServingSystemManagerStub::handleNetworkRejection(::telStub::NetworkRejectInfoEvent event) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = event.phone_id();
    if( phoneId_ != phoneId ) {
        LOG(DEBUG, __FUNCTION__, " Ignoring events for subcription ", phoneId);
        return;
    }
    std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
    if (listenerMgr_) {
        NetworkRejectInfo rejectInfo;
        listenerMgr_->getAvailableListeners(
            ServingSystemNotificationType::NETWORK_REJ_INFO, applisteners);

        rejectInfo.rejectSrvInfo.rat = static_cast<telux::tel::RadioTechnology>(event.reject_rat());
        rejectInfo.rejectSrvInfo.domain =
            static_cast<telux::tel::ServiceDomain>(event.reject_domain());
        rejectInfo.rejectCause = event.reject_cause();
        rejectInfo.mcc = event.mcc();
        rejectInfo.mnc = event.mnc();

        LOG(DEBUG, __FUNCTION__, " MCC is ", rejectInfo.mcc, ", MNC is ", rejectInfo.mnc);

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onNetworkRejection(rejectInfo);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

RFBandList::RFBandList() {
}

RFBandList::~RFBandList() {
}

std::vector<GsmRFBand> RFBandList::getGsmBands() {
    std::lock_guard<std::mutex> lock(mtx_);
    return gsmBands_;
}

std::vector<WcdmaRFBand> RFBandList::getWcdmaBands() {
    std::lock_guard<std::mutex> lock(mtx_);
    return wcdmaBands_;
}

std::vector<LteRFBand> RFBandList::getLteBands() {
    std::lock_guard<std::mutex> lock(mtx_);
    return lteBands_;
}

std::vector<NrRFBand> RFBandList::getNrBands(NrType type) {
    std::vector<NrRFBand> nrBands;
    std::lock_guard<std::mutex> lock(mtx_);
    switch (type) {
        case NrType::NSA:
            nrBands.assign(nsaBands_.begin(), nsaBands_.end());
            break;
        case NrType::SA:
            nrBands.assign(saBands_.begin(), saBands_.end());
            break;
        case NrType::COMBINED:
            nrBands.assign(nrBands_.begin(), nrBands_.end());
            break;
    }
    return nrBands;
}

void RFBandList::setGsmBands(std::vector<GsmRFBand> bands) {
    std::lock_guard<std::mutex> lock(mtx_);
    gsmBands_.assign(bands.begin(), bands.end());
}

void RFBandList::setWcdmaBands(std::vector<WcdmaRFBand> bands) {
    std::lock_guard<std::mutex> lock(mtx_);
    wcdmaBands_.assign(bands.begin(), bands.end());
}

void RFBandList::setLteBands(std::vector<LteRFBand> bands) {
    std::lock_guard<std::mutex> lock(mtx_);
    lteBands_.assign(bands.begin(), bands.end());
}

void RFBandList::setNrBands(NrType type, std::vector<NrRFBand> bands) {
    std::lock_guard<std::mutex> lock(mtx_);
    switch (type) {
        case NrType::NSA:
            nsaBands_.assign(bands.begin(), bands.end());
            break;
        case NrType::SA:
            saBands_.assign(bands.begin(), bands.end());
            break;
        case NrType::COMBINED:
            nrBands_.assign(bands.begin(), bands.end());
            break;
    }
}

bool RFBandList::isGSMBandPresent(GsmRFBand band) {
    bool result = false;
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto gsmBand : gsmBands_) {
        if (gsmBand == band) {
            result = true;
        }
    }
    return result;
}

bool RFBandList::isWcdmaBandPresent(WcdmaRFBand band) {
    bool result = false;
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto wcdmaBand : wcdmaBands_) {
        if (wcdmaBand == band) {
            result = true;
        }
    }
    return result;
}

bool RFBandList::isLteBandPresent(LteRFBand band) {
    bool result = false;
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto lteBand : lteBands_) {
        if (lteBand == band) {
            result = true;
        }
    }
    return result;
}

bool RFBandList::isNrBandPresent(NrType type, NrRFBand band) {
    bool result = false;
    std::vector<NrRFBand> bands;
    std::lock_guard<std::mutex> lock(mtx_);
    switch (type) {
        case NrType::NSA:
            bands.assign(nsaBands_.begin(), nsaBands_.end());
            break;
        case NrType::SA:
            bands.assign(saBands_.begin(), saBands_.end());
            break;
        case NrType::COMBINED:
            bands.assign(nrBands_.begin(), nrBands_.end());
            break;
    }
    for (auto nrBand : bands) {
        if (nrBand == band) {
            result = true;
        }
    }
    return result;
}

RFBandListBuilder::RFBandListBuilder() {
    rfBandList_ = std::make_shared<RFBandList>();
}

RFBandListBuilder &RFBandListBuilder::addGsmRFBands(std::vector<GsmRFBand> bands) {
    if (rfBandList_) {
        rfBandList_->setGsmBands(bands);
    }
    return *this;
}

RFBandListBuilder &RFBandListBuilder::addWcdmaRFBands(std::vector<WcdmaRFBand> bands) {
    if (rfBandList_) {
        rfBandList_->setWcdmaBands(bands);
    }
    return *this;
}

RFBandListBuilder &RFBandListBuilder::addLteRFBands(std::vector<LteRFBand> bands) {
    if (rfBandList_) {
        rfBandList_->setLteBands(bands);
    }
    return *this;
}

RFBandListBuilder &RFBandListBuilder::addNrRFBands(NrType type, std::vector<NrRFBand> bands) {
    if (rfBandList_) {
        rfBandList_->setNrBands(type, bands);
    }
    return *this;
}

std::shared_ptr<IRFBandList> RFBandListBuilder::build(telux::common::ErrorCode &errCode) {
    if (rfBandList_) {
        errCode = telux::common::ErrorCode::SUCCESS;
        return rfBandList_;
    } else {
        errCode = telux::common::ErrorCode::MISSING_ARGUMENTS;
        return nullptr;
    }
}

