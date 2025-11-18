/*
 * Copyright (c) 2022-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       WlanUtils.cpp
 *
 * @brief      This class class performs common functions in Wlan.
 */

#include <iostream>
#include "WlanUtils.hpp"

std::string WlanUtils::getWlanDeviceName(telux::wlan::HwDeviceType device) {
   std::string retStr;

   switch(device) {
      case telux::wlan::HwDeviceType::UNKNOWN:
         retStr = "UNKNOWN";
         break;
      case telux::wlan::HwDeviceType::QCA6574:
         retStr = "QCA6574";
         break;
      case telux::wlan::HwDeviceType::QCA6696:
         retStr = "QCA6696";
         break;
      case telux::wlan::HwDeviceType::QCA6595:
         retStr = "QCA6595";
         break;
      default:
         retStr = "CUSTOM";
         break;
   }
   return retStr;
}

std::string WlanUtils::getWlanApType(telux::wlan::ApType apType) {
   std::string retStr;
   switch(apType) {
      case telux::wlan::ApType::UNKNOWN:
         retStr = "UNKNOWN";
         break;
      case telux::wlan::ApType::PRIVATE:
         retStr = "PRIVATE AP";
         break;
      case telux::wlan::ApType::GUEST:
         retStr = "GUEST AP";
         break;
      default:
         break;
   }
   return retStr;
}

std::string WlanUtils::getWlanId(telux::wlan::Id id) {
   std::string retStr;
   switch(id) {
      case telux::wlan::Id::PRIMARY:
         retStr = "PRIMARY";
         break;
      case telux::wlan::Id::SECONDARY:
         retStr = "SECONDARY";
         break;
      case telux::wlan::Id::TERTIARY:
         retStr = "TERTIARY";
         break;
      case telux::wlan::Id::QUATERNARY:
         retStr = "QUATERNARY";
         break;
   }
   return retStr;
}

std::string WlanUtils::getStaConnectionStatus(telux::wlan::StaInterfaceStatus status) {
   std::string retStr = "";
   switch(status) {
      case telux::wlan::StaInterfaceStatus::UNKNOWN:
         retStr = "UNKNOWN";
         break;
      case telux::wlan::StaInterfaceStatus::CONNECTING:
         retStr = "CONNECTING";
         break;
      case telux::wlan::StaInterfaceStatus::CONNECTED:
         retStr = "CONNECTED";
         break;
      case telux::wlan::StaInterfaceStatus::DISCONNECTED:
         retStr = "DISCONNECTED";
         break;
      case telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED:
         retStr = "ASSOCIATION_FAILED";
         break;
      case telux::wlan::StaInterfaceStatus::IP_ASSIGNMENT_FAILED:
         retStr = "IP_ASSIGNMENT_FAILED";
         break;
      default:
         break;
   }
   return retStr;
}

std::string WlanUtils::apAccessToString(telux::wlan::ApInterworking interworking) {
   std::string retString = "";
   switch(interworking) {
      case telux::wlan::ApInterworking::INTERNET_ACCESS:
         retString = "INTERNET ACCESS";
         break;
      case telux::wlan::ApInterworking::FULL_ACCESS:
         retString = "FULL ACCESS";
         break;
      default:
         break;
   }
   return retString;
}

std::string WlanUtils::apRadioTypeToString(telux::wlan::BandType radio) {
   std::string retString = "";
   switch(radio) {
      case telux::wlan::BandType::BAND_5GHZ:
         retString = "5 GHZ";
         break;
      case telux::wlan::BandType::BAND_2GHZ:
         retString = "2.4 GHZ";
         break;
      case telux::wlan::BandType::BAND_6GHZ:
         retString = "6 GHZ";
         break;
      default:
         break;
   }
   return retString;
}

std::string WlanUtils::apSecurityModeToString(telux::wlan::SecMode mode) {
   std::string retString = "";
   switch(mode) {
      case telux::wlan::SecMode::OPEN:
         retString = "OPEN";
         break;
      case telux::wlan::SecMode::WEP:
         retString = "WEP";
         break;
      case telux::wlan::SecMode::WPA:
         retString = "WPA";
         break;
      case telux::wlan::SecMode::WPA2:
         retString = "WPA2";
         break;
      case telux::wlan::SecMode::WPA3:
         retString = "WPA3";
         break;
      default:
         break;
   }
   return retString;
}

