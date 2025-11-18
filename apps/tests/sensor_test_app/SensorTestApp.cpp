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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file       sensorTestApp.cpp
 *
 * @brief      This is entry class for test application for sensor,
 *             It allows one to interactively invoke most of the public APIs in sensor.
 */

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <memory>

extern "C" {
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
}

#include "SensorTestApp.hpp"
#include <telux/common/Version.hpp>
#include "../../common/utils/Utils.hpp"

#define SENSOR_RECORDING_ROTATED_CONFIG "104,50,1"
#define SENSOR_RECORDING_UNROTATED_CONFIG "104,50,0"
#define RECORDING_MODE_SLEEP 60

std::shared_ptr<SensorTestApp> sensorTestApp;

SensorTestApp::SensorTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    commandlineArgs_.verboseNotification = false;
    commandlineArgs_.quiet = false;
    commandlineArgs_.printPeriod = 1;
}

SensorTestApp::~SensorTestApp() {
}

telux::common::ServiceStatus SensorTestApp::init() {
    initConsole();
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

void SensorTestApp::initConsole() {
    std::shared_ptr<ConsoleAppCommand> sensorControlMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Sensor_Control", {},
            std::bind(&SensorTestApp::sensorControlMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> sensorFeatureControlMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Sensor_Feature_Control", {},
            std::bind(&SensorTestApp::sensorFeatureControlMenu, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
        = {sensorControlMenuCommand, sensorFeatureControlMenuCommand};

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void SensorTestApp::sensorControlMenu(std::vector<std::string> userInput) {
    if (sensorControlMenu_ == nullptr) {
        sensorControlMenu_ = std::make_shared<SensorControlMenu>(
            "Sensor control menu", "sensor_control> ", commandlineArgs_);
        if (sensorControlMenu_->init(true) != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Failed to initialize sensor control menu" << std::endl;
            sensorControlMenu_ = nullptr;
            return;
        }
    } else {
        sensorControlMenu_->displayMenu();
    }
    sensorControlMenu_->mainLoop();
    displayMenu();
}

void SensorTestApp::sensorFeatureControlMenu(std::vector<std::string> userInput) {

    if (sensorFeatureControlMenu_ == nullptr) {
        sensorFeatureControlMenu_ = std::make_shared<SensorFeatureControlMenu>(
            "Sensor feature control menu", "sensor_feature_control> ", commandlineArgs_);
        if (sensorFeatureControlMenu_->init(true)
            != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Failed to initialize sensor control menu" << std::endl;
            sensorFeatureControlMenu_ = nullptr;
            return;
        }
    } else {
        sensorFeatureControlMenu_->displayMenu();
    }
    sensorFeatureControlMenu_->mainLoop();
    displayMenu();
}

void SensorTestApp::printHelp(std::string programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]" << std::endl
              << std::endl
              << "-n           Enable detailed notification information" << std::endl
              << "-q [period]  Quiet mode with threshold, events count will be printed once every "
                 "[period] seconds"
              << std::endl
              << "To launch sensor test app in non-interactive mode, refer to the -a or -g options."
              << std::endl
              << "-a samplerate,batchcount,isRotated Creates an accelerometer client with the arguments."
                 "To create multiple accelerometer clients- "
              << std::endl
              << " -a samplerate,batchcount,isRotated -a samplerate,batchcount,isRotated"
              << std::endl
              << "-g samplerate,batchcount,isRotated Creates a gyroscope client with the arguments. "
                 "To create multiple gyroscope clients- "
              << std::endl
              << " -g samplerate,batchcount,isRotated -g samplerate,batchcount,isRotated"
              << std::endl
              << "-r Create accel and gyro clients for configs [104,50,1] and [104,50,0] and enable recording mode."
              << std::endl
              << "-h           This help" << std::endl
              << "In case -q and -n both are specified, the argument specified in the end would "
                 "take effect"
              << std::endl;
}

void SensorTestApp::parseArgs(int argc, char **argv) {
    int c = -1;
    static const struct option long_options[]
        = {{"notification configuration", no_argument, 0, 'n'}, {"help", no_argument, 0, 'h'},
            {"quiet mode", required_argument, 0, 'q'},
            {"accel", required_argument, nullptr, 'a'},
            {"gyro", required_argument, nullptr, 'g'},
            {"recording", no_argument, nullptr, 'r'}, {0, 0, 0, 0}};
    int option_index = 0;
    c = getopt_long(argc, argv, "nq:a:g:rh", long_options, &option_index);
    if (c == -1) {
        return;
    }
    // getopt/getopt_long returns '?' in case it finds an argument that was not in the list or
    // when it finds that an argument that expected a parameter does not have one
    if (c == '?') {
        exit(1);
    }
    do {
        switch (c) {
            case 'a' : {
                commandlineArgs_.verboseNotification = true;
                commandlineArgs_.quiet = false;
                std::string str = optarg;
                telux::sensor::SensorConfiguration sensorConfig;
                updateSensorConfig(str, sensorConfig);
                sensorList_.push_back({"Accel", sensorConfig});
                break;
            }
            case 'g' : {
                commandlineArgs_.verboseNotification = true;
                commandlineArgs_.quiet = false;
                std::string str = optarg;
                telux::sensor::SensorConfiguration sensorConfig;
                updateSensorConfig(str, sensorConfig);
                sensorList_.push_back({"Gyro", sensorConfig});
                break;
            }
            case 'n': {
                commandlineArgs_.verboseNotification = true;
                commandlineArgs_.quiet = false;
                break;
            }
            case 'q': {
                commandlineArgs_.quiet = true;
                try {
                    commandlineArgs_.printPeriod = std::stoi(optarg);
                } catch (std::exception &e) {
                    std::cout << "Invalid value " << optarg << " provided for period (in seconds)"
                              << std::endl;
                    exit(1);
                }
                commandlineArgs_.verboseNotification = false;
                break;
            }
            case 'r': {
                commandlineArgs_.verboseNotification = true;
                commandlineArgs_.quiet = false;
                std::string rotatedConfigStr = SENSOR_RECORDING_ROTATED_CONFIG;
                std::string unrotatedConfigStr = SENSOR_RECORDING_UNROTATED_CONFIG;
                telux::sensor::SensorConfiguration rotatedSensorConfig;
                telux::sensor::SensorConfiguration unrotatedSensorConfig;
                updateSensorConfig(rotatedConfigStr, rotatedSensorConfig);
                updateSensorConfig(unrotatedConfigStr, unrotatedSensorConfig);
                sensorList_.push_back({"Accel", rotatedSensorConfig});
                sensorList_.push_back({"Gyro", rotatedSensorConfig});
                sensorList_.push_back({"Accel", unrotatedSensorConfig});
                sensorList_.push_back({"Gyro", unrotatedSensorConfig});
                setRecordingFlag(true);
                break;
            }
            case 'h': {
                printHelp(argv[0]);
                exit(0);
            }
        }
        c = getopt_long(argc, argv, "nq:a:g:h", long_options, &option_index);
    } while (c != -1);
    if (commandlineArgs_.verboseNotification) {
        std::cout << "Enabling verbose notification" << std::endl;
    }
    if (commandlineArgs_.quiet) {
        std::cout << "Enabling quiet mode with period = " << commandlineArgs_.printPeriod
                  << std::endl;
    }
}

void SensorTestApp::updateSensorConfig(std::string str,
    telux::sensor::SensorConfiguration &sensorConfig) {
    std::istringstream ss(str);
    std::string token;
    std::vector<std::string> configList;
    while(std::getline(ss, token, ',')) {
        configList.push_back(token);
    }
    sensorConfig.samplingRate = std::stof(configList[0]);
    sensorConfig.batchCount = std::stoi(configList[1]);
    sensorConfig.isRotated = std::stoi(configList[2]);
    sensorConfig.validityMask.set(telux::sensor::SensorConfigParams::SAMPLING_RATE);
    sensorConfig.validityMask.set(telux::sensor::SensorConfigParams::BATCH_COUNT);
    sensorConfig.validityMask.set(telux::sensor::SensorConfigParams::ROTATE);
}

void SensorTestApp::nonInteractiveLaunch() {
    //Sensor Mgr init
    std::shared_ptr<SensorControlMenu> sensorControlMenu = std::make_shared<SensorControlMenu>(
            "Sensor control menu", "sensor_control> ", commandlineArgs_);
    if (sensorControlMenu->init(false) != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Failed to initialize sensor manager" << std::endl;
        return;
    }

    //Retrieve Sensor Mgr.
    std::shared_ptr<telux::sensor::ISensorManager>
        sensorManager = sensorControlMenu->getSensorManager();

    //Creating extra client IDs for tracking different accelerometers and gyroscopes.
    int accelID = 1;
    int gyroID = 1;
    for(auto it: sensorList_) {
        // "it" represents a sensor - {Sensor Name, Sensor Configuration}
        std::string sensorName = it.first;
        int sensorID;
        if(sensorName == "Accel") {
            sensorID = accelID;
            accelID++;
        } else {
            sensorID = gyroID;
            gyroID++;
        }

        //Create corresponding sensor client and register listener.
        std::shared_ptr<telux::sensor::ISensorClient> sensor;
        telux::common::Status status = sensorManager->getSensorClient(sensor, sensorName);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " Failed to get sensor: "<< sensorName << " " << sensorID << "\n";
            return;
        }
        std::cout << " Getting sensor: " << sensorName << " " << sensorID <<
            " Sampling Rate: " << it.second.samplingRate << " Batch Count: " <<
                it.second.batchCount << "Rotate: " << it.second.isRotated << "\n";

        std::shared_ptr<SensorClient> sensorClient
            = std::make_shared<SensorClient>(sensorID, sensor, commandlineArgs_);
        if (!sensorClient) {
            std::cout << "sensorclinet is Null \n";
            return;
        }
        sensorClient->init();
        sensorClientList_.push_back(sensorClient);

        //Use SensorClient to configure Sensor
        telux::sensor::SensorConfiguration sensorConfig = it.second;
        status = sensorClient->configure(sensorConfig);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " Failed to configure sensor: "<< sensorName << " " << sensorID << "\n";
            return;
        }

        if(isRecordingEnabled_){
            sensorClient->setRecordingFlag(true);
        }

        //Activate the client.
        status = sensorClient->activate();
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Failed to activate sensor \n";
            return;
        }
    }
    if(isRecordingEnabled_){
        while(1) {
            //Infinite polling to keep retrieving position reports.
            std::this_thread::sleep_for(std::chrono::seconds(RECORDING_MODE_SLEEP));
        }
    }

    //Retrieve reports for a minute
    std::this_thread::sleep_for(std::chrono::seconds(60));

    //Cleanup the clients.
    std::cout << "Deactivating clients \n";
    for(auto sensorClient: sensorClientList_) {
        //Deactivate the client and deregister the listener.
        sensorClient->cleanup();
    }
}

static void signalHandler(int signal) {
    sensorTestApp = nullptr;
    exit(0);
}

static void setupSignalHandler() {
    signal(SIGINT, signalHandler);
}

int main(int argc, char **argv) {
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "Sensor test app - SDK v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch) +"\n" +
                          "Release name: " + sdkReleaseName;

    setupSignalHandler();
    sensorTestApp = std::make_shared<SensorTestApp>(appName, "sensor> ");
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "sensors", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    sensorTestApp->parseArgs(argc, argv);
    if(!sensorTestApp->sensorList_.empty()) {
        std::cout << "Sensor App launched During boot\n";
        sensorTestApp->nonInteractiveLaunch();
    } else {
        // initialize commands and display
        if (sensorTestApp->init() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            return -1;
        }
        sensorTestApp->mainLoop();  // Main loop to continuously read and execute commands
        sensorTestApp = nullptr;
    }
    return 0;
}

void SensorTestApp::setRecordingFlag(bool enable) {
   isRecordingEnabled_ = enable;
}
