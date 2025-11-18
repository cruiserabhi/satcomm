/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/data/DataDefines.hpp>
#include "common/Logger.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

extern "C" {
#include <arpa/inet.h>
}

class DataUtilsStub {
public:
    static ::dataStub::TechPreference::TechPref convertTechPrefStringToEnum(
        std::string techPref) {
        LOG(DEBUG, __FUNCTION__);

        ::dataStub::TechPreference::TechPref enumTechPref;
        if (techPref == "TP_3GPP") {
            enumTechPref = ::dataStub::TechPreference::TP_3GPP;
        } else if (techPref == "TP_3GPP2") {
            enumTechPref = ::dataStub::TechPreference::TP_3GPP2;
        } else if (techPref == "TP_ANY") {
            enumTechPref = ::dataStub::TechPreference::TP_ANY;
        } else {
            enumTechPref = ::dataStub::TechPreference::UNKNOWN;
        }
        return enumTechPref;
    }

    static ::dataStub::IpFamilyType::Type convertIpFamilyStringToEnum(
        std::string ipFamily) {
        LOG(DEBUG, __FUNCTION__);

        ::dataStub::IpFamilyType::Type ipFamilyType;
        if (ipFamily == "IPV4") {
            ipFamilyType = ::dataStub::IpFamilyType::IPV4;
        } else if (ipFamily == "IPV6") {
            ipFamilyType = ::dataStub::IpFamilyType::IPV6;
        } else if (ipFamily == "IPV4V6") {
            ipFamilyType = ::dataStub::IpFamilyType::IPV4V6;
        } else {
            ipFamilyType = ::dataStub::IpFamilyType::UNKNOWN;
        }
        return ipFamilyType;
    }

    static ::dataStub::AuthProtocolType::AuthProto convertAuthProtocolStringToEnum(
        std::string authProtocol) {
        LOG(DEBUG, __FUNCTION__);

        ::dataStub::AuthProtocolType::AuthProto authProtocolType;
        if (authProtocol == "AUTH_PAP") {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_PAP;
        } else if (authProtocol == "AUTH_CHAP") {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_CHAP;
        } else if (authProtocol == "AUTH_PAP_CHAP") {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_PAP_CHAP;
        } else {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_NONE;
        }
        return authProtocolType;
    }

    static std::string convertTechPrefEnumToString(
        ::dataStub::TechPreference::TechPref enumTechPref) {
        LOG(DEBUG, __FUNCTION__);

        std::string techPref;
        if (enumTechPref == ::dataStub::TechPreference::TP_3GPP) {
            techPref = "TP_3GPP";
        } else if (enumTechPref == ::dataStub::TechPreference::TP_3GPP2) {
            techPref = "TP_3GPP2";
        } else if (enumTechPref == ::dataStub::TechPreference::TP_ANY) {
            techPref = "TP_ANY";
        } else {
            techPref = "Unknown";
        }
        LOG(DEBUG, __FUNCTION__, " TechPreference is :",  techPref);
        return techPref;
    }

    static std::string convertIpFamilyEnumToString(
        ::dataStub::IpFamilyType::Type ipFamilyType) {
        LOG(DEBUG, __FUNCTION__);

        std::string ipFamily;
        if (ipFamilyType == ::dataStub::IpFamilyType::IPV4) {
            ipFamily = "IPV4";
        } else if (ipFamilyType == ::dataStub::IpFamilyType::IPV6) {
            ipFamily = "IPV6";
        } else if (ipFamilyType == ::dataStub::IpFamilyType::IPV4V6) {
            ipFamily = "IPV4V6";
        } else {
            ipFamily = "Unknown";
        }
        LOG(DEBUG, __FUNCTION__, " ipFamily is :",  ipFamily);
        return ipFamily;
    }