std::string WlanUtils::apSecurityAuthToString(telux::wlan::SecAuth auth) {
   std::string retString = "";
   switch(auth) {
      case telux::wlan::SecAuth::NONE:
         retString = "NONE";
         break;
      case telux::wlan::SecAuth::PSK:
         retString = "PSK";
         break;
      case telux::wlan::SecAuth::EAP_SIM:
         retString = "EAP SIM";
         break;
      case telux::wlan::SecAuth::EAP_AKA:
         retString = "EAP AKA";
         break;
      case telux::wlan::SecAuth::EAP_LEAP:
         retString = "EAP LEAP";
         break;
      case telux::wlan::SecAuth::EAP_TLS:
         retString = "EAP TLS";
         break;
      case telux::wlan::SecAuth::EAP_TTLS:
         retString = "EAP TTLS";
         break;
      case telux::wlan::SecAuth::EAP_PEAP:
         retString = "EAP PEAP";
         break;
      case telux::wlan::SecAuth::EAP_FAST:
         retString = "EAP FAST";
         break;
      case telux::wlan::SecAuth::EAP_PSK:
         retString = "EAP PSK";
         break;
      case telux::wlan::SecAuth::SAE:
         retString = "SAE";
         break;
      default:
         break;
   }
   return retString;
}

std::string WlanUtils::apSecurityEncryptToString(telux::wlan::SecEncrypt encrypt) {
   std::string retString = "";
   switch(encrypt) {
      case telux::wlan::SecEncrypt::RC4:
         retString = "RC4";
         break;
      case telux::wlan::SecEncrypt::TKIP:
         retString = "TKIP";
         break;
      case telux::wlan::SecEncrypt::AES:
         retString = "AES";
         break;
      case telux::wlan::SecEncrypt::GCMP:
         retString = "GCMP";
         break;
      default:
         break;
   }
   return retString;
}

telux::wlan::Id WlanUtils::convertIntToWlanId(int id) {
   telux::wlan::Id retId = telux::wlan::Id::PRIMARY;
   switch(id) {
      case 1:
         retId = telux::wlan::Id::PRIMARY;
         break;
      case 2:
         retId = telux::wlan::Id::SECONDARY;
         break;
      case 3:
         retId = telux::wlan::Id::TERTIARY;
         break;
      case 4:
         retId = telux::wlan::Id::QUATERNARY;
         break;
      default:
         break;
   }
   return retId;
}

telux::wlan::ApType WlanUtils::convertIntToApType(int type) {
   telux::wlan::ApType retType = telux::wlan::ApType::UNKNOWN;
   switch(type) {
      case 0:
         retType = telux::wlan::ApType::UNKNOWN;
         break;
      case 1:
         retType = telux::wlan::ApType::PRIVATE;
         break;
      case 2:
         retType = telux::wlan::ApType::GUEST;
         break;
      default:
         break;
   }
   return retType;
}

telux::wlan::ApInterworking WlanUtils::convertIntToInterworking(int interworking) {
   telux::wlan::ApInterworking retInterworking = telux::wlan::ApInterworking::INTERNET_ACCESS;
   switch(interworking) {
      case 0:
         retInterworking = telux::wlan::ApInterworking::INTERNET_ACCESS;
         break;
      case 1:
         retInterworking = telux::wlan::ApInterworking::FULL_ACCESS;
         break;
      default:
         break;
   }
   return retInterworking;
}

telux::wlan::SecMode WlanUtils::convertIntToSecMode(int mode) {
   telux::wlan::SecMode retMode = telux::wlan::SecMode::OPEN;
   switch(mode) {
      case 0:
         retMode = telux::wlan::SecMode::OPEN;
         break;
      case 1:
         retMode = telux::wlan::SecMode::WEP;
         break;
      case 2:
         retMode = telux::wlan::SecMode::WPA;
         break;
      case 3:
         retMode = telux::wlan::SecMode::WPA2;
         break;
      case 4:
         retMode = telux::wlan::SecMode::WPA3;
         break;
      default:
         break;
   }
   return retMode;
}

