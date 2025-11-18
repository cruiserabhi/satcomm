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

/* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2017-2019, 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SENSORTESTAPP_HPP
#define SENSORTESTAPP_HPP

#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <utility>
#include <thread>
#include <future>
#include <chrono>


#include "SensorControlMenu.hpp"
#include "SensorFeatureControlMenu.hpp"
#include "SensorUtils.hpp"

#include <telux/sensor/SensorDefines.hpp>
#include <telux/sensor/SensorClient.hpp>
#include <telux/sensor/SensorFactory.hpp>

#include "ConsoleApp.hpp"

using namespace telux::common;

class SensorTestApp : public ConsoleApp {
 public:
    SensorTestApp(std::string appName, std::string cursor);
    ~SensorTestApp();
    telux::common::ServiceStatus init();
    void parseArgs(int argc, char **argv);
    void nonInteractiveLaunch();
    std::vector<std::pair<std::string, telux::sensor::SensorConfiguration>> sensorList_;

 private:
    void initConsole();
    void printHelp(std::string programName);
    telux::common::ServiceStatus initSensorManager();
    telux::common::ServiceStatus initSensorFeatureManager();
    void sensorControlMenu(std::vector<std::string> userInput);
    void sensorFeatureControlMenu(std::vector<std::string> userInput);
    void updateSensorConfig(std::string str, telux::sensor::SensorConfiguration &sensorConfig);
    void setRecordingFlag(bool enable);

    std::vector<std::shared_ptr<SensorClient>> sensorClientList_;
    // Instance of all menu created are stored to maintain parallel running streams
    std::shared_ptr<SensorControlMenu> sensorControlMenu_;
    std::shared_ptr<SensorFeatureControlMenu> sensorFeatureControlMenu_;
    // Structure instance to store the command line args passed
    SensorTestAppArguments commandlineArgs_;
    bool isRecordingEnabled_ = false;
};

#endif  // SensorTestApp_HPP
