/*
 *  Copyright (c) 2018-2019, 2021, The Linux Foundation. All rights reserved.
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

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "DataListener.hpp"
#include "DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

DataListener::DataListener(SlotId slotId)
   : slotId_(slotId) {
}

void DataListener::initDataCallListResponseCb(
    const std::vector<std::shared_ptr<telux::data::IDataCall>> &dataCallList,
    telux::common::ErrorCode error) {

    if (telux::common::ErrorCode::SUCCESS == error) {
        for (auto dataCall:dataCallList) {
            updateDataCallMap(dataCall);
        }
    }
}

void DataListener::onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   logDataCallDetails(dataCall);
   updateDataCallMap(dataCall);
}

void DataListener::onServiceStatusChange(telux::common::ServiceStatus status) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << " ** Data onServiceStatusChange on Slot-" << slotId_ << " ** \n";
   switch(status) {
      case telux::common::ServiceStatus::SERVICE_AVAILABLE:
         std::cout << " SERVICE_AVAILABLE";
         break;
      case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
         std::cout << " SERVICE_UNAVAILABLE";
         break;
      default:
         std::cout << " Unknown service status";
         break;
   }
   std::cout << std::endl;
}

void DataListener::onThrottledApnInfoChanged(
   const std::vector<telux::data::APNThrottleInfo> &throttleInfoList) {
   PRINT_NOTIFICATION << "onThrottledApnInfoChanged \n";
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
}

void DataListener::onHwAccelerationChanged(telux::data::ServiceState state) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << " ** Data onHwAccelerationChanged on Slot-" << slotId_ << " ** \n";
   switch(state) {
      case telux::data::ServiceState::ACTIVE:
         std::cout << " HW_ACCELERATION_ACTIVE";
         break;
      case telux::data::ServiceState::INACTIVE:
         std::cout << " HW_ACCELERATION_INACTIVE";
         break;
      default:
         std::cout << " Unknown IPACM State";
         break;
   }
   std::cout << std::endl;
}

void DataListener::onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** WWAN Connectivity Config has changed ** \n";
    std::cout << "WWAN Connectivity Config on SlotId: " << static_cast<int>(slotId) << " is: ";
    if(isConnectivityAllowed) {
        std::cout << "Allowed";
    } else {
        std::cout << "Disallowed";
    }
    std::cout << std::endl << std::endl;
}

std::shared_ptr<telux::data::IDataCall> DataListener::getDataCall(int slotId, int profileId) {
   std::lock_guard<std::mutex> lk(mtx_);
   std::shared_ptr<telux::data::IDataCall> dataCall = nullptr;
   for (auto& dc : dataCallMap_) {
      //If datacall with same profile id and slot id is found and it is in connected state
      if ((dc.first == profileId) && (slotId == dc.second->getSlotId()) &&
          (telux::data::DataCallStatus::NET_CONNECTED == dc.second->getDataCallStatus())) {
          dataCall = dc.second;
      }
   }
   return dataCall;
}

void DataListener::updateDataCallMap(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   if(dataCall) {
      std::lock_guard<std::mutex> lk(mtx_);
      int profileId = dataCall->getProfileId();
      //Find if datacall object exist
      std::multimap<int, std::shared_ptr<telux::data::IDataCall>>::iterator dataCallMapItr =
          dataCallMap_.end();
      for (auto dcItr = dataCallMap_.begin(); dcItr != dataCallMap_.end(); ++dcItr) {
         if ((dcItr->first == profileId) && (dataCall->getSlotId() == dcItr->second->getSlotId())) {
            dataCallMapItr = dcItr;
            break;
         }
      }
      //If data call object not found
      if((dataCallMapItr == dataCallMap_.end())) {
         //If it is not disconnect notification
         if(telux::data::DataCallStatus::NET_NO_NET != dataCall->getDataCallStatus()) {
            //Add it to list
            dataCallMap_.emplace(profileId, dataCall);
         }
         else {
            // it is disconnect notification, ignore it
         }
      }
      else {
         //If it is not disconnect notification
         if(telux::data::DataCallStatus::NET_NO_NET != dataCall->getDataCallStatus()) {
            //Update data call object
            dataCallMapItr->second = dataCall;
         }
         else {
            // it is disconnect notification, remove it from list
            dataCallMap_.erase(dataCallMapItr);
         }
      }
   }
}

void DataListener::logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << " ** DataCall Details **\n";
   std::cout << " SlotID: " << dataCall->getSlotId()
             << "\n ProfileID: " << dataCall->getProfileId()
             << "\n InterfaceName: " << dataCall->getInterfaceName()
             << "\n DataCallStatus: " << DataUtils::dataCallStatusToString(dataCall->getDataCallStatus())
             << "\n DataCallEndReason:\n   Type: "
             << DataUtils::callEndReasonTypeToString(dataCall->getDataCallEndReason().type)
             << ", Code: " << DataUtils::callEndReasonCode(dataCall->getDataCallEndReason()) << std::endl;
   if (telux::data::IpFamilyType::IPV4V6 == dataCall->getIpFamilyType()) {
      std::cout << " IPv4 Status: " << DataUtils::dataCallStatusToString(
         dataCall->getIpv4Info().status) << std::endl;
      std::cout << " IPv6 Status: " << DataUtils::dataCallStatusToString(
         dataCall->getIpv6Info().status) << std::endl;
   }
   std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
   for(auto &it : ipAddrList) {
      struct in_addr ifMaskAddr, gwMaskAddr;

      std::cout << "\n ifAddress: " << it.ifAddress << "\n gwAddress: " << it.gwAddress
                << "\n primaryDnsAddress: " << it.primaryDnsAddress
                << "\n secondaryDnsAddress: " << it.secondaryDnsAddress ;
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
}

void DataListener::onTrafficFlowTemplateChange(
    const std::shared_ptr<telux::data::IDataCall> &dataCall,
    const std::vector<std::shared_ptr<telux::data::TftChangeInfo>> &tfts) {
   for (auto tft:tfts) {
      std::cout << " ----------------------------------------------------------\n";
      std::cout << " ** TFT Details **\n";
      std::cout << " Flow State: "
         << DataUtils::flowStateEventToString(tft->stateChange) << std::endl;
      DataUtils::logQosDetails(tft->tft);
      std::cout << " ----------------------------------------------------------\n\n";
   }
}