telux::wlan::SecAuth WlanUtils::convertIntToSecAuth(int auth) {
   telux::wlan::SecAuth retAuth = telux::wlan::SecAuth::NONE;
   switch(auth) {
      case 0:
         retAuth = telux::wlan::SecAuth::NONE;
         break;
      case 1:
         retAuth = telux::wlan::SecAuth::PSK;
         break;
      case 2:
         retAuth = telux::wlan::SecAuth::EAP_SIM;
         break;
      case 3:
         retAuth = telux::wlan::SecAuth::EAP_AKA;
         break;
      case 4:
         retAuth = telux::wlan::SecAuth::EAP_LEAP;
         break;
      case 5:
         retAuth = telux::wlan::SecAuth::EAP_TLS;
         break;
      case 6:
         retAuth = telux::wlan::SecAuth::EAP_TTLS;
         break;
      case 7:
         retAuth = telux::wlan::SecAuth::EAP_PEAP;
         break;
      case 8:
         retAuth = telux::wlan::SecAuth::EAP_FAST;
         break;
      case 9:
         retAuth = telux::wlan::SecAuth::EAP_PSK;
         break;
      case 10:
         retAuth = telux::wlan::SecAuth::SAE;
         break;
      default:
         break;
   }
   return retAuth;
}

telux::wlan::SecEncrypt WlanUtils::convertIntToSecEncrypt(int encrypt) {
   telux::wlan::SecEncrypt retEncrypt = telux::wlan::SecEncrypt::RC4;
   switch(encrypt) {
      case 0:
         retEncrypt = telux::wlan::SecEncrypt::RC4;
         break;
      case 1:
         retEncrypt = telux::wlan::SecEncrypt::TKIP;
         break;
      case 2:
         retEncrypt = telux::wlan::SecEncrypt::AES;
         break;
      case 3:
         retEncrypt = telux::wlan::SecEncrypt::GCMP;
         break;
      default:
         break;
   }
   return retEncrypt;
}

void WlanUtils::printAPStatus(std::vector<telux::wlan::ApStatus>& apStatus) {
   if(apStatus.size() > 0) {
       std::cout << "List of APs:" << std::endl;
       for(auto& ap:apStatus) {
           std::cout << "--------------------------------------------" << std::endl;
           std::cout << "Id                 : " << WlanUtils::getWlanId(ap.id) << std::endl;
           std::cout << "Network Interface  : " << ap.name << std::endl;
           std::cout << "IPv4 Addr          : " << ap.ipv4Address << std::endl;
           std::cout << "MAC Addr           : " << ap.macAddress << std::endl;
           for(auto& netInfo:ap.network) {
               std::cout << "SSID               : " << netInfo.ssid << std::endl;
               std::cout << "Radio Type         : "
                  << apRadioTypeToString(netInfo.info.apRadio) << std::endl;
               std::cout << "AP Type            : "
                  << WlanUtils::getWlanApType(netInfo.info.apType) << std::endl;
           }
           std::cout << std::endl;
       }
   } else {
       std::cout << "No AP is currently active" << std::endl;
   }
}

void WlanUtils::printStaStatus(std::vector<telux::wlan::StaStatus>& staStatus) {
   if(staStatus.size() > 0) {
       std::cout << "List of Stations:" << std::endl;
       for(auto& sta:staStatus) {
           std::cout << "--------------------------------------------" << std::endl;
           std::cout << "Id                : " << WlanUtils::getWlanId(sta.id) << std::endl;
           std::cout << "Network Interface : " << sta.name << std::endl;
           std::cout << "IPv4 Addr         : " << sta.ipv4Address << std::endl;
           std::cout << "IPv6 Addr         : " << sta.ipv6Address << std::endl;
           std::cout << "MAC Addr          : " << sta.macAddress  << std::endl;
           std::cout << "Status            : "
                     << WlanUtils::getStaConnectionStatus(sta.status) << std::endl;
       }
       std::cout << std::endl;
   } else {
       std::cout << "No Station is currently active" << std::endl;
   }
}

