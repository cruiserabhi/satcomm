/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "ThermalCommandCallback.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

ThermalCommandCallback::ThermalCommandCallback(std::string cmdName) {
   commandName_ = cmdName;
}

void ThermalCommandCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << commandName_ << " command sent successfully" << std::endl;
      respStatusProm_.set_value(true);
      std::cout << " Notifying callback response - true " << std::endl;
   } else {
      PRINT_CB << commandName_ << " command failed\n errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
      respStatusProm_.set_value(false);
      std::cout << " Notifying callback response - false " << std::endl;
   }
}

std::future<bool> ThermalCommandCallback::commandResponseStatus() {
    return respStatusProm_.get_future();
}

void ThermalCommandCallback::getCmdResponse(AutoShutdownMode state) {

    if(state == AutoShutdownMode::ENABLE) {
        PRINT_CB << commandName_<< " Current auto shutdown mode is Enable" << std::endl;
    } else if(state == AutoShutdownMode::DISABLE) {
        PRINT_CB << commandName_ <<" Current auto shutdown mode is Disable" << std::endl;
    } else {
        PRINT_CB << commandName_ <<
           " Current auto shutdown mode is Unknown  " << std::endl;
    }
    prom_.set_value(true);
}

std::future<bool> ThermalCommandCallback::getPromValue(){

  return prom_.get_future();
}
