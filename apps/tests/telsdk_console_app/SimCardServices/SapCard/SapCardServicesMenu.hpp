/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef SAPCARDSERVICESMENU_HPP
#define SAPCARDSERVICESMENU_HPP

#include <iostream>
#include <string>
#include <memory>

#include <telux/tel/SapCardManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"
#include "MySapCardListener.hpp"

class SapCardServicesMenu : public ConsoleApp {
public:
   SapCardServicesMenu(std::string appName, std::string cursor);
   ~SapCardServicesMenu();
   bool init();

private:
   void openSapConnection(std::vector<std::string> userInput);
   void getSapAtr(std::vector<std::string> userInput);
   void requestSapState(std::vector<std::string> userInput);
   void getState(std::vector<std::string> userInput);
   void transmitSapApdu(std::vector<std::string> userInput);
   void sapSimPowerOff(std::vector<std::string> userInput);
   void sapSimPowerOn(std::vector<std::string> userInput);
   void sapSimReset(std::vector<std::string> userInput);
   void sapCardReaderStatus(std::vector<std::string> userInput);
   void closeSapConnection(std::vector<std::string> userInput);
   void selectSimSlot(std::vector<std::string> userInput);
   void logSapState(telux::tel::SapState sapState);

   std::shared_ptr<telux::tel::ISapCardListener> sapCardListener_ = nullptr;
   std::shared_ptr<MySapCommandResponseCallback> mySapCmdResponseCb_ = nullptr;
   std::shared_ptr<MyCardReaderCallback> mySapCardReaderCb_ = nullptr;
   std::shared_ptr<MySapTransmitApduResponseCallback> myTransmitApduResponseCb_ = nullptr;
   std::shared_ptr<MyAtrResponseCallback> myAtrCb_ = nullptr;
   int slot_ = DEFAULT_SLOT_ID;
   std::vector<std::shared_ptr<telux::tel::ISapCardManager>> sapManagers_;
};

#endif  // SAPCARDSERVICESMENU_HPP
