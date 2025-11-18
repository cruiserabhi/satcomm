/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:

 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef DATAUTILS_HPP
#define DATAUTILS_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/ServingSystemManager.hpp>
#include "../../../common/utils/Utils.hpp"

#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_TCP_UDP 253
class DataUtils {
public:

    template <typename T>
    static void validateInput(T &input, std::initializer_list<T> list) {
        return Utils::validateInput(input, list);
    }

   static std::string callEndReasonTypeToString(telux::data::EndReasonType type);
   static int callEndReasonCode(telux::data::DataCallEndReason ceReason);
   static std::string techPreferenceToString(telux::data::TechPreference techPref);
   static std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType);
   static std::string dataCallStatusToString(telux::data::DataCallStatus dcStatus);
   static std::string usageResetReasonToString(telux::data::UsageResetReason usageResetReason);
   static std::string bearerTechToString(telux::data::DataBearerTechnology bearerTech);
   static std::string operationTypeToString(telux::data::OperationType oprType);
   static std::string protocolToString(telux::data::IpProtocol proto);
   static telux::data::IpProtocol getProtcol(std::string protoStr);
   static std::string drbStatusToString(telux::data::DrbStatus stat);
   static std::string serviceRatToString(telux::data::NetworkRat rat);
   static std::string vlanInterfaceToString(
    telux::data::InterfaceType interface, telux::data::OperationType oprType);
   static std::string trafficClassToString(telux::data::IpTrafficClassType tc);
   static std::string flowStateEventToString(telux::data::QosFlowStateChangeEvent state);
   static void logQosDetails(std::shared_ptr<telux::data::TrafficFlowTemplate> &tft);
   static void printFilterDetails(std::shared_ptr<telux::data::IIpFilter> filter);
   static void populateBackhaulInfo(telux::data::BackhaulInfo& backhaulInfo);
   static std::string backhaulToString(telux::data::BackhaulType backhaul);
   static std::string emergencyAllowedTypeToString(telux::data::EmergencyCapability cap);
   static std::string networkTypeToString(telux::data::NetworkType networkType);
};

#endif  // DATAUTILS_HPP
