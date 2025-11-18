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
 *
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <telux/tel/PhoneFactory.hpp>

#include "Phone/MyPhoneListener.hpp"
#include "ModemStatus.hpp"
#include "Utils.hpp"

ModemStatus::ModemStatus() {
}

bool ModemStatus::init() {
   if ( phoneManager_ == nullptr ) {
      std::chrono::time_point<std::chrono::steady_clock> startTime, endTime;
      startTime = std::chrono::steady_clock::now();
      std::promise< telux::common::ServiceStatus > prom;
      //  Get the PhoneFactory and PhoneManager instances.
      phoneManager_ =  telux::tel::PhoneFactory::getInstance().getPhoneManager(
                       [&]( telux::common::ServiceStatus status) {
         prom.set_value(status);
      });
      if (!phoneManager_) {
         std::cout << "ERROR - Failed to get Phone Manager \n";
         return false;
      }
      telux::common::ServiceStatus phoneMgrStatus = phoneManager_->getServiceStatus();
      if (phoneMgrStatus !=  telux::common::ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << "Phone Manager subsystem is not ready, Please wait \n";
      }
      phoneMgrStatus = prom.get_future().get();
      if ( phoneMgrStatus ==  telux::common::ServiceStatus::SERVICE_AVAILABLE ) {
         endTime = std::chrono::steady_clock::now();
         std::chrono::duration<double> elapsedTime = endTime - startTime;
         std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                   << std::endl;
      } else {
          //  Return from application, if SDK is unable to initialize telephony subsystems
         std::cout << "*** ERROR - Unable to initialize telephony subsystem " << std::endl;
         return false;
      }
   }
   return true;
}

void ModemStatus::printOperatingMode() {
   phoneManager_->requestOperatingMode(shared_from_this());
   if(callbackPromise_.get_future().get()) {
      return;
   }
}

void ModemStatus::operatingModeResponse(telux::tel::OperatingMode operatingMode,
                                        telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << "Operating Mode is : " << MyPhoneHelper::operatingModeToString(operatingMode)
                << std::endl
                << std::endl;
   } else {
      std::cout << "Operating Mode is : Unknown, errorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl
                << std::endl;
   }
   callbackPromise_.set_value(true);
}
