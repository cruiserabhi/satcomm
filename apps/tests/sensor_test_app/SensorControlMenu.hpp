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
 *  Copyright (c) 2021, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SENSORCONTROLMENU_HPP
#define SENSORCONTROLMENU_HPP

#include <bitset>
#include <vector>
#include <memory>
#include <string>

#include <telux/sensor/SensorFactory.hpp>
#include "ConsoleApp.hpp"
#include "SensorClient.hpp"

using namespace telux::sensor;
using namespace telux::common;

class SensorControlMenu : public ConsoleApp {
 public:
    SensorControlMenu(
        std::string appName, std::string cursor, SensorTestAppArguments commandLineArgs);
    ~SensorControlMenu();
    telux::common::ServiceStatus init(bool shouldInitConsole);
    void cleanup();
    void parseArgs(int argc, char **argv);
    std::shared_ptr<ISensorManager> getSensorManager() {
        return sensorManager_;
    }

 private:
    void initConsole();
    void printHelp(std::string programName);
    int getAvailableID();
    telux::common::ServiceStatus initSensorManager();
    telux::common::ServiceStatus initSensorFeatureManager();
    void listAvailableSensors(std::vector<std::string> userInput);
    void createSensorClient(std::vector<std::string> userInput);
    void listCreatedSensors(std::vector<std::string> userInput);
    void configureSensor(std::vector<std::string> userInput);
    void activateSensor(std::vector<std::string> userInput);
    void deactivateSensor(std::vector<std::string> userInput);
    void enableLowPowerMode(std::vector<std::string> userInput);
    void disableLowPowerMode(std::vector<std::string> userInput);
    void deleteSensorClient(std::vector<std::string> userInput);
    void listActiveClients(std::vector<std::string> userInput);
    void cleanupReinit(std::vector<std::string> userInput);
    void startSelfTest(std::vector<std::string> userInput);
    void setEulerAngles(std::vector<std::string> userInput);
    void startSelfTestEx(std::vector<std::string> userInput);

    // Structure instance to store the command line args passed
    SensorTestAppArguments commandLineArgs_;
    // Instance of all menu created are stored to maintain parallel running streams
    std::shared_ptr<ISensorManager> sensorManager_;
    std::vector<std::shared_ptr<SensorClient>> sensorClients_;
    std::bitset<64> clientIdMask_;
};

#endif  // SENSORCONTROLMENU_HPP
