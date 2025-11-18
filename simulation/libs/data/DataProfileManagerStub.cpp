/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataProfileManagerStub.hpp"
#include <grpcpp/grpcpp.h>
#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>
#include <thread>
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {

DataProfileManagerStub::DataProfileManagerStub(SlotId slotId,
    telux::common::InitResponseCb callback) {

    LOG(DEBUG, __FUNCTION__, "initializing DataProfileManagerStub for slotId:",
        slotId);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();

    auto f = std::async(std::launch::async,
             [this, callback]() {
                this->initSync(callback);
            }).share();
    taskQ_->add(f);

    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    slotId_ = slotId;
}

DataProfileManagerStub::~DataProfileManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    cleanup();
}

telux::common::Status DataProfileManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
    setSubSystemStatus(telux::common::ServiceStatus::SERVICE_FAILED);
    setSubsystemReady(false);

    return telux::common::Status::SUCCESS;
}

std::future<bool> DataProfileManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(
        std::launch::async, [&] { return DataProfileManagerStub::waitForInitialization(); });
    return future;
}

bool DataProfileManagerStub::waitForInitialization() {
    LOG(INFO, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    if (!isSubsystemReady()) {
        cv_.wait(lock);
    }
    return isSubsystemReady();
}

telux::common::ServiceStatus DataProfileManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

bool DataProfileManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

void DataProfileManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::DataProfileManager>();

    ::dataStub::SlotInfo request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_slot_id(slotId_);
    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay = DEFAULT_DELAY;

    if (reqStatus.ok()) {
        cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, __FUNCTION__, " InitService request failed");
    }

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
}

void DataProfileManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

void DataProfileManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

int DataProfileManagerStub::getSlotId() {
    LOG(DEBUG, __FUNCTION__);;
    return slotId_;
}

telux::common::Status DataProfileManagerStub::registerListener(
    std::weak_ptr<IDataProfileListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data profile subsystem not ready");
        return telux::common::Status::NOTREADY;
    }
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

telux::common::Status DataProfileManagerStub::deregisterListener(
    std::weak_ptr<telux::data::IDataProfileListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Data profile subsystem not ready");
        return telux::common::Status::NOTREADY;
    }
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

telux::common::Status DataProfileManagerStub::createProfile(
    const ProfileParams &profileParams,
    std::shared_ptr<IDataCreateProfileCallback> callback) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::CreateProfileReply response;
    ::dataStub::CreateProfileRequest request;
    ClientContext context;

    request.set_profile_name(profileParams.profileName);
    request.set_apn_name(profileParams.apn);
    request.set_user_name(profileParams.userName);
    request.set_password(profileParams.password);
    request.set_slot_id(slotId_);
    request.set_apn_types(convertApnTypeEnumToString(profileParams.apnTypes));
    request.mutable_tech_preference()->
        set_tech_preference((::dataStub::TechPreference::TechPref)profileParams.techPref);
    request.mutable_auth_type()->
        set_auth_type((::dataStub::AuthProtocolType::AuthProto)profileParams.authType);
    request.mutable_ip_family_type()->
        set_ip_family_type((::dataStub::IpFamilyType::Type)profileParams.ipFamilyType);
    request.set_emergency_capability(
        (::dataStub::EmergencyCapability)profileParams.emergencyAllowed);

    grpc::Status reqStatus = stub_->CreateProfile(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " CreateProfile request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        if (callback && (delay != SKIP_CALLBACK)) {
            uint8_t profileId = response.profile_id();
            LOG(DEBUG, __FUNCTION__, " created profile profileId:",
                profileId);
            auto f = std::async(std::launch::async,
                [this, callback, profileId, error]() {
                    callback->onResponse(profileId, error);
                }).share();
            taskQ_->add(f);
        }

        if (error == telux::common::ErrorCode::SUCCESS) {
            this->onProfileUpdate(response.profile_id(), profileParams.techPref,
            ProfileChangeEvent::CREATE_PROFILE_EVENT);
        }
    }

    return status;
}

telux::common::Status DataProfileManagerStub::deleteProfile(uint8_t profileId,
    TechPreference techPreference, std::shared_ptr<ICommandResponseCallback> callback) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::DefaultReply response;
    ::dataStub::DeleteProfileRequest request;
    ClientContext context;

    request.mutable_profile()->set_profile_id(profileId);
    request.mutable_profile()->set_slot_id(slotId_);
    request.mutable_profile()->mutable_tech_preference()->
        set_tech_preference((::dataStub::TechPreference::TechPref)techPreference);

    grpc::Status reqStatus = stub_->DeleteProfile(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " DeleteProfile request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        if (callback && (delay != SKIP_CALLBACK)) {
            LOG(DEBUG, __FUNCTION__, " deleted profile profileId:",
                profileId);
            auto f = std::async(std::launch::async,
                [this, callback, error]() {
                    callback->commandResponse(error);
                }).share();
            taskQ_->add(f);
        }

        if (error == telux::common::ErrorCode::SUCCESS) {
            this->onProfileUpdate(profileId, techPreference,
                ProfileChangeEvent::DELETE_PROFILE_EVENT);
        }
    }

    return status;
}

