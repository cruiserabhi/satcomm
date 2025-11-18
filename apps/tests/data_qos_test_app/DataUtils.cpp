/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#include <iostream>
#include <iomanip>

#include "DataUtils.hpp"

std::string DataUtils::techPreferenceToString(telux::data::TechPreference techPref) {
   switch(techPref) {
      case telux::data::TechPreference::TP_3GPP:
         return "3gpp";
      case telux::data::TechPreference::TP_3GPP2:
         return "3gpp2";
      case telux::data::TechPreference::TP_ANY:
      default:
         return "Any";
   }
}

std::string DataUtils::ipFamilyTypeToString(telux::data::IpFamilyType ipType) {
   switch(ipType) {
      case telux::data::IpFamilyType::IPV4:
         return "IPv4";
      case telux::data::IpFamilyType::IPV6:
         return "IPv6";
      case telux::data::IpFamilyType::IPV4V6:
         return "IPv4v6";
      case telux::data::IpFamilyType::UNKNOWN:
      default:
         return "NA";
   }
}

std::string DataUtils::operationTypeToString(telux::data::OperationType oprType) {
   switch(oprType) {
      case telux::data::OperationType::DATA_LOCAL:
         return "LOCAL";
      case telux::data::OperationType::DATA_REMOTE:
         return "REMOTE";
      default:
         return "NA";
   }
}

std::string DataUtils::callEndReasonTypeToString(telux::data::EndReasonType type) {
   switch(type) {
      case telux::data::EndReasonType::CE_MOBILE_IP:
         return "CE_MOBILE_IP";
      case telux::data::EndReasonType::CE_INTERNAL:
         return "CE_INTERNAL";
      case telux::data::EndReasonType::CE_CALL_MANAGER_DEFINED:
         return "CE_CALL_MANAGER_DEFINED";
      case telux::data::EndReasonType::CE_3GPP_SPEC_DEFINED:
         return "CE_3GPP_SPEC_DEFINED";
      case telux::data::EndReasonType::CE_PPP:
         return "CE_PPP";
      case telux::data::EndReasonType::CE_EHRPD:
         return "CE_EHRPD";
      case telux::data::EndReasonType::CE_IPV6:
         return "CE_IPV6";
      case telux::data::EndReasonType::CE_UNKNOWN:
         return "CE_UNKNOWN";
      default: { return "CE_UNKNOWN"; }
   }
}

int DataUtils::callEndReasonCode(telux::data::DataCallEndReason ceReason) {
   switch(ceReason.type) {
      case telux::data::EndReasonType::CE_MOBILE_IP:
         return static_cast<int>(ceReason.IpCode);
      case telux::data::EndReasonType::CE_INTERNAL:
         return static_cast<int>(ceReason.internalCode);
      case telux::data::EndReasonType::CE_CALL_MANAGER_DEFINED:
         return static_cast<int>(ceReason.cmCode);
      case telux::data::EndReasonType::CE_3GPP_SPEC_DEFINED:
         return static_cast<int>(ceReason.specCode);
      case telux::data::EndReasonType::CE_PPP:
         return static_cast<int>(ceReason.pppCode);
      case telux::data::EndReasonType::CE_EHRPD:
         return static_cast<int>(ceReason.ehrpdCode);
      case telux::data::EndReasonType::CE_IPV6:
         return static_cast<int>(ceReason.ipv6Code);
      case telux::data::EndReasonType::CE_UNKNOWN:
         return -1;
      default: { return -1; }
   }
}

std::string DataUtils::dataCallStatusToString(telux::data::DataCallStatus dcStatus) {
   switch(dcStatus) {
      case telux::data::DataCallStatus::NET_CONNECTED:
         return "CONNECTED";
      case telux::data::DataCallStatus::NET_NO_NET:
         return "NO_NET";
      case telux::data::DataCallStatus::NET_IDLE:
         return "IDLE";
      case telux::data::DataCallStatus::NET_CONNECTING:
         return "CONNECTING";
      case telux::data::DataCallStatus::NET_DISCONNECTING:
         return "DISCONNECTING";
      case telux::data::DataCallStatus::NET_RECONFIGURED:
         return "RECONFIGURED";
      case telux::data::DataCallStatus::NET_NEWADDR:
         return "NEWADDR";
      case telux::data::DataCallStatus::NET_DELADDR:
         return "DELADDR";
      default: { return "UNKNOWN"; }
   }
}

