/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * Utility helper class
 * @brief RefAppUtils class performs common error code conversions
 */

#ifndef POWER_REF_APP_UTILS_HPP
#define POWER_REF_APP_UTILS_HPP

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/ServingSystemManager.hpp>
#include <telux/common/Log.hpp>
#include <telux/power/TcuActivityDefines.hpp>

#include "define.hpp"

class RefAppUtils
{
public:

    /**
     * Get error description for given ErrorCode
     */
    static std::string getErrorCodeAsString(telux::common::ErrorCode error);
    static std::map<telux::common::ErrorCode, std::string> errorCodeToStringMap_;
    static TriggerType stringToTriggerType(std::string stringTrigger);
    static bool stringToBool(std::string enable);
    static bool stringToProtocol(std::string protocol);

    static telux::data::IpProtocol getProtcol(std::string protoStr);
    static int callEndReasonCode(telux::data::DataCallEndReason ceReason);
    static std::string callEndReasonTypeToString(telux::data::EndReasonType type);
    static std::string techPreferenceToString(telux::data::TechPreference techPref);
    static std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType);
    static std::string dataCallStatusToString(telux::data::DataCallStatus dcStatus);
    static std::string bearerTechToString(telux::data::DataBearerTechnology bearerTech);
    static std::string operationTypeToString(telux::data::OperationType oprType);
    static std::string protocolToString(telux::data::IpProtocol proto);
    static std::string drbStatusToString(telux::data::DrbStatus stat);
    static std::string serviceRatToString(telux::data::NetworkRat rat);
    static std::string eventStatusToString(EventStatus status);
    static std::string tcuActivityStateToString(telux::power::TcuActivityState state);
    static std::string teluxStatusToString(telux::common::Status status);
    static std::string triggerTypeToString(TriggerType triggeredBy);
    static std::string serviceStatusToString(telux::common::ServiceStatus status);
    static std::string dataRestrictModeTypeToString(telux::data::DataRestrictModeType filterMode);
};

#endif
