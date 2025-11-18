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

#include <iostream>
#include <memory>
#include <bitset>

#include <telux/data/DataDefines.hpp>

#include "MyProfileListener.hpp"

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

MyProfileListener::MyProfileListener(SlotId slotId)
   : slotId_(slotId) {
}

void MyProfileListener::onServiceStatusChange(telux::common::ServiceStatus status) {
   std::string stat;

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

   print_notification << " ** Data Profile onServiceStatusChange on Slot-" << slotId_
                      << " ** " << stat << std::endl;
}

void MyProfileListener::onProfileUpdate(int profileId, telux::data::TechPreference techPreference,
                                        telux::data::ProfileChangeEvent event) {
   print_notification << "Slot-" << slotId_ << " Profile updated: " << std::endl;
   print_notification << "Profile Id: " << profileId
                      << " TechPreference: " << static_cast<int>(techPreference)
                      << " ProfileChangeEvent: " << getProfileEventString(event) << std::endl;
}

std::string MyProfileListener::getProfileEventString(
   telux::data::ProfileChangeEvent event) {
   switch(event) {
      case telux::data::ProfileChangeEvent::CREATE_PROFILE_EVENT:
         return std::string("CREATE PROFILE");
      case telux::data::ProfileChangeEvent::DELETE_PROFILE_EVENT:
         return std::string("DELETE PROFILE");
      case telux::data::ProfileChangeEvent::MODIFY_PROFILE_EVENT:
         return std::string("MODIFY PROFILE");
      default:
         std::cout << "Unexpected Profile Event = " << static_cast<int>(event) << std::endl;
         return std::string("UNKNOWN");
   }
}