void WlanUtils::printDeviceInfo(std::vector<telux::wlan::DeviceInfo>& info) {
   if(info.size() > 0) {
       std::cout << "List of connected devices:" << std::endl;
       for(auto& dev:info) {
           std::cout << "----------------------------------------------" << std::endl;
           std::cout << "Associated AP          : " << WlanUtils::getWlanId(dev.id) << std::endl;
           std::cout << "Device Name            : " << dev.name << std::endl;
           std::cout << "Device IPv4 Address    : " << dev.ipv4Address << std::endl;
           for(const auto& ipv6:dev.ipv6Address) {
              std::cout << "Device IPv6 Address    : " << ipv6 << std::endl;
           }
           std::cout << "Device MAC Address     : " << dev.macAddress << std::endl;
       }
    } else {
        std::cout << "No Devices are currently connected to any AP" << std::endl;
    }
}

void WlanUtils::printApElementInfo(telux::wlan::ApElementInfoConfig ElementInfoConfig) {
   std::cout << "AP Element Info: \n";
   std::cout << "    Enabled             : "
             << ((ElementInfoConfig.isEnabled)? "Yes":"No") << std::endl;
   std::cout << "    Interworking Enabled: "
             << ((ElementInfoConfig.isInterworkingEnabled)? "Yes":"No") << std::endl;
   std::cout << "    Network Access Type: " << WlanUtils::apElementInfoAccessTypeToString(
            ElementInfoConfig.netAccessType) << std::endl;
   std::cout << "    Connectivity to Internet: "
             << ((ElementInfoConfig.internet)? "Yes":"No") << std::endl;
   std::cout << "    Additional step required for access: "
             << ((ElementInfoConfig.asra)? "Yes":"No") << std::endl;
   std::cout << "    Emergency services reachable: "
             << ((ElementInfoConfig.esr)? "Yes":"No") << std::endl;
   std::cout << "    Unauthenticated emergency service accessible: "
             << ((ElementInfoConfig.uesa)? "Yes":"No") << std::endl;
   std::cout << "    Venue group: " << ElementInfoConfig.venueGroup << std::endl;
   std::cout << "    Venue type: " << ElementInfoConfig.venueType << std::endl;
   std::cout << "    Homogeneous ESS identifier: " << ElementInfoConfig.hessid << std::endl;
   std::cout << "    Vendor elements for Beacon and Probe Response frames: "
             << ElementInfoConfig.vendorElements << std::endl;
   std::cout << "    Vendor elements for (Re)Association Response frames: "
             << ElementInfoConfig.assocRespElements << std::endl;
}

std::string WlanUtils::apElementInfoAccessTypeToString(telux::wlan::NetAccessType accessType) {
   std::string retString = "";
   switch(accessType) {
      case telux::wlan::NetAccessType::PRIVATE:
         retString = "PRIVATE";
         break;
      case telux::wlan::NetAccessType::PRIVATE_WITH_GUEST:
         retString = "PRIVATE WITH GUEST";
         break;
      case telux::wlan::NetAccessType::CHARGEABLE_PUBLIC:
         retString = "CHARGEABLE PUBLIC";
         break;
      case telux::wlan::NetAccessType::FREE_PUBLIC:
         retString = "FREE PUBLIC";
         break;
      case telux::wlan::NetAccessType::PERSONAL_DEVICE:
         retString = "PERSONAL DEVICE";
         break;
      case telux::wlan::NetAccessType::EMERGENCY_SERVICES_ONLY:
         retString = "EMERGENCY SERVICES ONLY";
         break;
      case telux::wlan::NetAccessType::TEST_OR_EXPERIMENTAL:
         retString = "TEST OR EXPERIMENTAL";
         break;
      case telux::wlan::NetAccessType::WILDCARD:
         retString = "WILDCARD";
         break;
      default:
         break;
   }
   return retString;
}
