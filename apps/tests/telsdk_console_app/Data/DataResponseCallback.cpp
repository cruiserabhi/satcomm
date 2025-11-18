/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

 *  Copyright (c) 2021-2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <iostream>
#include <iomanip>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "DataResponseCallback.hpp"
#include "DataMenu.hpp"
#include "DataUtils.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyDataProfilesCallback::onProfileListResponse(
   const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
   telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << std::endl << std::endl;
      PRINT_CB << " ** onProfileListResponse **" << std::endl;
      std::cout << std::setw(2)
                << "+------------------------------------------------------------------------------"
                << "------------------------+"
                << std::endl;
      std::cout << std::setw(14) << "| Profile # | " << std::setw(11) << "TechPref | "
                << std::setw(15) << "      APN      " << std::setw(17) << "|  ProfileName  |"
                << std::setw(10) << " IP Type |" << std::setw(16) << "    APN Type    |"
                << std::setw(20) << " Emergency Allowed|"
                << std::endl;
      std::cout << std::setw(2)
                << "+------------------------------------------------------------------------------"
                << "------------------------+"
                << std::endl;
      for(auto it : profiles) {
         std::cout << std::left << std::setw(4) << "  " << std::setw(10) << it->getId()
                   << std::setw(11) << DataUtils::techPreferenceToString(it->getTechPreference())
                   << std::setw(15) << it->getApn() << std::setw(17) << it->getName()
                   << std::setw(10) << DataUtils::ipFamilyTypeToString(it->getIpFamilyType())
                   << std::setw(16) << it->getApnTypes().to_string()
                   << std::setw(2) << " " << std::setw(20)
                   << DataUtils::emergencyAllowedTypeToString(it->getIsEmergencyAllowed())
                   << std::endl;
      }
      std::cout << std::endl << std::endl;
   } else {
      std::cout << "ProfileList response failed, ErrorCode:" << (int)error
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyDataProfileCallback::onResponse(const std::shared_ptr<telux::data::DataProfile> &profile,
                                       telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << std::endl << std::endl;
      PRINT_CB << "onProfileResponse:" << std::endl;
      PRINT_CB
         << "ProfileID : " << profile->getId() << ", ProfileName : " << profile->getName()
         << ", TechPreference : " << DataUtils::techPreferenceToString(profile->getTechPreference())
         << ", APN : " << profile->getApn() << ", UserName : " << profile->getUserName()
         << ", Password : " << profile->getPassword()
         << ", AuthPreference : " << (int)profile->getAuthProtocolType()
         << ", IpFamilyType : " << DataUtils::ipFamilyTypeToString(profile->getIpFamilyType())
         << ", EmergencyAllowed : "
         << DataUtils::emergencyAllowedTypeToString(profile->getIsEmergencyAllowed())
         << std::endl;
   } else {
      PRINT_CB << "Unable to create profile or request profile by ID, errorCode: "
               << static_cast<int>(error) << ", description: " << Utils::getErrorCodeAsString(error)
               << std::endl;
   }
   std::cout << std::endl << std::endl;
}

void MyDataCreateProfileCallback::onResponse(int profileId, telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << std::endl << std::endl;
      PRINT_CB << "onResponse:" << std::endl;
      PRINT_CB << "ProfileID : " << profileId << std::endl;
   } else {
      PRINT_CB << "Unable to create profile or request profile by ID, errorCode: "
               << static_cast<int>(error) << ", description: " << Utils::getErrorCodeAsString(error)
               << std::endl;
   }
   std::cout << std::endl << std::endl;
}

void MyDeleteProfileCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Delete Profile is successful " << std::endl;
   } else {
      PRINT_CB << " Delete Profile is failure, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyModifyProfileCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Modify Profile is successful " << std::endl;
   } else {
      PRINT_CB << " Modify Profile is failure, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

// Implementation of My Data callback
void MyDataCallResponseCallback::startDataCallResponseCallBack(
   const std::shared_ptr<telux::data::IDataCall> &dataCall, telux::common::ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      if (telux::data::DataCallStatus::NET_CONNECTED == dataCall->getDataCallStatus()) {
         PRINT_CB <<
            "start DataCallResponseCb is successful - NO_EFFECT, data call already connected"
            << std::endl;
      } else if (telux::data::DataCallStatus::NET_CONNECTING == dataCall->getDataCallStatus()){
         PRINT_CB << "start DataCallResponseCb is successful " << std::endl;
      }
   } else {
      PRINT_CB << "start DataCallResponseCb failed,  errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyDataCallResponseCallback::stopDataCallResponseCallBack(
   const std::shared_ptr<telux::data::IDataCall> &dataCall, telux::common::ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      if (telux::data::DataCallStatus::NET_CONNECTED == dataCall->getDataCallStatus()) {
         PRINT_CB <<
            "stop DataCallResponseCb is successful - DataCall remain active, still in use"
            << std::endl;
      } else if (telux::data::DataCallStatus::NET_NO_NET == dataCall->getDataCallStatus()){
         PRINT_CB << "stop DataCallResponseCb is successful - NO_EFFECT" << std::endl;
      } else if (telux::data::DataCallStatus::NET_DISCONNECTING == dataCall->getDataCallStatus()){
         PRINT_CB << "stop DataCallResponseCb is successful " << std::endl;
      }
   } else {
      PRINT_CB << "stop DataCallResponseCb failed,  errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
   std::cout << std::endl;
}

void DataCallStatisticsResponseCb::requestStatisticsResponse(
   const telux::data::DataCallStats dCallStats, telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "requestDataCallStatistics Response is successful \n";
      std::cout << " RX packets: " << dCallStats.packetsRx
                << " dropped: " << dCallStats.packetsDroppedRx << " bytes: " << dCallStats.bytesRx
                << std::endl;
      std::cout << " TX packets: " << dCallStats.packetsTx
                << " dropped: " << dCallStats.packetsDroppedRx << " bytes: " << dCallStats.bytesTx
                << std::endl;
   } else {
      PRINT_CB
         << "requestDataCallStatistics Response failed, errorCode: " << static_cast<int>(error)
         << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void DataCallStatisticsResponseCb::resetStatisticsResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   PRINT_CB << "resetDataCallStatistics Response"
            << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
            << ". ErrorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void MyDataCallResponseCallback::dataCallListResponseCb(
    const std::vector<std::shared_ptr<telux::data::IDataCall>> &dataCallList, telux::common::ErrorCode error) {
    std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << " ** Found "<<dataCallList.size()<<" DataCalls in the list **\n";
      for(auto dataCall:dataCallList) {
         std::cout << " SlotID: " << dataCall->getSlotId()
             << "\n ProfileID: " << dataCall->getProfileId()
             << "\n InterfaceName: " << dataCall->getInterfaceName()
             << "\n DataCallStatus: " << DataUtils::dataCallStatusToString(dataCall->getDataCallStatus())
             << "\n DataCallEndReason:\n   Type: "
             << DataUtils::callEndReasonTypeToString(dataCall->getDataCallEndReason().type)
             << ", Code: " << DataUtils::callEndReasonCode(dataCall->getDataCallEndReason()) << std::endl;
         std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
         for(auto &it : ipAddrList) {

            struct in_addr ifMaskAddr, gwMaskAddr;
            std::cout << "\n ifAddress: " << it.ifAddress << "\n gwAddress: " << it.gwAddress
                      << "\n primaryDnsAddress: " << it.primaryDnsAddress
                      << "\n secondaryDnsAddress: " << it.secondaryDnsAddress;

            if (it.ifMask) {
                ifMaskAddr.s_addr= it.ifMask;
                std::cout << "\n ifMask: " << inet_ntoa(ifMaskAddr);
            }
            if (it.gwMask) {
                gwMaskAddr.s_addr= it.gwMask;
                std::cout << "\n gwMask: " << inet_ntoa(gwMaskAddr);
            }
            std::cout << '\n';

         }
         std::cout << " IpFamilyType: " << DataUtils::ipFamilyTypeToString(dataCall->getIpFamilyType()) << '\n';
         std::cout << " TechPreference: " << DataUtils::techPreferenceToString(dataCall->getTechPreference())
                   << '\n';
         std::cout << " OperationType: " << DataUtils::operationTypeToString(dataCall->getOperationType())
                   << '\n';
         std::cout << " ----------------------------------------------------------\n\n";
      }
   } else {
      PRINT_CB << "requestDataCallList() failed,  errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyDataCallResponseCallback::requestThrottledApnInfoCb(
   const std::vector<telux::data::APNThrottleInfo> &throttleInfoList,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "requestThrottledApnInfo Response is successful \n";
      std::cout << " Number of throttled APN: " << throttleInfoList.size() << std::endl;
      int index = 0;
      for (auto throttleInfo: throttleInfoList) {
         std::cout << " index = " << ++index << std::endl << " Profile IDs = ";
         for (int profileId: throttleInfo.profileIds) {
            std::cout << profileId << ", ";
         }
         std::cout << std::endl << " APN: " << throttleInfo.apn << std::endl
                   << " ipv4Time (msec): " << throttleInfo.ipv4Time << std::endl
                   << " ipv6Time (msec): " << throttleInfo.ipv6Time << std::endl
                   << " isBlocked: " << (throttleInfo.isBlocked ? "True" : "False") << std::endl
                   << " mcc: " << throttleInfo.mcc << std::endl
                   << " mnc: " << throttleInfo.mnc << std::endl << std::endl;
      }
   } else {
      PRINT_CB
         << "requestThrottledApnInfo Response failed, errorCode: " << static_cast<int>(error)
         << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void DataFilterModeResponseCb::requestDataRestrictModeResponse(
    telux::data::DataRestrictMode mode, telux::common::ErrorCode error) {
  std::cout << std::endl << std::endl;
  if (error == telux::common::ErrorCode::SUCCESS) {
    PRINT_CB << "requestDataRestrictMode Response is successful \n";
    if (mode.filterMode == DataRestrictModeType::DISABLE) {
      std::cout << " DataRestrictMode Disabled" << std::endl;
    } else if (mode.filterMode == DataRestrictModeType::ENABLE) {
      std::cout << " DataRestrictMode Enabled" << std::endl;
    } else {
      std::cout << " Invalid DataRestrictMode" << std::endl;
    }
  } else {
    PRINT_CB << "requestDataRestrictMode Response failed, errorCode: "
             << static_cast<int>(error)
             << ", description: " << Utils::getErrorCodeAsString(error)
             << std::endl;
  }
}

void MyDefaultProfilesCallback::onProfileListResponse(
   const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
   telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      this->profileList_ = profiles;
   }
   this->prom_.set_value(error);
}
