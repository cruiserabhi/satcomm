/*
 *  Copyright (c) 2019,2021 The Linux Foundation. All rights reserved.
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
#include "MyDataFilterListener.hpp"

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void MyDataFilterListener::onDataRestrictModeChange(DataRestrictMode mode) {
    if (mode.filterMode == DataRestrictModeType::ENABLE) {
        print_notification << "Data Filter Mode : Enable" << std::endl;
    } else if (mode.filterMode == DataRestrictModeType::DISABLE) {
        print_notification << "Data Filter Mode : Disable" << std::endl;
    } else {
        std::cout << " ERROR: Invalid Data Filter mode notified" << std::endl;
    }
}

void MyDataFilterListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
   print_notification << " ** Data Filter onServiceStatusChange **\n" << stat << std::endl;
}