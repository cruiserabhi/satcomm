/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * SimProfile  implementation
 */

#include <sstream>
#include <telux/tel/SimProfile.hpp>
#include "common/Logger.hpp"

namespace telux {
namespace tel {

SimProfile::SimProfile(int profileId, ProfileType profileType, const std::string& iccid,
    bool isActive, const std::string& nickName, const std::string& spn, const std::string &name,
    IconType iconType, std::vector<uint8_t> icon, ProfileClass profileClass,
    PolicyRuleMask policyRuleMask, int slotId)
   : profileId_(profileId)
   , profileType_(profileType)
   , iccid_(iccid)
   , isActive_(isActive)
   , nickName_(nickName)
   , spn_(spn)
   , name_(name)
   , iconType_(iconType)
   , icon_(icon)
   , profileClass_(profileClass)
   , policyRuleMask_(policyRuleMask)
   , slotId_(slotId) {
}

int SimProfile::getSlotId() {
    return slotId_;
}

int SimProfile::getProfileId() {
    return profileId_;
}

ProfileType SimProfile::getType() {
    return profileType_;
}

const std::string& SimProfile::getIccid() {
    return iccid_;
}

bool SimProfile::isActive() {
    return isActive_;
}

const std::string& SimProfile::getNickName() {
    return nickName_;
}

const std::string& SimProfile::getSPN() {
    return spn_;
}

const std::string& SimProfile::getName() {
    return name_;
}

IconType SimProfile::getIconType() {
    return iconType_;
}

std::vector<uint8_t> SimProfile::getIcon() {
    return icon_;
}

ProfileClass SimProfile::getClass() {
    return profileClass_;
}

PolicyRuleMask SimProfile::getPolicyRule() {
    return policyRuleMask_;
}

std::string profileTypeToString(ProfileType profileType) {
    std::string type = "";
    switch(profileType) {
        case ProfileType::REGULAR :
            type = "REGULAR";
            break;
        case ProfileType::EMERGENCY :
            type = "EMERGENCY";
            break;
        default:
            type = "UNKNOWN";
            break;
    }
    return type;
}
std::string profileClassToString(ProfileClass profileClass) {
    std::string profClass = "";
    switch(profileClass) {
        case ProfileClass::TEST :
            profClass = "TEST";
            break;
        case ProfileClass::PROVISIONING :
            profClass = "PROVISIONING";
            break;
        case ProfileClass::OPERATIONAL :
            profClass = "OPERATIONAL";
            break;
        default:
            profClass = "UNKNOWN";
            break;
    }
    return profClass;
}

std::string iconTypeToString(IconType iconType) {
    std::string icon = "";
    switch(iconType) {
        case IconType::JPEG :
            icon = "JPEG";
            break;
        case IconType::PNG :
            icon = "PNG";
            break;
        default:
            icon = "UNKNOWN";
            break;
    }
    return icon;
}

std::string convertPolicyRuleMaskToString(PolicyRuleMask policyRuleMask) {
    std::string ruleMask = "";
    if (policyRuleMask[static_cast<int>(telux::tel::PolicyRuleType::PROFILE_DISABLE_NOT_ALLOWED)]) {
        ruleMask = ruleMask + " PROFILE_DISABLE_NOT_ALLOWED";
    }
    if (policyRuleMask[static_cast<int>(telux::tel::PolicyRuleType::PROFILE_DELETE_NOT_ALLOWED)]) {
        ruleMask = ruleMask + " PROFILE_DELETE_NOT_ALLOWED";
    }
    if (policyRuleMask[static_cast<int>(telux::tel::PolicyRuleType::PROFILE_DELETE_ON_DISABLE)]) {
        ruleMask = ruleMask + " PROFILE_DELETE_ON_DISABLE";
    }

    if(ruleMask.empty()) {
        ruleMask = " No PPR/s set";
    }
    return ruleMask;
}

std::string SimProfile::toString() {
    std::stringstream ss;
    ss << " Profile Id: " << profileId_ << ", Profile Type: " << profileTypeToString(profileType_)
       << ", ICCID: " << iccid_ << ", Active: " << isActive_ << ", NickName: " << nickName_
       << ", SPN: " << spn_ << ", Profile Name: " << name_
       << ", Profile Icon Type: " << iconTypeToString(iconType_)
       << ", Profile Class: " << profileClassToString(profileClass_)
       << ", \n Policy Rules: " << convertPolicyRuleMaskToString(policyRuleMask_);
    return ss.str();
}

}  // end of namespace tel

}  // end namespace telux