    static std::string convertAuthProtocolEnumToString(
            ::dataStub::AuthProtocolType::AuthProto authProtocolType) {
        LOG(DEBUG, __FUNCTION__);

        std::string authProtocol;
        if (authProtocolType == ::dataStub::AuthProtocolType::AUTH_PAP) {
            authProtocol = "AUTH_PAP";
        } else if (authProtocolType == ::dataStub::AuthProtocolType::AUTH_CHAP) {
            authProtocol = "AUTH_CHAP";
        } else if (authProtocolType == ::dataStub::AuthProtocolType::AUTH_PAP_CHAP) {
            authProtocol = "AUTH_PAP_CHAP";
        } else {
            authProtocol = "AUTH_NONE";
        }
        LOG(DEBUG, __FUNCTION__, " authProtocol is :",  authProtocol);
        return authProtocol;
    }

    static bool isValidIpv4Address(const std::string &addr) {
        struct sockaddr_in sa;
        int res = inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr));
        return res != 0;
    }

    static bool isValidIpv6Address(const std::string &addr) {
        struct sockaddr_in6 sa;
        int res = inet_pton(AF_INET6, addr.c_str(), &(sa.sin6_addr));
        return res != 0;
    }

    static std::string protocolToString(telux::data::IpProtocol proto) {
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
            case 58:
                return "ICMP6";
            case 253:
                return "PROTO_TCP_UDP";
            default: {
                return "Unknown";
            }
        }
    }

    static uint8_t stringToProtocol(std::string proto) {
        if (proto == "ICMP") {
            return 1;
        } else if (proto == "IGMP") {
            return 2;
        } else if (proto == "TCP") {
            return 6;
        } else if (proto == "UDP") {
            return 17;
        } else if (proto == "ESP") {
            return 50;
        } else if (proto == "ICMP6") {
            return 58;
        } else if (proto == "PROTO_TCP_UDP") {
            return 253;
        }
        return 0;
    }

    static std::string convertEnumToBackhaulPrefString(
        ::dataStub::BackhaulPreference pref) {

        switch (pref) {
            case ::dataStub::BackhaulPreference::PREF_ETH:
                return "ETH";
            case ::dataStub::BackhaulPreference::PREF_USB:
                return "USB";
            case ::dataStub::BackhaulPreference::PREF_WLAN:
                return "WLAN";
            case ::dataStub::BackhaulPreference::PREF_WWAN:
                return "WWAN";
            case ::dataStub::BackhaulPreference::PREF_BLE:
                return "BLE";
            case ::dataStub::BackhaulPreference::INVALID:
            default:
                return "INVALID";
        }
        return "INVALID";
    }

    static std::string convertEnumToInterfaceTypeString(
        ::dataStub::InterfaceType ifType) {

        switch (ifType) {
            case ::dataStub::InterfaceType::WLAN:
                return "WLAN";
            case ::dataStub::InterfaceType::ETH:
                return "ETH";
            case ::dataStub::InterfaceType::ECM:
                return "ECM";
            case ::dataStub::InterfaceType::RNDIS:
                return "RNDIS";
            case ::dataStub::InterfaceType::MHI:
                return "MHI";
            case ::dataStub::InterfaceType::ETH2:
                return "ETH2";
            case ::dataStub::InterfaceType::AP_PRIMARY:
                return "AP_PRIMARY";
            case ::dataStub::InterfaceType::AP_SECONDARY:
                return "AP_SECONDARY";
            case ::dataStub::InterfaceType::AP_TERTIARY:
                return "AP_TERTIARY";
            case ::dataStub::InterfaceType::AP_QUATERNARY:
                return "AP_QUATERNARY";
            case ::dataStub::InterfaceType::VMTAP0:
                return "VMTAP0";
            case ::dataStub::InterfaceType::VMTAP1:
                return "VMTAP1";
            case ::dataStub::InterfaceType::UNKNOWN:
            default:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static telux::data::InterfaceType convertInterfaceTypeToStruct(
        ::dataStub::InterfaceType ifType) {

        switch (ifType) {
            case ::dataStub::InterfaceType::WLAN:
                return telux::data::InterfaceType::WLAN;
            case ::dataStub::InterfaceType::ETH:
                return telux::data::InterfaceType::ETH;
            case ::dataStub::InterfaceType::ECM:
                return telux::data::InterfaceType::ECM;
            case ::dataStub::InterfaceType::RNDIS:
                return telux::data::InterfaceType::RNDIS;
            case ::dataStub::InterfaceType::MHI:
                return telux::data::InterfaceType::MHI;
            case ::dataStub::InterfaceType::ETH2:
                return telux::data::InterfaceType::ETH2;
            case ::dataStub::InterfaceType::AP_PRIMARY:
                return telux::data::InterfaceType::AP_PRIMARY;
            case ::dataStub::InterfaceType::AP_SECONDARY:
                return telux::data::InterfaceType::AP_SECONDARY;
            case ::dataStub::InterfaceType::AP_TERTIARY:
                return telux::data::InterfaceType::AP_TERTIARY;
            case ::dataStub::InterfaceType::AP_QUATERNARY:
                return telux::data::InterfaceType::AP_QUATERNARY;
            case ::dataStub::InterfaceType::VMTAP0:
                return telux::data::InterfaceType::VMTAP0;
            case ::dataStub::InterfaceType::VMTAP1:
                return telux::data::InterfaceType::VMTAP1;
            case ::dataStub::InterfaceType::UNKNOWN:
            default:
                return telux::data::InterfaceType::UNKNOWN;
        }
        return telux::data::InterfaceType::UNKNOWN;
    }

    static ::dataStub::InterfaceType convertInterfaceTypeToGrpc(
            telux::data::InterfaceType ifType) {

        if (ifType == telux::data::InterfaceType::WLAN) {
            return ::dataStub::InterfaceType::WLAN;
        } else if (ifType == telux::data::InterfaceType::ETH) {
            return ::dataStub::InterfaceType::ETH;
        } else if (ifType == telux::data::InterfaceType::ECM) {
            return ::dataStub::InterfaceType::ECM;
        } else if (ifType == telux::data::InterfaceType::RNDIS) {
            return ::dataStub::InterfaceType::RNDIS;
        } else if (ifType == telux::data::InterfaceType::MHI) {
            return ::dataStub::InterfaceType::MHI;
        } else if (ifType == telux::data::InterfaceType::ETH2) {
            return ::dataStub::InterfaceType::ETH2;
        } else if (ifType == telux::data::InterfaceType::AP_PRIMARY) {
            return ::dataStub::InterfaceType::AP_PRIMARY;
        } else if (ifType == telux::data::InterfaceType::AP_SECONDARY) {
            return ::dataStub::InterfaceType::AP_SECONDARY;
        } else if (ifType == telux::data::InterfaceType::AP_TERTIARY) {
            return ::dataStub::InterfaceType::AP_TERTIARY;
        } else if (ifType == telux::data::InterfaceType::AP_QUATERNARY) {
            return ::dataStub::InterfaceType::AP_QUATERNARY;
        } else if (ifType == telux::data::InterfaceType::VMTAP0) {
            return ::dataStub::InterfaceType::VMTAP0;
        } else if (ifType == telux::data::InterfaceType::VMTAP1) {
            return ::dataStub::InterfaceType::VMTAP1;
        } else {
            return ::dataStub::InterfaceType::UNKNOWN;
        }
    }

    static ::dataStub::InterfaceType convertInterfaceTypeStringToEnum(
        std::string ifType) {

            if (ifType == "WLAN") {
                return ::dataStub::InterfaceType::WLAN;
            } else if (ifType == "ETH") {
                return ::dataStub::InterfaceType::ETH;
            } else if (ifType == "ECM") {
                return ::dataStub::InterfaceType::ECM;
            } else if (ifType == "RNDIS") {
                return ::dataStub::InterfaceType::RNDIS;
            } else if (ifType == "MHI") {
                return ::dataStub::InterfaceType::MHI;
            } else if (ifType == "ETH2") {
                return ::dataStub::InterfaceType::ETH2;
            } else if (ifType == "AP_PRIMARY") {
                return ::dataStub::InterfaceType::AP_PRIMARY;
            } else if (ifType == "AP_SECONDARY") {
                return ::dataStub::InterfaceType::AP_SECONDARY;
            } else if (ifType == "AP_TERTIARY") {
                return ::dataStub::InterfaceType::AP_TERTIARY;
            } else if (ifType == "AP_QUATERNARY") {
                return ::dataStub::InterfaceType::AP_QUATERNARY;
            } else if (ifType == "VMTAP0") {
                return ::dataStub::InterfaceType::VMTAP0;
            } else if (ifType == "VMTAP1") {
                return ::dataStub::InterfaceType::VMTAP1;
            } else {
                return ::dataStub::InterfaceType::UNKNOWN;
            }
    }

    static telux::data::IpFamilyType convertIpFamilyToStruct(::dataStub::IpFamilyType ipFamily) {
        auto type = ipFamily.ip_family_type();
        switch (type) {
            case ::dataStub::IpFamilyType_Type_IPV4:
                return telux::data::IpFamilyType::IPV4;
            case ::dataStub::IpFamilyType_Type_IPV6:
                return telux::data::IpFamilyType::IPV6;
            case ::dataStub::IpFamilyType_Type_IPV4V6:
                return telux::data::IpFamilyType::IPV4V6;
            default:
               return telux::data::IpFamilyType::UNKNOWN;
        }
        return telux::data::IpFamilyType::UNKNOWN;
    }

    static ::dataStub::IpFamilyType_Type convertIpFamilyTypeToGrpc(
            telux::data::IpFamilyType ipFamilyType) {
        switch (ipFamilyType) {
            case telux::data::IpFamilyType::IPV4:
                return ::dataStub::IpFamilyType_Type_IPV4;
            case telux::data::IpFamilyType::IPV6:
                return ::dataStub::IpFamilyType_Type_IPV6;
            case telux::data::IpFamilyType::IPV4V6:
                return ::dataStub::IpFamilyType_Type_IPV4V6;
            default:
                return ::dataStub::IpFamilyType_Type_UNKNOWN;
        }
        return ::dataStub::IpFamilyType_Type_UNKNOWN;
    }

    static telux::data::IpAssignType convertIpTypeToStruct(::dataStub::IpType ipType) {
        auto type = ipType.ip_type();
        switch (type) {
            case ::dataStub::IpType_IpAssignType_STATIC_IP:
                return telux::data::IpAssignType::STATIC_IP;
            case ::dataStub::IpType_IpAssignType_DYNAMIC_IP:
                return telux::data::IpAssignType::DYNAMIC_IP;
            default:
               return telux::data::IpAssignType::UNKNOWN;
        }
        return telux::data::IpAssignType::UNKNOWN;
    }

    static ::dataStub::IpType_IpAssignType convertIpTypeToGrpc(telux::data::IpAssignType ipType) {
        switch (ipType) {
            case telux::data::IpAssignType::STATIC_IP:
                return ::dataStub::IpType_IpAssignType_STATIC_IP;
            case telux::data::IpAssignType::DYNAMIC_IP:
                return ::dataStub::IpType_IpAssignType_DYNAMIC_IP;
            default:
                return ::dataStub::IpType_IpAssignType_UNKNOWN;
        }
        return ::dataStub::IpType_IpAssignType_UNKNOWN;
    }

    static telux::data::IpAssignOperation convertIpAssignToStruct(::dataStub::IpAssign ipAssign) {
        auto opr = ipAssign.ip_assign();
        switch (opr) {
            case ::dataStub::IpAssign_IpAssignOperation_DISABLE:
                return telux::data::IpAssignOperation::DISABLE;
            case ::dataStub::IpAssign_IpAssignOperation_ENABLE:
                return telux::data::IpAssignOperation::ENABLE;
            case ::dataStub::IpAssign_IpAssignOperation_RECONFIGURE:
                return telux::data::IpAssignOperation::RECONFIGURE;
            default:
               return telux::data::IpAssignOperation::UNKNOWN;
        }
        return telux::data::IpAssignOperation::UNKNOWN;
    }

    static ::dataStub::IpAssign_IpAssignOperation convertIpAssignToGrpc(
            telux::data::IpAssignOperation ipAssign) {
        switch (ipAssign) {
            case telux::data::IpAssignOperation::DISABLE:
                return ::dataStub::IpAssign_IpAssignOperation_DISABLE;
            case telux::data::IpAssignOperation::ENABLE:
                return ::dataStub::IpAssign_IpAssignOperation_ENABLE;
            case telux::data::IpAssignOperation::RECONFIGURE:
                return ::dataStub::IpAssign_IpAssignOperation_RECONFIGURE;
            default:
                return ::dataStub::IpAssign_IpAssignOperation_UNKNOWN;
        }
        return ::dataStub::IpAssign_IpAssignOperation_UNKNOWN;
    }

    static void convertIpAddrInfoToStruct(const ::dataStub::IpAddrInfo ipAddrInfoGrpc,
            telux::data::IpAddrInfo &ipAddrInfoStruct) {
        ipAddrInfoStruct.ifAddress                 = ipAddrInfoGrpc.if_address();
        try {
            ipAddrInfoStruct.ifMask                 = std::stol(ipAddrInfoGrpc.if_mask());
        } catch (std::invalid_argument const &e) {
            LOG(ERROR, __FUNCTION__, " Invalid ifmask");
            ipAddrInfoStruct.ifMask = 0;
        }
        ipAddrInfoStruct.gwAddress              = ipAddrInfoGrpc.gw_address();
        ipAddrInfoStruct.primaryDnsAddress      = ipAddrInfoGrpc.primary_dns_address();
        ipAddrInfoStruct.secondaryDnsAddress    = ipAddrInfoGrpc.secondary_dns_address();
    }

    static void convertIpAddrInfoToGrpc(const telux::data::IpAddrInfo &ipAddrInfoStruct,
            ::dataStub::IpAddrInfo *&ipAddrInfoGrpc) {
        ipAddrInfoGrpc->set_if_address(ipAddrInfoStruct.ifAddress);
        ipAddrInfoGrpc->set_if_mask(std::to_string(ipAddrInfoStruct.ifMask));
        ipAddrInfoGrpc->set_gw_address(ipAddrInfoStruct.gwAddress);
        ipAddrInfoGrpc->set_primary_dns_address(ipAddrInfoStruct.primaryDnsAddress);
        ipAddrInfoGrpc->set_secondary_dns_address(ipAddrInfoStruct.secondaryDnsAddress);
    }

    static std::string convertEnumToIpptOprString(
        ::dataStub::IpptOperation ipptOpr) {

        switch (ipptOpr.ippt_opr()) {
            case ::dataStub::IpptOperation_Operation_DISABLE:
                return "DISABLE";
            case ::dataStub::IpptOperation_Operation_ENABLE:
                return "ENABLE";
            case ::dataStub::IpptOperation_Operation_UNKNOWN:
                return "UNKNOWN";
            default:
                return "UNKNOWN";
        }
    }

    static ::dataStub::IpptOperation_Operation convertIpptOprStringToEnum(
        std::string ipptOpr) {

        if (ipptOpr == "ENABLE") {
            return ::dataStub::IpptOperation_Operation_ENABLE;
        } else if (ipptOpr == "DISABLE") {
            return ::dataStub::IpptOperation_Operation_DISABLE;
        } else {
            return ::dataStub::IpptOperation_Operation_UNKNOWN;
        }
    }

    static ::dataStub::IpptOperation_Operation convertIpptOprToGrpc(
        telux::data::Operation ipptOpr) {

        if (ipptOpr == telux::data::Operation::ENABLE) {
            return ::dataStub::IpptOperation_Operation_ENABLE;
        } else if (ipptOpr == telux::data::Operation::DISABLE) {
            return ::dataStub::IpptOperation_Operation_DISABLE;
        } else {
            return ::dataStub::IpptOperation_Operation_UNKNOWN;
        }
    }

    static telux::data::Operation convertIpptOprToStruct(
            const ::dataStub::IpptOperation ipptOpr) {
        auto opr = ipptOpr.ippt_opr();

        if (opr == ::dataStub::IpptOperation_Operation_ENABLE) {
            return telux::data::Operation::ENABLE;
        } else if (opr == ::dataStub::IpptOperation_Operation_DISABLE) {
            return telux::data::Operation::DISABLE;
        } else {
            return telux::data::Operation::UNKNOWN;
        }
    }

    static ::dataStub::BackhaulPreference convertBackhaulPrefStringToEnum(
        std::string backhaul) {
        if (backhaul == "ETH") {
            return ::dataStub::BackhaulPreference::PREF_ETH;
        } else if (backhaul == "USB") {
            return ::dataStub::BackhaulPreference::PREF_USB;
        } else if (backhaul == "WLAN") {
            return ::dataStub::BackhaulPreference::PREF_WLAN;
        } else if (backhaul == "WWAN") {
            return ::dataStub::BackhaulPreference::PREF_WWAN;
        } else if (backhaul == "BLE") {
            return ::dataStub::BackhaulPreference::PREF_BLE;
        }

        return ::dataStub::BackhaulPreference::INVALID;
    }

    static ::dataStub::L2tpProtocol stringToL2tpProtocol(std::string proto) {
        if (proto == "IP") {
            return ::dataStub::L2tpProtocol::IP;
        } else if (proto == "UDP") {
            return ::dataStub::L2tpProtocol::UDP;
        }
        return ::dataStub::L2tpProtocol::NONE;
    }

    static std::string l2tpProtocolToString(::dataStub::L2tpProtocol proto) {
        switch (proto) {
            case ::dataStub::L2tpProtocol::IP:
                return "IP";
            case ::dataStub::L2tpProtocol::UDP:
                return "UDP";
            default :
                return "NONE";
        }
    }

    static telux::data::NetworkType convertNetworkTypeToEnum(const ::dataStub::Network
            &nwType) {

        switch(nwType.nw_type()) {
            case ::dataStub::Network_NetworkType_LAN:
                return telux::data::NetworkType::LAN;
            case ::dataStub::Network_NetworkType_WAN:
                return telux::data::NetworkType::WAN;
            case ::dataStub::Network_NetworkType_UNKNOWN:
            default:
                return telux::data::NetworkType::UNKNOWN;
        }
        return telux::data::NetworkType::UNKNOWN;
    }

    static ::dataStub::Network_NetworkType convertNetworkTypeToGrpc(const telux::data::NetworkType
            &nwType) {

        switch(nwType) {
            case telux::data::NetworkType::LAN:
                return ::dataStub::Network_NetworkType_LAN;
            case telux::data::NetworkType::WAN:
                return ::dataStub::Network_NetworkType_WAN;
            case telux::data::NetworkType::UNKNOWN:
            default:
                return ::dataStub::Network_NetworkType_UNKNOWN;
        }
        return ::dataStub::Network_NetworkType_UNKNOWN;
    }

    static std::string convertNetworkTypeToString(const ::dataStub::Network &nwType) {
        switch(nwType.nw_type()) {
            case ::dataStub::Network_NetworkType_LAN:
                return "LAN";
            case ::dataStub::Network_NetworkType_WAN:
                return "WAN";
            case ::dataStub::Network_NetworkType_UNKNOWN:
            default:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static ::dataStub::Network_NetworkType convertNetworkTypeToGrpc(const std::string nwType) {

        if (nwType == "LAN") {
                return ::dataStub::Network_NetworkType_LAN;
        } else if (nwType == "WAN") {
                return ::dataStub::Network_NetworkType_WAN;
        } else {
                return ::dataStub::Network_NetworkType_UNKNOWN;
        }
    }
};
