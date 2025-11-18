/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
#include "MyImsSettingsHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyImsSettingsCallback::onRequestImsServiceConfig(SlotId slotId,
   telux::tel::ImsServiceConfig config, telux::common::ErrorCode errorCode) {
    std::cout << " Request IMS service config response received on slotId "
              << static_cast<int>(slotId) << "\n";
    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request failed with errorCode: " << static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode) << "\n";
    } else {
        //For VOIMS configuration
        if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_VOIMS]) {
           if (config.voImsEnabled) {
               PRINT_CB << "VOIMS is enabled \n";
           } else {
               PRINT_CB << "VOIMS is disabled \n";
           }
        }
        //For IMS service configuration
        if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_IMS_SERVICE]) {
           if (config.imsServiceEnabled) {
               PRINT_CB << "IMS service is enabled \n";
           } else {
               PRINT_CB << "IMS service is disabled \n";
           }
        }
        //For SMS over IMS configuration
        if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_SMS]) {
           if (config.smsEnabled) {
               PRINT_CB << "SMS over IMS is enabled \n";
           } else {
               PRINT_CB << "SMS over IMS is disabled \n";
           }
        }
        //For RTT over IMS configuration
        if (config.configValidityMask[telux::tel::ImsServiceConfigType::IMSSETTINGS_RTT]) {
           if (config.rttEnabled) {
               PRINT_CB << "RTT over IMS is enabled \n";
           } else {
               PRINT_CB << "RTT over IMS is disabled \n";
           }
        }
    }
}

void MyImsSettingsCallback::onResponseCallback(telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request failed with errorCode: " << static_cast<int>(error)
                 << " Description : " << Utils::getErrorCodeAsString(error) << "\n";
    } else {
        PRINT_CB << "Request processed successfully \n";
    }
}

void MyImsSettingsCallback::onRequestImsSipUserAgentConfig(SlotId slotId,
   std::string sipUserAgent, telux::common::ErrorCode errorCode) {
    std::cout << " Request IMS SIP user agent config response received on slotId "
              << static_cast<int>(slotId) << "\n";
    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request failed with errorCode: " << static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode) << "\n";
    } else {
        //SipUserAgent configuration
        if (!sipUserAgent.empty()) {
            PRINT_CB << "sipUserAgent is " << sipUserAgent << "\n";
        } else {
            PRINT_CB << "sipUserAgent is empty\n";
        }
    }
}

void MyImsSettingsCallback::onRequestImsVonr(SlotId slotId,
    bool isEnable, telux::common::ErrorCode errorCode) {
    std::cout << " Request IMS VoNR response received on slotId "
              << static_cast<int>(slotId) << "\n";
    if (errorCode != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request failed with errorCode: " << static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode) << "\n";
    } else {
       if (isEnable) {
           PRINT_CB << "VoNR is enabled \n";
       } else {
           PRINT_CB << "VoNR is disabled \n";
       }
    }
}
