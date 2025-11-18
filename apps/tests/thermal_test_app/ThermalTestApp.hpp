/*
 *  Copyright (c) 2019 The Linux Foundation. All rights reserved.
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
 *
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMALTESTAPP_HPP
#define THERMALTESTAPP_HPP

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <telux/therm/ThermalManager.hpp>
#include "ThermalHelper.hpp"

#include "console_app_framework/ConsoleApp.hpp"

enum InitWithProc {
    LOCAL,
    REMOTE,
    BOTH,
};

class ThermalListener;

class ThermalTestApp : public ConsoleApp {
 public:
    /**
     * Initialize commands and SDK
     */
    bool init();

    ThermalTestApp(std::string appName, std::string cursor);

    ~ThermalTestApp();

    void getThermalZones(std::vector<std::string> userInput);
    void getCoolingDevices(std::vector<std::string> userInput);
    void getThermalZoneById(std::vector<std::string> userInput);
    void getCoolingDeviceById(std::vector<std::string> userInput);

    void controlRegistration(std::vector<std::string> userInput);

    void signalHandler(int signum);

 private:
    bool initThermalManager(telux::common::ProcType procType);
    telux::common::Status manageIndication(telux::common::ProcType procType,
        bool registerInd, telux::therm::ThermalNotificationMask mask = 0xFFFF);
    telux::common::ProcType getProcType();
    int readAndValidate(std::string msg, int minRange, int maxRange);

    void handleAllUnSolicitedEvents(telux::common::ProcType procType, bool isRegister);
    void handleTripUpdateEvent(telux::common::ProcType procType, bool isRegister);
    void handleCdevLevelUpdateEvent(telux::common::ProcType procType, bool isRegister);

    void cleanup();

    std::map<telux::common::ProcType, std::shared_ptr<telux::therm::IThermalManager>>
        thermalManagerMap_;
    std::map<telux::common::ProcType, std::shared_ptr<ThermalListener>> thermalListenerMap_;
    InitWithProc initWithProc_;

    using memberFun = void (ThermalTestApp::*)(telux::common::ProcType, bool);
    memberFun memberFunArr[3] = {&ThermalTestApp::handleAllUnSolicitedEvents,
        &ThermalTestApp::handleTripUpdateEvent, &ThermalTestApp::handleCdevLevelUpdateEvent};

    template <typename T>
    static void getInput(std::string prompt, T &input) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        ss >> input;
        bool valid = false;
        do {
            if (!ss.bad() && ss.eof() && !ss.fail()) {
                valid = true;
            } else {
                // If an error occurs then an error flag is set and future attempts to get
                // input will fail. Clear the error flag on cin.
                std::cin.clear();
                // Clear the string stream's states and buffer
                ss.clear();
                ss.str("");
                std::cout << "Invalid input, please re-enter" << std::endl;
                std::cout << prompt;
                std::getline(std::cin, line);
                ss << line;
                ss >> input;
            }
        } while (!valid);
    }
};

#endif  // THERMALTESTAPP_HPP
