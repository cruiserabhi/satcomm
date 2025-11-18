/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CARDSERVICESMENU_HPP
#define CARDSERVICESMENU_HPP

#include "telux/tel/CardManager.hpp"
#include "telux/tel/CardDefines.hpp"
#include "console_app_framework/ConsoleApp.hpp"
#include "MyCardListener.hpp"
#include "telux/tel/CardApp.hpp"

class CardServicesMenu : public ConsoleApp {
public:
   CardServicesMenu(std::string appName, std::string cursor);
   ~CardServicesMenu();
   bool init();

private:
   void getCardState(std::vector<std::string> userInput);
   void getSupportedApps(std::vector<std::string> userInput);
   void openLogicalChannel(std::vector<std::string> userInput);
   void closeLogicalChannel(std::vector<std::string> userInput);
   void transmitApdu(std::vector<std::string> userInput);
   void basicTransmitApdu(std::vector<std::string> userInput);
   void changeCardPin(std::vector<std::string> userInput);
   void unlockCardByPuk(std::vector<std::string> userInput);
   void unlockCardByPin(std::vector<std::string> userInput);
   void queryPin1LockState(std::vector<std::string> userInput);
   void queryFdnLockState(std::vector<std::string> userInput);
   void setCardLock(std::vector<std::string> userInput);
   void cardPower(std::vector<std::string> userInput);
   void cardFileMenu(std::vector<std::string> userInput);
   void cardRefreshMenu(std::vector<std::string> userInput);
   void checkNtnProfileActive(std::vector<std::string> userInput);
   void selectCardSlot(std::vector<std::string> userInput);
   std::string appTypeToString(telux::tel::AppType appType);
   std::string appStateToString(telux::tel::AppState appState);
   std::string cardStateToString(telux::tel::CardState state);

   std::shared_ptr<telux::tel::ICardListener> cardListener_;
   std::shared_ptr<MyOpenLogicalChannelCallback> myOpenLogicalChannelCb_;
   std::shared_ptr<MyTransmitApduResponseCallback> myTransmitApduCb_;
   std::shared_ptr<MyCardCommandResponseCallback> myCloseLogicalChannelCb_;
   std::shared_ptr<telux::tel::ICardManager> cardManager_;
   int slot_ = DEFAULT_SLOT_ID;
   std::vector<std::shared_ptr<telux::tel::ICard>> cards_;
};

#endif  // CARDSERVICESMENU_HPP