telux::common::Status DataProfileManagerStub::modifyProfile(uint8_t profileId,
    const ProfileParams &profileParams,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::DefaultReply response;
    ::dataStub::ModifyProfileRequest request;
    ClientContext context;

    request.set_profile_id(profileId);
    request.set_profile_name(profileParams.profileName);
    request.set_apn_name(profileParams.apn);
    request.set_user_name(profileParams.userName);
    request.set_password(profileParams.password);
    request.set_slot_id(slotId_);
    request.set_apn_types(convertApnTypeEnumToString(profileParams.apnTypes));
    request.mutable_tech_preference()->
        set_tech_preference((::dataStub::TechPreference::TechPref)profileParams.techPref);
    request.mutable_auth_type()->
        set_auth_type((::dataStub::AuthProtocolType::AuthProto)profileParams.authType);
    request.mutable_ip_family_type()->
        set_ip_family_type((::dataStub::IpFamilyType::Type)profileParams.ipFamilyType);
    request.set_emergency_capability(
        (::dataStub::EmergencyCapability)profileParams.emergencyAllowed);

    grpc::Status reqStatus = stub_->ModifyProfile(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " ModifyProfile request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        if (callback && (delay != SKIP_CALLBACK)) {
            LOG(DEBUG, __FUNCTION__, " modified profile profileId:",
                profileId);
            auto f = std::async(std::launch::async,
                [this, callback, error]() {
                    callback->commandResponse(error);
                }).share();
            taskQ_->add(f);
        }

        if (error == telux::common::ErrorCode::SUCCESS) {
            this->onProfileUpdate(profileId, profileParams.techPref,
                ProfileChangeEvent::MODIFY_PROFILE_EVENT);
        }
    }

    return status;
}

