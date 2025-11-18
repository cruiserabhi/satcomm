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

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <string>

#include "ImsSettingsListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void ImsSettingsListener::onImsServiceConfigsChange(SlotId slotId,
    telux::tel::ImsServiceConfig config) {
    PRINT_NOTIFICATION << "onImsServiceConfigChange, SlotId: " << static_cast<int>(slotId)
                       << "\n";
    //For VOIMS configuration
    if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_VOIMS]) {
       if (config.voImsEnabled) {
           PRINT_NOTIFICATION << "VOIMS is enabled \n";
       } else {
           PRINT_NOTIFICATION << "VOIMS is disabled \n";
       }
    }
    //For IMS service configuration
    if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_IMS_SERVICE]) {
       if (config.imsServiceEnabled) {
           PRINT_NOTIFICATION << "IMS service is enabled \n";
       } else {
           PRINT_NOTIFICATION << "IMS service is disabled \n";
       }
    }
    //For SMS over IMS configuration
    if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_SMS]) {
       if (config.smsEnabled) {
           PRINT_NOTIFICATION << "SMS over IMS is enabled \n";
       } else {
           PRINT_NOTIFICATION << "SMS over IMS is disabled \n";
       }
    }
    //For RTT configuration
    if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_RTT]) {
       if (config.rttEnabled) {
           PRINT_NOTIFICATION << "RTT is enabled \n";
       } else {
           PRINT_NOTIFICATION << "RTT is disabled \n";
       }
    }
}

// Notify ImsSettingsManager subsystem status
void ImsSettingsListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
   PRINT_NOTIFICATION << " IMS Settings onServiceStatusChange" << stat << "\n";
}

void ImsSettingsListener::onImsSipUserAgentChange(SlotId slotId, std::string sipUserAgent) {

   PRINT_NOTIFICATION << " IMS SIP user agent is " << sipUserAgent << " on slot "
      << static_cast<int>(slotId) << "\n";
}
