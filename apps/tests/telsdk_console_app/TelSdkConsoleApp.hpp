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
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       TelSdkConsoleApp.hpp
 *
 * @brief      This is entry class for console application for Telematics SDK,
 *             It allows one to interactively invoke most of the public APIs in the Telematics SDK.
 */

#ifndef TELSDKCONSOLEAPP_HPP
#define TELSDKCONSOLEAPP_HPP

#include <string>
#include <vector>

#include "ModemStatus.hpp"
#include "console_app_framework/ConsoleApp.hpp"
#include "Audio/AudioClient.hpp"

class TelSdkConsoleApp : public ConsoleApp {
public:
   TelSdkConsoleApp(std::string appName, std::string cursor);
   ~TelSdkConsoleApp();

   /**
    * Used for creating a menus of high level features
    */
   void init();

   // Displays main menu
   void displayMenu();

   // Check Modem availability for Telephony
    void onModemAvailable();

private:
   void phoneMenu(std::vector<std::string> userInput);
   void callMenu(std::vector<std::string> userInput);
   void eCallMenu(std::vector<std::string> userInput);
   void smsMenu(std::vector<std::string> userInput);
   void simCardMenu(std::vector<std::string> userInput);
   void dataMenu(std::vector<std::string> userInput);
   void multiSimMenu(std::vector<std::string> userInput);
   void cellbroadcastMenu(std::vector<std::string> userInput);
   void rspMenu(std::vector<std::string> userInput);
   void imsSettingsMenu(std::vector<std::string> userInput);
   void imsServingSystemMenu(std::vector<std::string> userInput);
   void apSimProfileMenu(std::vector<std::string> userInput);
};

#endif  // TELSDKCONSOLEAPP_HPP