telux::common::Status DataProfileManagerStub::requestProfile(uint8_t profileId,
        TechPreference techPreference, std::shared_ptr<IDataProfileCallback> callback) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RequestProfileByIdReply response;
    ::dataStub::RequestProfileByIdRequest request;
    ClientContext context;

    request.mutable_profile()->set_profile_id(profileId);
    request.mutable_profile()->set_slot_id(slotId_);
    request.mutable_profile()->mutable_tech_preference()->
        set_tech_preference((::dataStub::TechPreference::TechPref)techPreference);

    grpc::Status reqStatus = stub_->RequestProfileById(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " RequestProfileById request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        std::shared_ptr<DataProfile> queryProfile;
        uint8_t profileId = response.profile().profile_id();
        std::string name = response.profile().profile_name();
        std::string apn = response.profile().apn_name();
        std::string username = response.profile().user_name();
        std::string password = response.profile().password();
        ApnTypes apnTypes =
            convertApnTypeStringToEnum(
            response.profile().apn_types());
        IpFamilyType ipFamily =
            static_cast<telux::data::IpFamilyType>(
            response.profile().ip_family_type().ip_family_type());
        TechPreference techPref =
            static_cast<telux::data::TechPreference>(
            response.profile().tech_preference().tech_preference());
        AuthProtocolType authType =
            static_cast<telux::data::AuthProtocolType>(
            response.profile().auth_type().auth_type());
        EmergencyCapability emergencyAllowed =
            static_cast<telux::data::EmergencyCapability>(
            response.profile().emergency_capability());

        queryProfile = std::make_shared<DataProfile>(profileId, name, apn, username,
            password, ipFamily, techPref, authType, apnTypes, emergencyAllowed);

        LOG(DEBUG, __FUNCTION__, " requestProfile successful profileId:",
                profileId);
        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
             [this, error, queryProfile, callback]() {
                   callback->onResponse(queryProfile, error);
               }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataProfileManagerStub::requestProfileList(
        std::shared_ptr<IDataProfileListCallback> callback) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RequestProfileListReply response;
    ::dataStub::RequestProfileListRequest request;
    ClientContext context;

    request.set_slot_id(slotId_);

    grpc::Status reqStatus = stub_->RequestProfileList(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " RequestProfileList request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        std::vector<std::shared_ptr<DataProfile>> requestedProfiles;
        for (int idx = 0; idx < response.profiles_size(); idx++) {
            uint8_t profileId = response.mutable_profiles(idx)->profile_id();
            std::string name = response.mutable_profiles(idx)->profile_name();
            std::string apn = response.mutable_profiles(idx)->apn_name();
            std::string username = response.mutable_profiles(idx)->user_name();
            std::string password = response.mutable_profiles(idx)->password();
            ApnTypes apnTypes =
                convertApnTypeStringToEnum(
                response.mutable_profiles(idx)->apn_types());
            IpFamilyType ipFamily =
                static_cast<telux::data::IpFamilyType>(
                response.mutable_profiles(idx)->ip_family_type().ip_family_type());
            TechPreference techPref =
                static_cast<telux::data::TechPreference>(
                response.mutable_profiles(idx)->tech_preference().tech_preference());
            AuthProtocolType authType =
                static_cast<telux::data::AuthProtocolType>(
                response.mutable_profiles(idx)->auth_type().auth_type());
            EmergencyCapability emergencyAllowed =
                static_cast<telux::data::EmergencyCapability>(
                response.mutable_profiles(idx)->emergency_capability());

            auto queryProfile = std::make_shared<DataProfile>(profileId, name, apn,
                username, password, ipFamily, techPref, authType, apnTypes,
                emergencyAllowed);
            LOG(DEBUG, __FUNCTION__, " requestProfileList successful profileId:",
                profileId);
            requestedProfiles.push_back(queryProfile);
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
            [this, callback, requestedProfiles, error]() {
                   callback->onProfileListResponse(requestedProfiles, error);
            }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataProfileManagerStub::queryProfile(const ProfileParams &profileParams,
    std::shared_ptr<IDataProfileListCallback> callback) {

    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::QueryProfileReply response;
    ::dataStub::QueryProfileRequest request;
    ClientContext context;

    request.set_profile_name(profileParams.profileName);
    request.set_apn_name(profileParams.apn);
    request.set_user_name(profileParams.userName);
    request.set_password(profileParams.password);
    request.set_slot_id(slotId_);
    request.set_apn_types(convertApnTypeEnumToString(profileParams.apnTypes));
    request.mutable_tech_preference()->
        set_tech_preference((::dataStub::TechPreference::TechPref)profileParams.techPref);
    request.mutable_auth_type()->
        set_auth_type((::dataStub::AuthProtocolType::AuthProto)profileParams.authType);
    request.mutable_ip_family_type()->
        set_ip_family_type((::dataStub::IpFamilyType::Type)profileParams.ipFamilyType);
    request.set_emergency_capability(
        (::dataStub::EmergencyCapability)profileParams.emergencyAllowed);

    grpc::Status reqStatus = stub_->QueryProfile(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " QueryProfile request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        std::vector<std::shared_ptr<DataProfile>> queriedProfiles;
        for (int idx = 0; idx < response.profiles_size(); idx++) {
            uint8_t profileId = response.mutable_profiles(idx)->profile_id();
            std::string name = response.mutable_profiles(idx)->profile_name();
            std::string apn = response.mutable_profiles(idx)->apn_name();
            std::string username = response.mutable_profiles(idx)->user_name();
            std::string password = response.mutable_profiles(idx)->password();
            ApnTypes apnTypes =
                convertApnTypeStringToEnum(
                response.mutable_profiles(idx)->apn_types());
            IpFamilyType ipFamily =
                static_cast<telux::data::IpFamilyType>(
                response.mutable_profiles(idx)->ip_family_type().ip_family_type());
            TechPreference techPref =
                static_cast<telux::data::TechPreference>(
                response.mutable_profiles(idx)->tech_preference().tech_preference());
            AuthProtocolType authType =
                static_cast<telux::data::AuthProtocolType>(
                response.mutable_profiles(idx)->auth_type().auth_type());
            EmergencyCapability emergencyAllowed =
                static_cast<telux::data::EmergencyCapability>(
                response.mutable_profiles(idx)->emergency_capability());

            auto queryProfile = std::make_shared<DataProfile>(profileId, name, apn,
                username, password, ipFamily, techPref, authType, apnTypes,
                emergencyAllowed);
            queriedProfiles.push_back(queryProfile);
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
            [this, callback, queriedProfiles, error]() {
                   callback->onProfileListResponse(queriedProfiles, error);
            }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

void DataProfileManagerStub::getAvailableListeners(
   std::vector<std::shared_ptr<telux::data::IDataProfileListener>> &listeners) {
   // Entering critical section, copy lockable shared_ptr from global listener
   std::lock_guard<std::mutex> lock(mutex_);
   LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners_.size());
   for(auto it = listeners_.begin(); it != listeners_.end();) {
      auto sp = (*it).lock();
      if(sp) {
         listeners.emplace_back(sp);
         ++it;
      } else {
         // if we unable to lock the listener, we should remove it from
         // listenerList
         LOG(DEBUG, "erased obselete weak pointer from data profile manager's listener");
         it = listeners_.erase(it);
      }
   }
}

void DataProfileManagerStub::onProfileUpdate(int profileId,
    TechPreference techPreference, ProfileChangeEvent event) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::shared_ptr<IDataProfileListener>> applisteners;
    this->getAvailableListeners(applisteners);
    for (auto &listener : applisteners) {
        listener->onProfileUpdate(profileId, techPreference, event);
    }
}

std::string DataProfileManagerStub::convertApnTypeEnumToString(
    telux::data::ApnTypes apnType) {
    LOG(DEBUG, __FUNCTION__);

    std::string apn = apnType.to_string();
    return apn;
}

telux::data::ApnTypes DataProfileManagerStub::convertApnTypeStringToEnum(
    std::string apn) {
    LOG(DEBUG, __FUNCTION__);
    telux::data::ApnTypes apnType(apn);
    return apnType;
}

} //end of namespace telux
} //end of namespace data
