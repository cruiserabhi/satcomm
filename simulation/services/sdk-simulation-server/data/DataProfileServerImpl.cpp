/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <set>
#include <functional>

#include "DataProfileServerImpl.hpp"
#include "libs/data/DataUtilsStub.hpp"

#define DATA_PROFILE_API_SLOT1_JSON "api/data/IDataProfileManagerSlot1.json"
#define DATA_PROFILE_API_SLOT2_JSON "api/data/IDataProfileManagerSlot2.json"
#define DATA_PROFILE_STATE_SLOT1_JSON "system-state/data/IDataProfileManagerStateSlot1.json"
#define DATA_PROFILE_STATE_SLOT2_JSON "system-state/data/IDataProfileManagerStateSlot2.json"
#define SLOT_2 2

DataProfileServerImpl::DataProfileServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

DataProfileServerImpl::~DataProfileServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status DataProfileServerImpl::InitService(ServerContext* context,
    const dataStub::SlotInfo* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string filePath = (request->slot_id() == SLOT_2)? DATA_PROFILE_API_SLOT2_JSON
        : DATA_PROFILE_API_SLOT1_JSON;
    Json::Value rootObj;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataProfileManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataProfileManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataProfileServerImpl::CreateProfile(ServerContext* context,
    const dataStub::CreateProfileRequest* request, dataStub::CreateProfileReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_API_SLOT2_JSON
        : DATA_PROFILE_API_SLOT1_JSON;

    std::string stateJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_STATE_SLOT2_JSON
        : DATA_PROFILE_STATE_SLOT1_JSON;
    std::string subsystem = "IDataProfileManager";
    std::string method = "createProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        Json::Value newProfile;
        int currentProfileCount =
            data.stateRootObj[subsystem]["requestProfileList"]["profiles"].size();

        int index = 0;
        int profileId = 0;
        std::set<int, std::greater<int>> profileList;
        for (; index < currentProfileCount; index++) {
            profileId = data.stateRootObj[subsystem]
                ["requestProfileList"]["profiles"][index]["profileId"].asInt();
            profileList.insert(profileId);
        }

        int maxProfile = *(profileList.begin());
        int minProfile = 1;
        for (index = minProfile; index <= maxProfile; index++) {
            if (profileList.find(index) != profileList.end()) {
                continue;
            }
            break;
        }
        if(index == maxProfile)
            profileId = maxProfile + 1;
        else
            profileId = index;

        newProfile["profileName"] = request->profile_name();
        newProfile["apn"] = request->apn_name();
        newProfile["username"] = request->user_name();
        newProfile["password"] = request->password();
        newProfile["apnTypes"] = request->apn_types();
        newProfile["ipFamilyType"] =
            DataUtilsStub::convertIpFamilyEnumToString(
            request->ip_family_type().ip_family_type());
        newProfile["profileId"] = profileId;
        newProfile["techPref"] =
            DataUtilsStub::convertTechPrefEnumToString(
            request->tech_preference().tech_preference());
        newProfile["authProtocolType"] =
            DataUtilsStub::convertAuthProtocolEnumToString(
            request->auth_type().auth_type());
        dataStub::EmergencyCapability emergencyAllowed =
            request->emergency_capability();
        if (emergencyAllowed ==
            dataStub::EmergencyCapability::UNSPECIFIED) {
            emergencyAllowed = dataStub::EmergencyCapability::NOT_ALLOWED;
        }
        newProfile["emergencyAllowed"] =
            ((int)emergencyAllowed);
        data.stateRootObj[subsystem]["requestProfileList"]
            ["profiles"][currentProfileCount] = newProfile;

        LOG(DEBUG, __FUNCTION__, " profileId::", profileId);
        // Updating the new profile list in JSON File.
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        response->set_profile_id(profileId);
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataProfileServerImpl::DeleteProfile(ServerContext* context,
    const dataStub::DeleteProfileRequest* request, dataStub::DefaultReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->profile().slot_id() == SLOT_2)?
        DATA_PROFILE_API_SLOT2_JSON : DATA_PROFILE_API_SLOT1_JSON;
    std::string stateJsonPath = (request->profile().slot_id() == SLOT_2)?
        DATA_PROFILE_STATE_SLOT2_JSON : DATA_PROFILE_STATE_SLOT1_JSON;
    std::string subsystem = "IDataProfileManager";
    std::string method = "deleteProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        Json::Value newRoot;
        int profileId = request->profile().profile_id();
        std::string techPref = DataUtilsStub::convertTechPrefEnumToString(
            request->profile().tech_preference().tech_preference());
        int currentProfileCount = data.stateRootObj[subsystem]
            ["requestProfileList"]["profiles"].size();
        int newCount = 0;
        int index = 0;
        bool profileFound = false;
        for (; index < currentProfileCount; index++) {
            int itrProfileId = data.stateRootObj[subsystem]
                ["requestProfileList"]["profiles"][index]["profileId"].asInt();
            if (itrProfileId == profileId ) {
                std::string itrTechPref = data.stateRootObj[subsystem]
                    ["requestProfileList"]["profiles"][index]["techPref"].asString();
                if (itrTechPref == techPref) {
                    LOG(DEBUG, __FUNCTION__, " deleting profile ", profileId);
                    profileFound = true;
                    continue;
                }
            }
            newRoot[subsystem]["requestProfileList"]["profiles"][newCount]
                = data.stateRootObj[subsystem]["requestProfileList"]
                  ["profiles"][index];
            newCount++;
        }
        if (profileFound) {
            data.stateRootObj[subsystem]["requestProfileList"]["profiles"]
                = newRoot[subsystem]["requestProfileList"]["profiles"];
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            LOG(DEBUG, __FUNCTION__, " profile not found ");
            error = telux::common::ErrorCode::EXTENDED_INTERNAL;
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataProfileServerImpl::ModifyProfile(ServerContext* context,
    const dataStub::ModifyProfileRequest* request, dataStub::DefaultReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_API_SLOT2_JSON
        : DATA_PROFILE_API_SLOT1_JSON;
    std::string stateJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_STATE_SLOT2_JSON
        : DATA_PROFILE_STATE_SLOT1_JSON;
    std::string subsystem = "IDataProfileManager";
    std::string method = "modifyProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        Json::Value updatedProfile;
        int profileId = request->profile_id();
        int currentProfileCount = data.stateRootObj[subsystem]
            ["requestProfileList"]["profiles"].size();

        int profileIndex = 0;
        bool profileFound = false;
        for (; profileIndex < currentProfileCount; profileIndex++) {
            int itrProfileId = data.stateRootObj[subsystem]
                ["requestProfileList"]["profiles"][profileIndex]["profileId"].asInt();
            if (itrProfileId == profileId ) {
                LOG(DEBUG, __FUNCTION__, " profile found ", profileId);
                profileFound = true;
                break;
            }
        }

        if (profileFound) {
            updatedProfile["profileId"] = profileId;
            updatedProfile["profileName"] = request->profile_name();
            updatedProfile["apn"] = request->apn_name();
            updatedProfile["username"] = request->user_name();
            updatedProfile["password"] = request->password();
            updatedProfile["ipFamilyType"] =
                DataUtilsStub::convertIpFamilyEnumToString(
                request->ip_family_type().ip_family_type());
            updatedProfile["apnTypes"] = request->apn_types();
            updatedProfile["techPref"] =
                DataUtilsStub::convertTechPrefEnumToString(
                request->tech_preference().tech_preference());
            updatedProfile["authProtocolType"] =
                DataUtilsStub::convertAuthProtocolEnumToString(
                request->auth_type().auth_type());

            dataStub::EmergencyCapability emergencyAllowed =
                request->emergency_capability();
            if (emergencyAllowed ==
                dataStub::EmergencyCapability::UNSPECIFIED) {
                    emergencyAllowed = dataStub::EmergencyCapability::NOT_ALLOWED;
            }
            updatedProfile["emergencyAllowed"] =
                ((int)emergencyAllowed);

            Json::Value newRoot;
            for (auto idx = 0; idx < currentProfileCount; idx++) {
                if (idx == profileIndex) {
                    newRoot[subsystem]["requestProfileList"]["profiles"][profileIndex]
                        = updatedProfile;
                    continue;
                }
                newRoot[subsystem]["requestProfileList"]["profiles"][idx]
                    = data.stateRootObj[subsystem]["requestProfileList"]
                    ["profiles"][idx];
            }

            data.stateRootObj[subsystem]["requestProfileList"]["profiles"]
                = newRoot[subsystem]["requestProfileList"]["profiles"];
            LOG(DEBUG, __FUNCTION__, " profileId::", profileId);
            // Updating the new profile list in JSON File.
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            LOG(DEBUG, __FUNCTION__, " profile not found ");
            error = telux::common::ErrorCode::EXTENDED_INTERNAL;
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataProfileServerImpl::RequestProfileById(ServerContext* context,
    const dataStub::RequestProfileByIdRequest* request,
    dataStub::RequestProfileByIdReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->profile().slot_id() == SLOT_2)?
        DATA_PROFILE_API_SLOT2_JSON : DATA_PROFILE_API_SLOT1_JSON;
    std::string stateJsonPath = (request->profile().slot_id() == SLOT_2)?
        DATA_PROFILE_STATE_SLOT2_JSON : DATA_PROFILE_STATE_SLOT1_JSON;
    std::string subsystem = "IDataProfileManager";
    std::string method = "requestProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        bool profileFound = false;
        int profileId = request->profile().profile_id();
        std::string techPref = DataUtilsStub::convertTechPrefEnumToString(
            request->profile().tech_preference().tech_preference());

        int currentProfileCount =
            data.stateRootObj[subsystem]["requestProfileList"]["profiles"].size();
        int index = 0;
        for (; index < currentProfileCount; index++) {
            int itrProfileId =
                    data.stateRootObj[subsystem]["requestProfileList"]
                    ["profiles"][index]["profileId"].asInt();

            if (itrProfileId == profileId) {
                std::string itrTechPref = data.stateRootObj[subsystem]
                    ["requestProfileList"]["profiles"][index]["techPref"].asString();
                if (itrTechPref == techPref) {
                    LOG(DEBUG, __FUNCTION__, " profile found ", profileId);
                    profileFound = true;
                    break;
                }
            }
        }

        if (profileFound) {
            Json::Value requestedProfile =
                data.stateRootObj[subsystem]["requestProfileList"]
                ["profiles"][index];
            response->mutable_profile()->set_profile_id(
                requestedProfile["profileId"].asInt());
            response->mutable_profile()->set_profile_name(
                requestedProfile["profileName"].asString());
            response->mutable_profile()->set_apn_name(
                requestedProfile["apn"].asString());
            response->mutable_profile()->set_user_name(
                requestedProfile["username"].asString());
            response->mutable_profile()->set_password(
                requestedProfile["password"].asString());
            response->mutable_profile()->set_apn_types(
                requestedProfile["apnTypes"].asString());
            response->mutable_profile()->mutable_ip_family_type()->
                set_ip_family_type(DataUtilsStub::convertIpFamilyStringToEnum(
                requestedProfile["ipFamilyType"].asString()));
            response->mutable_profile()->mutable_tech_preference()->
                set_tech_preference(DataUtilsStub::convertTechPrefStringToEnum(
                requestedProfile["techPref"].asString()));
            response->mutable_profile()->mutable_auth_type()->
                set_auth_type(DataUtilsStub::convertAuthProtocolStringToEnum(
                requestedProfile["authProtocolType"].asString()));
            response->mutable_profile()->set_emergency_capability(
                (dataStub::EmergencyCapability)
                requestedProfile["emergencyAllowed"].asInt());
        } else {
            LOG(DEBUG, __FUNCTION__, " profile not found ");
            error = telux::common::ErrorCode::EXTENDED_INTERNAL;
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataProfileServerImpl::RequestProfileList(ServerContext* context,
    const dataStub::RequestProfileListRequest* request,
    dataStub::RequestProfileListReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_API_SLOT2_JSON
        : DATA_PROFILE_API_SLOT1_JSON;
    std::string stateJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_STATE_SLOT2_JSON
        : DATA_PROFILE_STATE_SLOT1_JSON;
    std::string subsystem = "IDataProfileManager";
    std::string method = "requestProfileList";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        int currentProfileCount =
            data.stateRootObj[subsystem]["requestProfileList"]
            ["profiles"].size();
        int index = 0;
        for (; index < currentProfileCount; index++) {
            dataStub::Profile *profile = response->add_profiles();
            Json::Value requestedProfile =
                data.stateRootObj[subsystem]["requestProfileList"]
                ["profiles"][index];
            profile->set_profile_id(requestedProfile["profileId"].asInt());
            profile->set_profile_name(requestedProfile["profileName"].asString());
            profile->set_apn_name(requestedProfile["apn"].asString());
            profile->set_user_name(requestedProfile["username"].asString());
            profile->set_password(requestedProfile["password"].asString());
            profile->set_apn_types(requestedProfile["apnTypes"].asString());
            dataStub::TechPreference techPref;
            techPref.set_tech_preference(DataUtilsStub::convertTechPrefStringToEnum(
                requestedProfile["techPref"].asString()));
            *profile->mutable_tech_preference() = techPref;
            dataStub::IpFamilyType ipFamilyType;
            ipFamilyType.set_ip_family_type(DataUtilsStub::convertIpFamilyStringToEnum(
                requestedProfile["ipFamilyType"].asString()));
            *profile->mutable_ip_family_type() = ipFamilyType;
            dataStub::AuthProtocolType authType;
            authType.set_auth_type(DataUtilsStub::convertAuthProtocolStringToEnum(
                requestedProfile["authProtocolType"].asString()));
            *profile->mutable_auth_type() = authType;
            profile->set_emergency_capability((dataStub::EmergencyCapability)
                requestedProfile["emergencyAllowed"].asInt());
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataProfileServerImpl::QueryProfile(ServerContext* context,
    const dataStub::QueryProfileRequest* request, dataStub::QueryProfileReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_API_SLOT2_JSON
        : DATA_PROFILE_API_SLOT1_JSON;
    std::string stateJsonPath = (request->slot_id() == SLOT_2)? DATA_PROFILE_STATE_SLOT2_JSON
        : DATA_PROFILE_STATE_SLOT1_JSON;
    std::string subsystem = "IDataProfileManager";
    std::string method = "queryProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    std::string profileName = request->profile_name();
    std::string apn = request->apn_name();
    std::string username = request->user_name();
    std::string password = request->password();
    std::string apnTypes = request->apn_types();
    std::string ipFamilyType =
        DataUtilsStub::convertIpFamilyEnumToString(
        request->ip_family_type().ip_family_type());
    std::string techPref =
        DataUtilsStub::convertTechPrefEnumToString(
        request->tech_preference().tech_preference());
    std::string authType =
        DataUtilsStub::convertAuthProtocolEnumToString(
        request->auth_type().auth_type());
    dataStub::EmergencyCapability emergencyAllowed =
        request->emergency_capability();

    if (data.status == telux::common::Status::SUCCESS) {
        int currentProfileCount =
            data.stateRootObj[subsystem]["requestProfileList"]
            ["profiles"].size();
        int index = 0;
        Json::Value requestedProfile;
        for (; index < currentProfileCount; index++) {
            requestedProfile =
                data.stateRootObj[subsystem]["requestProfileList"]
                ["profiles"][index];

            if ((requestedProfile["profileName"].asString() != profileName) &&
                    (!profileName.empty()) ) {
                continue;
            }

            if ((requestedProfile["apn"].asString() != apn) &&
                    (!apn.empty()) ) {
                continue;
            }

            if ((requestedProfile["username"].asString() != username) &&
                    (!username.empty()) ) {
                continue;
            }

            if ((requestedProfile["password"].asString() != password) &&
                    (!password.empty()) ) {
                continue;
            }

            if ((requestedProfile["techPref"].asString() != techPref) &&
                    (!techPref.empty()) ) {
                continue;
            }

            if ((requestedProfile["ipFamilyType"].asString() != ipFamilyType) &&
                    (!ipFamilyType.empty()) ) {
                continue;
            }

            if ((requestedProfile["authProtocolType"].asString() != authType) &&
                    (!authType.empty()) ) {
                continue;
            }

            if (emergencyAllowed ==
                dataStub::EmergencyCapability::UNSPECIFIED) {
                    emergencyAllowed = dataStub::EmergencyCapability::NOT_ALLOWED;
            }
            if ((requestedProfile["emergencyAllowed"].asInt() != emergencyAllowed)) {
                continue;
            }
            dataStub::Profile *profile = response->add_profiles();

            profile->set_profile_id(requestedProfile["profileId"].asInt());
            profile->set_profile_name(requestedProfile["profileName"].asString());
            profile->set_apn_name(requestedProfile["apn"].asString());
            profile->set_user_name(requestedProfile["username"].asString());
            profile->set_password(requestedProfile["password"].asString());
            profile->set_apn_types(requestedProfile["apnTypes"].asString());
            dataStub::TechPreference techPref;
            techPref.set_tech_preference(DataUtilsStub::convertTechPrefStringToEnum(
                requestedProfile["techPref"].asString()));
            *profile->mutable_tech_preference() = techPref;
            dataStub::IpFamilyType ipFamilyType;
            ipFamilyType.set_ip_family_type(DataUtilsStub::convertIpFamilyStringToEnum(
                requestedProfile["ipFamilyType"].asString()));
            *profile->mutable_ip_family_type() = ipFamilyType;
            dataStub::AuthProtocolType authType;
            authType.set_auth_type(DataUtilsStub::convertAuthProtocolStringToEnum(
                requestedProfile["authProtocolType"].asString()));
            profile->set_emergency_capability((dataStub::EmergencyCapability)
                requestedProfile["emergencyAllowed"].asInt());
            *profile->mutable_auth_type() = authType;
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}
