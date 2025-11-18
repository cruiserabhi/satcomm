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
 *
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <iomanip>
#include <algorithm>

#include <telux/common/DeviceConfig.hpp>
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

std::string DataUtils::usageResetReasonToString(telux::data::UsageResetReason usageResetReason) {
   switch(usageResetReason) {
      case telux::data::UsageResetReason::SUBSYSTEM_UNAVAILABLE:
         return "SUBSYSTEM_UNAVAILABLE";
      case telux::data::UsageResetReason::BACKHAUL_SWITCHED:
         return "BACKHAUL_SWITCHED";
      case telux::data::UsageResetReason::DEVICE_DISCONNECTED:
         return "DEVICE_DISCONNECTED";
      case telux::data::UsageResetReason::WLAN_DISABLED:
         return "WLAN_DISABLED";
      case telux::data::UsageResetReason::WWAN_DISCONNECTED:
         return "WWAN_DISCONNECTED";
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

telux::data::IpProtocol DataUtils::getProtcol(std::string protoStr) {
    std::string protoStrToCompare = protoStr;
    std::transform(protoStrToCompare.begin(), protoStrToCompare.end(), protoStrToCompare.begin(),
        [](unsigned char ch) { return std::tolower(ch); });

    telux::data::IpProtocol prot = 0;
    if(protoStrToCompare.compare("udp") == 0) {
        prot = 17;
    }
    else if (protoStrToCompare.compare("tcp") == 0) {
        prot = 6;
    }
    else if (protoStrToCompare.compare("igmp") == 0) {
        prot = 2;
    }
    else if (protoStrToCompare.compare("icmp") == 0) {
        prot = 1;
    }
    else if (protoStrToCompare.compare("esp") == 0) {
        prot = 50;
    }
    else if (protoStrToCompare.compare("tcp_udp") == 0) {
        prot = 253;
    }
    else if (protoStrToCompare.compare("icmp6") == 0) {
        prot = 58;
    }
    else {
        std::cout << "Error: invalid protocol \n ";
    }
    return prot;
}

std::string DataUtils::drbStatusToString(telux::data::DrbStatus stat) {
    std::string statusStr = "UNKNOWN";
    switch (stat) {
        case telux::data::DrbStatus::DORMANT:
            statusStr = "DORMANT";
            break;
        case telux::data::DrbStatus::ACTIVE:
            statusStr = "ACTIVE";
            break;
        case telux::data::DrbStatus::UNKNOWN:
        default:
            break;
    }
    return statusStr;
}

std::string DataUtils::serviceRatToString(telux::data::NetworkRat rat) {
    std::string ratStr = "UNKNOWN";
    switch (rat) {
        case telux::data::NetworkRat::CDMA_1X:
            ratStr = "CDMA 1X";
            break;
        case telux::data::NetworkRat::CDMA_EVDO:
            ratStr = "CDMA EVDO";
            break;
        case telux::data::NetworkRat::GSM:
            ratStr = "GSM";
            break;
        case telux::data::NetworkRat::WCDMA:
            ratStr = "WCDMA";
            break;
        case telux::data::NetworkRat::LTE:
            ratStr = "LTE";
            break;
        case telux::data::NetworkRat::TDSCDMA:
            ratStr = "TDSCDMA";
            break;
        case telux::data::NetworkRat::NR5G:
            ratStr = "NR5G";
            break;
        default:
            break;
    }
    return ratStr;
}

std::string DataUtils::vlanInterfaceToString(
   telux::data::InterfaceType interface, telux::data::OperationType oprType) {
   std::string ifName = "UNKNOWN";
   switch(interface) {
      case telux::data::InterfaceType::WLAN:
         ifName = "WLAN";
         break;
      case telux::data::InterfaceType::ETH:
         ifName = "ETH";
         break;
      case telux::data::InterfaceType::ECM:
         ifName = "ECM";
         break;
      case telux::data::InterfaceType::RNDIS:
         ifName = "RNDIS";
         break;
      case telux::data::InterfaceType::MHI:
         ifName = "MHI";
         break;
      case telux::data::InterfaceType::VMTAP0:
#ifdef TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED
         if(oprType == telux::data::OperationType::DATA_LOCAL) {
            ifName = "VMTAP0";
         } else {
            ifName = "VMTAP-TELEVM";
         }
#else
         ifName = "VMTAP-TELEVM";
#endif
         break;
      case telux::data::InterfaceType::VMTAP1:
#ifdef TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED
         if(oprType == telux::data::OperationType::DATA_LOCAL) {
            ifName = "VMTAP1";
         } else {
            ifName = "VMTAP-FOTAVM";
         }
#else
         ifName = "VMTAP-FOTAVM";
#endif
         break;
      default:
         break;
   }
   return ifName;
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

void DataUtils::printFilterDetails(std::shared_ptr<telux::data::IIpFilter> filter) {

   telux::data::IPv4Info ipv4Info_ = filter->getIPv4Info();
   if(!ipv4Info_.srcAddr.empty()) {
      std::cout << "\tIPv4 Src Address : " << ipv4Info_.srcAddr << std::endl;
   }
   if(!ipv4Info_.srcSubnetMask.empty()) {
      std::cout << "\tIPv4 Src Subnet Mask : " << ipv4Info_.srcSubnetMask << std::endl;
   }
   if(!ipv4Info_.destAddr.empty()) {
      std::cout << "\tIPv4 Dest Address : " << ipv4Info_.destAddr << std::endl;
   }
   if(!ipv4Info_.destSubnetMask.empty()) {
      std::cout << "\tIPv4 Dest Subnet Mask : " << ipv4Info_.destSubnetMask << std::endl;
   }
   if(ipv4Info_.value > 0) {
      std::cout << "\tIPv4 Type of service value : " << (int)ipv4Info_.value << std::endl;
   }
   if(ipv4Info_.mask > 0) {
      std::cout << "\tIPv4 Type of service mask : " << (int)ipv4Info_.mask << std::endl;
   }

   telux::data::IPv6Info ipv6Info_ = filter->getIPv6Info();
   if(!ipv6Info_.srcAddr.empty()) {
      std::cout << "\tIPv6 Src Address : " << ipv6Info_.srcAddr << std::endl;
   }
   if(!ipv6Info_.destAddr.empty()) {
      std::cout << "\tIPv6 Dest Address : " << ipv6Info_.destAddr << std::endl;
   }
   if(ipv6Info_.val > 0) {
      std::cout << "\tIPv6 Traffic class value : " << (int)ipv6Info_.val << std::endl;
   }
   if(ipv6Info_.mask > 0) {
      std::cout << "\tIPv6 Traffic class mask : " << (int)ipv6Info_.mask << std::endl;
   }
   if(ipv6Info_.flowLabel > 0) {
      std::cout << "\tIPv6 Flow label : " << (int)ipv6Info_.flowLabel << std::endl;
   }

   telux::data::IpProtocol proto = filter->getIpProtocol();
   switch (proto) {
      case PROTO_TCP: {
         auto tcpFilter = std::dynamic_pointer_cast<telux::data::ITcpFilter>(filter);
         if (tcpFilter) {
            telux::data::TcpInfo portInfo_ = tcpFilter->getTcpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tTCP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tTCP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tTCP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tTCP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      case PROTO_UDP: {
         auto udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(filter);
         if (udpFilter) {
            telux::data::UdpInfo portInfo_ = udpFilter->getUdpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tUDP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tUDP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tUDP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tUDP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      case PROTO_TCP_UDP: {
         auto tcpFilter = std::dynamic_pointer_cast<telux::data::ITcpFilter>(filter);
         if (tcpFilter) {
            telux::data::TcpInfo portInfo_ = tcpFilter->getTcpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tTCP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tTCP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tTCP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tTCP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
         auto udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(filter);
         if (udpFilter) {
            telux::data::UdpInfo portInfo_ = udpFilter->getUdpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tUDP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tUDP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tUDP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tUDP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      default: {
         std::cout << " Invalid XPort Protocol" <<std::endl;
      }
   }
}

void DataUtils::logQosDetails(
    std::shared_ptr<telux::data::TrafficFlowTemplate> &tft) {
   std::cout << " QoS Identifier : " << tft->qosId << std::endl;

   if (tft->mask.test(telux::data::QosFlowMaskType::MASK_FLOW_TX_GRANTED) &&
       (tft->txGrantedFlow.mask.test(
            telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
        tft->txGrantedFlow.mask.test(
            telux::data::QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX))) {
      std::cout << " TX QOS FLow Granted: " << std::endl;

      if (tft->txGrantedFlow.mask.test(
              telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS)) {
         std::cout << "\tIP FLow Traffic class: "
                   << DataUtils::trafficClassToString(
                          tft->txGrantedFlow.tfClass)
                   << std::endl;
      }
      if (tft->txGrantedFlow.mask.test(
              telux::data::QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)) {
         std::cout << "\tMaximum required data rate (bits per second): "
                   << tft->txGrantedFlow.dataRate.maxRate << std::endl;
         std::cout << "\tMinimum required data rate (bits per second): "
                   << tft->txGrantedFlow.dataRate.minRate << std::endl;
      }
   }

   if (tft->mask.test(telux::data::QosFlowMaskType::MASK_FLOW_RX_GRANTED) &&
       (tft->rxGrantedFlow.mask.test(
            telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
        tft->rxGrantedFlow.mask.test(
            telux::data::QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX))) {
      std::cout << " RX QOS FLow Granted: " << std::endl;

      if (tft->rxGrantedFlow.mask.test(
              telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS)) {
         std::cout << "\tIP FLow Traffic class: "
                   << DataUtils::trafficClassToString(
                          tft->rxGrantedFlow.tfClass)
                   << std::endl;
      }
      if (tft->rxGrantedFlow.mask.test(
              telux::data::QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)) {
         std::cout << "\tMaximum required data rate (bits per second): "
                   << tft->rxGrantedFlow.dataRate.maxRate << std::endl;
         std::cout << "\tMinimum required data rate (bits per second): "
                   << tft->rxGrantedFlow.dataRate.minRate << std::endl;
      }
   }

   if (tft->mask.test(telux::data::QosFlowMaskType::MASK_FLOW_TX_FILTERS)) {
      for (uint32_t i = 0; i < tft->txFiltersLength; i++) {
         for (auto filter : tft->txFilters[i].filter) {
            telux::data::IpProtocol proto = filter->getIpProtocol();
            std::string protocol = "TCP";
            if (PROTO_UDP == proto) {
               protocol = "UDP";
            }
            std::cout << " " << protocol << " TX Filter: " << (i + 1)
                      << std::endl;
            std::cout << "\tFilter ID: " << tft->txFilters[i].filterId
                      << std::endl;
            std::cout << "\tFilter Precedence: "
                      << tft->txFilters[i].filterPrecedence << std::endl;
            if (filter) {
               std::cout << "\tIP Family: "
                         << DataUtils::ipFamilyTypeToString(
                                filter->getIpFamily())
                         << std::endl;
               DataUtils::printFilterDetails(filter);
            }
         }
      }
   }

   if (tft->mask.test(telux::data::QosFlowMaskType::MASK_FLOW_RX_FILTERS)) {
      for (uint32_t i = 0; i < tft->rxFiltersLength; i++) {
         for (auto filter : tft->rxFilters[i].filter) {
            telux::data::IpProtocol proto = filter->getIpProtocol();
            std::string protocol = "TCP";
            if (PROTO_UDP == proto) {
               protocol = "UDP";
            }
            std::cout << " " << protocol << " RX Filter: " << (i + 1)
                      << std::endl;
            std::cout << "\tFilter ID: " << tft->rxFilters[i].filterId
                      << std::endl;
            std::cout << "\tFilter Precedence: "
                      << tft->rxFilters[i].filterPrecedence << std::endl;
            if (filter) {
               std::cout << "\tIP Family: "
                         << DataUtils::ipFamilyTypeToString(
                                filter->getIpFamily())
                         << std::endl;
               DataUtils::printFilterDetails(filter);
            }
         }
      }
   }
}

std::string DataUtils::backhaulToString(telux::data::BackhaulType backhaul) {
   std::string retString = "UNKNOWN";
   switch(backhaul) {
      case telux::data::BackhaulType::ETH:
         retString = "ETH";
         break;
      case telux::data::BackhaulType::USB:
         retString = "USB";
         break;
      case telux::data::BackhaulType::WLAN:
         retString = "WLAN";
         break;
      case telux::data::BackhaulType::WWAN:
         retString = "WWAN";
         break;
      case telux::data::BackhaulType::BLE:
         retString = "BLE";
         break;
      default:
         break;
   }
   return retString;
}

// Retuns true if multiple backhauls are supported
void DataUtils::populateBackhaulInfo(telux::data::BackhaulInfo& backhaulInfo) {
   int backhaul = 0, profileId = 0, vlanId = -1;
   std::cout << "Enter Backhaul Type (0-Wlan, 1-WWAN, 2-ETH): ";
   std::cin >> backhaul;
   Utils::validateInput(backhaul, {0, 1, 2});
   std::cout << std::endl;
   if (backhaul == 1) {
      backhaulInfo.backhaul = telux::data::BackhaulType::WWAN;
      int slotId = DEFAULT_SLOT_ID;
      if (telux::common::DeviceConfig::isMultiSimSupported()) {
         slotId = Utils::getValidSlotId();
      }
      backhaulInfo.slotId = static_cast<SlotId>(slotId);
      std::cout << "Enter Profile Id: ";
      std::cin >> profileId;
      Utils::validateInput(profileId);
      backhaulInfo.profileId = profileId;
   } else if (backhaul == 0) {
      backhaulInfo.backhaul = telux::data::BackhaulType::WLAN;
   } else if (backhaul == 2) {
      backhaulInfo.backhaul = telux::data::BackhaulType::ETH;
      std::cout << "Enter the vlan Id associated with backhaul: ";
      std::cin >> vlanId;
      Utils::validateInput(vlanId);
      backhaulInfo.vlanId = vlanId;
   }
}

std::string DataUtils::emergencyAllowedTypeToString(telux::data::EmergencyCapability cap) {
  std::string retString{"No"};
  if (cap == telux::data::EmergencyCapability::ALLOWED)
    retString = "yes";
  return retString;
}

std::string DataUtils::networkTypeToString(telux::data::NetworkType networkType) {
    switch (networkType) {
        case telux::data::NetworkType::LAN:
            return "LAN";
        case telux::data::NetworkType::WAN:
            return "WAN";
        default:
            return "UNKNOWN";
    }
}