std::string DataUtils::bearerTechToString(telux::data::DataBearerTechnology bearerTech) {
   switch(bearerTech) {
      case telux::data::DataBearerTechnology::CDMA_1X:
         return "1X technology";
      case telux::data::DataBearerTechnology::EVDO_REV0:
         return "CDMA Rev 0";
      case telux::data::DataBearerTechnology::EVDO_REVA:
         return "CDMA Rev A";
      case telux::data::DataBearerTechnology::EVDO_REVB:
         return "CDMA Rev B";
      case telux::data::DataBearerTechnology::EHRPD:
         return "EHRPD";
      case telux::data::DataBearerTechnology::FMC:
         return "Fixed mobile convergence";
      case telux::data::DataBearerTechnology::HRPD:
         return "HRPD";
      case telux::data::DataBearerTechnology::BEARER_TECH_3GPP2_WLAN:
         return "3GPP2 IWLAN";
      case telux::data::DataBearerTechnology::WCDMA:
         return "WCDMA";
      case telux::data::DataBearerTechnology::GPRS:
         return "GPRS";
      case telux::data::DataBearerTechnology::HSDPA:
         return "HSDPA";
      case telux::data::DataBearerTechnology::HSUPA:
         return "HSUPA";
      case telux::data::DataBearerTechnology::EDGE:
         return "EDGE";
      case telux::data::DataBearerTechnology::LTE:
         return "LTE";
      case telux::data::DataBearerTechnology::HSDPA_PLUS:
         return "HSDPA+";
      case telux::data::DataBearerTechnology::DC_HSDPA_PLUS:
         return "DC HSDPA+.";
      case telux::data::DataBearerTechnology::HSPA:
         return "HSPA";
      case telux::data::DataBearerTechnology::BEARER_TECH_64_QAM:
         return "64 QAM";
      case telux::data::DataBearerTechnology::TDSCDMA:
         return "TDSCDMA";
      case telux::data::DataBearerTechnology::GSM:
         return "GSM";
      case telux::data::DataBearerTechnology::BEARER_TECH_3GPP_WLAN:
         return "3GPP WLAN";
      case telux::data::DataBearerTechnology::BEARER_TECH_5G:
         return "5G";
      default: { return "UNKNOWN"; }
   }
}

std::string DataUtils::protocolToString(telux::data::IpProtocol proto) {
   switch(proto) {
      case 1:
         return "ICMP";
      case 2:
         return "IGMP";
      case 6:
         return "TCP";
      case 17:
         return "UDP";
      case 50:
         return "ESP";
      default: {
         return "Unknown";
      }
   }
}

std::string DataUtils::flowStateEventToString(telux::data::QosFlowStateChangeEvent state) {
   switch(state) {
      case telux::data::QosFlowStateChangeEvent::ACTIVATED:
         return "ACTIVATED";
      case telux::data::QosFlowStateChangeEvent::MODIFIED:
         return "MODIFIED";
      case telux::data::QosFlowStateChangeEvent::DELETED:
         return "DELETED";
      default: {
         return "Unknown";
      }
   }
}

std::string DataUtils::trafficClassToString(telux::data::IpTrafficClassType tc) {
   switch(tc) {
      case telux::data::IpTrafficClassType::CONVERSATIONAL:
         return "CONVERSATIONAL";
      case telux::data::IpTrafficClassType::STREAMING:
         return "STREAMING";
      case telux::data::IpTrafficClassType::INTERACTIVE:
         return "INTERACTIVE";
      case telux::data::IpTrafficClassType::BACKGROUND:
         return "BACKGROUND";
      default: {
         return "UNKNOWN";
      }
   }
}