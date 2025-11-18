/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sstream>

#include <telux/data/DataProfile.hpp>

namespace telux {
namespace data {

DataProfile::DataProfile(int id, const std::string &name, const std::string &apn,
                         const std::string &username, const std::string &password,
                         IpFamilyType ipFamilyType, TechPreference techPref,
                         AuthProtocolType authType, ApnTypes apnTypes,
                         EmergencyCapability emergencyAllowed)
   : id_(id)
   , name_(name)
   , apn_(apn)
   , username_(username)
   , password_(password)
   , ipFamilyType_(ipFamilyType)
   , techPref_(techPref)
   , authType_(authType)
   , apnTypes_(apnTypes)
   , emergencyAllowed_(emergencyAllowed) {
}

int DataProfile::getId() {
   return id_;
}

std::string DataProfile::getName() {
   return name_;
}

std::string DataProfile::getUserName() {
   return username_;
}

std::string DataProfile::getPassword() {
   return password_;
}
TechPreference DataProfile::getTechPreference() {
   return techPref_;
}

AuthProtocolType DataProfile::getAuthProtocolType() {
   return authType_;
}

IpFamilyType DataProfile::getIpFamilyType() {
   return ipFamilyType_;
}

std::string DataProfile::getApn() {
   return apn_;
}

ApnTypes DataProfile::getApnTypes() {
   return apnTypes_;
}

EmergencyCapability DataProfile::getIsEmergencyAllowed() {
    return emergencyAllowed_;
}

std::string DataProfile::toString() {
  std::stringstream ss;
  ss << " id: " << id_ << ", name: " << name_ << ", apn: " << apn_ << ", username: " << username_
    << ", password: " << password_ << ", IP Family: " << static_cast<int>(ipFamilyType_)
    << ", Tech Pref: " << static_cast<int>(techPref_)
    << ", Auth Type: " << static_cast<int>(authType_) << ", Apn Type: " << apnTypes_.to_string()
    << ", Emergency Allowed: " << static_cast<int>(emergencyAllowed_);
    return ss.str();
}
}
}
