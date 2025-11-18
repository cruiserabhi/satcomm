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

 *  Copyright (c) 2021,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "ServingSystemListener.hpp"
#include "../DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

 ServingSystemListener::ServingSystemListener(SlotId slotId) :
    slotId_(slotId) {
 }

void ServingSystemListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {

    std::string stat ="";
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

    PRINT_NOTIFICATION <<
        " ** Data ServingSystem onServiceStatusChange Slot: " << static_cast<int>(slotId_)
        <<  " **\n" << stat << std::endl;
}

void ServingSystemListener::onDrbStatusChanged(telux::data::DrbStatus status) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION <<
   " Serving System Listener - received Drb status: " << DataUtils::drbStatusToString(status)
   << " on SlotId: " << static_cast<int>(slotId_) << std::endl << std::endl;
}

void ServingSystemListener::onServiceStateChanged(telux::data::ServiceStatus status) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "Service Status Notification on SlotId " << slotId_ << std::endl;
   std::cout << std::endl;
   if(status.serviceState == telux::data::DataServiceState::OUT_OF_SERVICE) {
      std::cout << "Current Status is Out Of Service" << std::endl;
   } else {
      std::cout << "Current Status is In Service" << std::endl;
      std::cout << "Preferred Rat is "
                << DataUtils::serviceRatToString(status.networkRat) << std::endl;
   }
   std::cout << std::endl;
}

void ServingSystemListener::onRoamingStatusChanged(telux::data::RoamingStatus status) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "Roaming Status Notification on SlotId " << slotId_ << std::endl;
   std::cout << std::endl;

   bool isRoaming = status.isRoaming;
   if(isRoaming) {
      std::cout << "System is in Roaming State" << std::endl;
      std::cout << "Roaming Type: ";
      switch(status.type)  {
         case telux::data::RoamingType::INTERNATIONAL:
             std::cout << "International" << std::endl;
         break;
         case telux::data::RoamingType::DOMESTIC:
             std::cout << "Domestic" << std::endl;
         break;
         default:
             std::cout << "Unknown" << std::endl;
      }
   } else {
      std::cout << "System is not in Roaming State" << std::endl;
   }
   std::cout << std::endl;
}

void ServingSystemListener::onNrIconTypeChanged(telux::data::NrIconType type) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "NR icon type Notification on SlotId " << slotId_ << std::endl;
   std::cout << std::endl;

   std::cout << "NR icon Type: ";
   switch(type)  {
      case telux::data::NrIconType::BASIC:
         std::cout << "Basic" << std::endl;
      break;
      case telux::data::NrIconType::UWB:
         std::cout << "Ultrawide Band" << std::endl;
      break;
      default:
         std::cout << "Unknown" << std::endl;
      break;
   }
}

void ServingSystemListener::onLteAttachFailure(const telux::data::LteAttachFailureInfo info) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "onLteAttachFailure on SlotId " << slotId_ << std::endl << std::endl;
    std::cout << "Lte Attach Reject Reason:   Type: "
        << DataUtils::callEndReasonTypeToString(info.rejectReason.type)
        << ", Code: " << DataUtils::callEndReasonCode(info.rejectReason) << std::endl;
    std::cout << " PLMN:";
    for (unsigned int i = 0; i < info.plmnId.size(); ++i) {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(
            info.plmnId[i]);
    }
    std::cout << std::endl;

    if (info.primaryPlmnId.size()) {
        std::cout << " Primary PLMN:";
        for (unsigned  int i = 0; i < info.primaryPlmnId.size(); ++i) {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(
                info.primaryPlmnId[i]);
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
