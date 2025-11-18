/*
 *  Copyright (c) 2018-2020 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2023, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "MyNetworkSelectionHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::tel;
using namespace telux::common;

std::string
   MyNetworkSelectionHelper::networkSelectionModeToString(telux::tel::NetworkSelectionMode mode) {
   std::string modeString = "UNKNOWN";
   switch(mode) {
      case telux::tel::NetworkSelectionMode::AUTOMATIC:
         modeString = "AUTOMATIC";
         break;
      case telux::tel::NetworkSelectionMode::MANUAL:
         modeString = "MANUAL";
         break;
      default:
         break;
   }
   return modeString;
}

void MyNetworkSelectionHelper::logPreferredNetworkInfo(
   std::vector<telux::tel::PreferredNetworkInfo> nwInfo) {
   for(auto nw : nwInfo) {
      std::cout << " Mcc: " << nw.mcc << ", Mnc: " << nw.mnc << ", RAT type: ";
      for(int index = 0; index < nw.ratMask.size(); index++) {
         if(nw.ratMask.test(index)) {
            if(index == 15) {
               std::cout << "UMTS ";
            }
            if(index == 14) {
               std::cout << "LTE ";
            }
            if(index == 7) {
               std::cout << "GSM ";
            }
            if(index == 11) {
               std::cout << "NR5G ";
            }
         } else {
            continue;
         }
      }
      std::cout << std::endl;
   }
}
void MySelectionModeResponseCallback::selectionModeResponse(
   NetworkModeInfo info, telux::common::ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Network selection mode: "
               << MyNetworkSelectionHelper::networkSelectionModeToString(info.mode)
               << std::endl;
      if (info.mode == NetworkSelectionMode::MANUAL) {
          PRINT_CB << "MCC is: " << info.mcc << ", MNC is: " << info.mnc << std::endl;
      }
   } else {
      PRINT_CB << "Network selection mode failed, ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyPreferredNetworksResponseCallback::preferredNetworksResponse(
   std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo,
   std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo,
   telux::common::ErrorCode error) {
   std::cout << std::endl;
   PRINT_CB << "\n************* Preferred networks response *****************" << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      MyNetworkSelectionHelper::logPreferredNetworkInfo(preferredNetworks3gppInfo);
      PRINT_CB << "Static preferred networks: " << std::endl;
      MyNetworkSelectionHelper::logPreferredNetworkInfo(staticPreferredNetworksInfo);
   } else {
      PRINT_CB << "ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
   std::cout << "\n*********************************************************\n";
}

void MyNetworkResponsecallback::setNetworkSelectionModeResponseCb(telux::common::ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Set network selection mode is successful" << std::endl;
   } else {
      PRINT_CB << "Set network selection mode failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyNetworkResponsecallback::setPreferredNetworksResponseCb(telux::common::ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Set preferred networks is successful" << std::endl;
   } else {
      PRINT_CB << "Set preferred networks failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyNetworkSelectionHelper::logInUseStatus(int status) {
   switch(status) {
      case 0:
         std::cout << "In-use status: UNKNOWN, ";
         break;
      case 1:
         std::cout << "In-use status: CURRENT_SERVING, ";
         break;
      case 2:
         std::cout << "In-use status: AVAILABLE, ";
         break;
      default:
         break;
   }
}

void MyNetworkSelectionHelper::logRoamingStatus(int status) {
   switch(status) {
      case 0:
         std::cout << "Roaming status: UNKNOWN, ";
         break;
      case 1:
         std::cout << "Roaming status: HOME, ";
         break;
      case 2:
         std::cout << "Roaming status: ROAM, ";
         break;
      default:
         break;
   }
}

void MyNetworkSelectionHelper::logForbiddenStatus(int status) {
   switch(status) {
      case 0:
         std::cout << "Forbidden status: UNKNOWN, ";
         break;
      case 1:
         std::cout << "Forbidden status: FORBIDDEN, ";
         break;
      case 2:
         std::cout << "Forbidden status: NOT_FORBIDDEN, ";
         break;
      default:
         break;
   }
}

void MyNetworkSelectionHelper::logPreferredStatus(int status) {
   switch(status) {
      case 0:
         std::cout << "Preferred status: UNKNOWN" << std::endl;
         break;
      case 1:
         std::cout << "Preferred status: PREFERRED" << std::endl;
         break;
      case 2:
         std::cout << "Preferred status: NOT_PREFERRED" << std::endl;
         break;
      default:
         break;
   }
}

void MyPerformNetworkScanCallback::performNetworkScanResponseCb(
   telux::common::ErrorCode error) {
   std::cout << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Network scan is successful" << std::endl;
   } else {
      PRINT_CB << "Network scan failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

std::string MyNetworkSelectionListener::networkScanStatusToString(
   telux::tel::NetworkScanStatus scanStatus) {
   std::string status = "";
   switch (scanStatus) {
      case telux::tel::NetworkScanStatus::COMPLETE:
         status = "COMPLETE";
         break;
      case telux::tel::NetworkScanStatus::PARTIAL:
         status = "PARTIAL";
         break;
      case telux::tel::NetworkScanStatus::FAILED:
         status = "FAILED";
         break;
    }
    return status;
}

std::string MyNetworkSelectionListener::convertRatTypeAsString(telux::tel::RadioTechnology rat) {
   std::string ratType = "";
   switch (rat) {
      case telux::tel::RadioTechnology::RADIO_TECH_EDGE:
         ratType = "GERAN";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_UMTS:
         ratType = "UMTS";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_LTE:
         ratType = "LTE";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA:
         ratType = "TDSCDMA";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_NR5G:
         ratType = "NR5G";
         break;
      default:
         ratType = "UNKNOWN";
         break;
    }
    return ratType;
}

void MyNetworkSelectionListener::onNetworkScanResults(NetworkScanStatus scanStatus,
   std::vector<telux::tel::OperatorInfo> operatorInfos) {
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n************ Perform network scan response ************" << std::endl;
   std::cout << "Operator Info size: " << operatorInfos.size() << std::endl;
   std::cout << "Network Scan Results Status: " << networkScanStatusToString(scanStatus);
   for(auto it : operatorInfos) {
      std::cout << "\nName: " << it.getName() << "\nMcc: " << it.getMcc()
                << "\nMnc: " << it.getMnc() << "\nRat: "
                << convertRatTypeAsString(it.getRat()) << std::endl;
      MyNetworkSelectionHelper::logInUseStatus(static_cast<int>(it.getStatus().inUse));
      MyNetworkSelectionHelper::logRoamingStatus(static_cast<int>(it.getStatus().roaming));
      std::cout << std::endl;
      MyNetworkSelectionHelper::logForbiddenStatus(static_cast<int>(it.getStatus().forbidden));
      MyNetworkSelectionHelper::logPreferredStatus(static_cast<int>(it.getStatus().preferred));
      std::cout << std::endl;
   }
   std::cout << "\n*********************************************************\n";
}

void MyNetworkSelectionListener::onSelectionModeChanged(telux::tel::NetworkModeInfo info) {
   std::cout << std::endl;
   PRINT_NOTIFICATION
      << "Network selection mode: "
      << MyNetworkSelectionHelper::networkSelectionModeToString(info.mode)
      << std::endl;
   if (info.mode == NetworkSelectionMode::MANUAL) {
      PRINT_NOTIFICATION << "MCC is: " << info.mcc << ", MNC is: " << info.mnc << std::endl;
   }
}

// Notify NetworkSelectionManager subsystem status
void MyNetworkSelectionListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " Network Selection onServiceStatusChange" << stat << "\n";
}
