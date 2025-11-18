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
 *  Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorControlMenu.cpp
 *
 * @brief      This file hosts test for sensor configuration and data acquisition.
 */

#include <algorithm>
#include <chrono>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <future>
#include <string>

#include <telux/sensor/SensorFactory.hpp>
#include "SensorControlMenu.hpp"
#include "SensorUtils.hpp"
#include "../../common/utils/Utils.hpp"

SensorControlMenu::SensorControlMenu(
    std::string appName, std::string cursor, SensorTestAppArguments commandLineArgs)
   : ConsoleApp(appName, cursor)
   , commandLineArgs_(commandLineArgs) {
    clientIdMask_.reset();
}

SensorControlMenu::~SensorControlMenu() {
    cleanup();
}

telux::common::ServiceStatus SensorControlMenu::initSensorManager() {
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<ServiceStatus> prom;
    //  Get the SensorFactory and SensorManager instances.
    auto &sensorFactory = telux::sensor::SensorFactory::getInstance();
    sensorManager_ = sensorFactory.getSensorManager(
        [&prom](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (!sensorManager_) {
        std::cout << "Failed to get SensorManager object" << std::endl;
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }
    //  Check if sensor subsystem is ready
    //  If sensor subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = sensorManager_->getServiceStatus();
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nSensor subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }
    //  Exit the application, if SDK is unable to initialize sensor subsystems
    if (managerStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Sensor Subsystems to ready : " << elapsedTime.count() << "s"
                  << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize sensor subsystem" << std::endl;
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::ServiceStatus SensorControlMenu::init(bool shouldInitConsole) {
    telux::common::ServiceStatus serviceStatus = initSensorManager();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        if (shouldInitConsole) {
            initConsole();
        }
    }
    return serviceStatus;
}

void SensorControlMenu::initConsole() {
    std::shared_ptr<ConsoleAppCommand> listAvailableSensorsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "List_Available_Sensors", {},
            std::bind(&SensorControlMenu::listAvailableSensors, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> createSensorClientCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Create_Sensor_Client", {},
            std::bind(&SensorControlMenu::createSensorClient, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> listCreatedSensorsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "List_Created_Sensor_Clients",
            {}, std::bind(&SensorControlMenu::listCreatedSensors, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> configureSensorCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Configure_Sensor_Client", {},
            std::bind(&SensorControlMenu::configureSensor, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> activateSensorCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Activate_Sensor_Client", {},
            std::bind(&SensorControlMenu::activateSensor, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> deactivateSensorCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Deactivate_Sensor_Client", {},
            std::bind(&SensorControlMenu::deactivateSensor, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> deleteSensorClientCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Delete_Sensor_Client", {},
            std::bind(&SensorControlMenu::deleteSensorClient, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> listActiveClientsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "List_Active_Clients", {},
            std::bind(&SensorControlMenu::listActiveClients, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> startSelfTestCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Start_Self_Test", {},
            std::bind(&SensorControlMenu::startSelfTest, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> setEulerAnglesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Set_Euler_Angles", {},
            std::bind(&SensorControlMenu::setEulerAngles, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> startSelfTestExCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "Start_Self_Test_Ex", {},
            std::bind(&SensorControlMenu::startSelfTestEx, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
        = {listAvailableSensorsCommand, createSensorClientCommand, listCreatedSensorsCommand,
            configureSensorCommand, activateSensorCommand, deactivateSensorCommand,
            deleteSensorClientCommand, listActiveClientsCommand, startSelfTestCommand,
            setEulerAnglesCommand, startSelfTestExCommand};

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void SensorControlMenu::listAvailableSensors(std::vector<std::string> userInput) {
    std::vector<SensorInfo> info;
    telux::common::Status status = sensorManager_->getAvailableSensorInfo(info);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getAvailableSensorInfo failed: " << std::endl;
        Utils::printStatus(status);
        return;
    }
    std::cout << "Sensor info request successful" << std::endl;
    for (SensorInfo i : info) {
        SensorUtils::printSensorInfo(i);
    }
}

int SensorControlMenu::getAvailableID() {
    const uint32_t maxId = sizeof(uint64_t) * 8;  // Number of bits in uint64_t (= 64)
    for (uint32_t id = 1; id < maxId; ++id) {
        if (clientIdMask_.test(id)) {
            continue;
        }
        return static_cast<int>(id);
    }
    return -1;
}

void SensorControlMenu::createSensorClient(std::vector<std::string> userInput) {
    std::string name;
    SensorUtils::getInput("Enter sensor name: ", name);
    std::shared_ptr<ISensorClient> sensor;
    telux::common::Status status = sensorManager_->getSensorClient(sensor, name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getSensorClient failed: ";
        Utils::printStatus(status);
        return;
    }
    int cid = getAvailableID();
    if (cid == -1) {
        std::cout << "Limit reached on number of sensor clients (63). Unable to create client. "
                     "Delete one or more client."
                  << std::endl;
        return;
    }
    std::shared_ptr<SensorClient> sensorClient
        = std::make_shared<SensorClient>(cid, sensor, commandLineArgs_);
    sensorClient->init();
    sensorClients_.push_back(sensorClient);
    std::cout << "Sensor client with id " << cid << " created successfully" << std::endl;
    clientIdMask_.set(cid);
    listCreatedSensors(userInput);
}

void SensorControlMenu::listCreatedSensors(std::vector<std::string> userInput) {
    for (auto s : sensorClients_) {
        s->printInfo();
    }
}

void SensorControlMenu::configureSensor(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    SensorConfiguration config = SensorUtils::getSensorConfig(sensor);
    sensor->configure(config);
}

void SensorControlMenu::setEulerAngles(std::vector<std::string> userInput) {
    telux::sensor::EulerAngleConfig EulerAngleConfig = SensorUtils::getEulerAngleConfig();
    telux::common::Status status = sensorManager_->setEulerAngleConfig(EulerAngleConfig);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "sensor setEulerAngleConfig failed: ";
        Utils::printStatus(status);
    } else {
        std::cout << "Sensor setEulerAngleConfig successful" << std::endl;
    }
}

void SensorControlMenu::activateSensor(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    sensor->activate();
}

void SensorControlMenu::deactivateSensor(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    sensor->deactivate();
}

void SensorControlMenu::enableLowPowerMode(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    sensor->enableLowPowerMode();
}

void SensorControlMenu::disableLowPowerMode(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    sensor->disableLowPowerMode();
}

void SensorControlMenu::deleteSensorClient(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    sensor->cleanup();
    auto it = std::remove_if(sensorClients_.begin(), sensorClients_.end(),
        [&](std::shared_ptr<SensorClient> s) { return (s == sensor); });
    sensorClients_.erase(it, sensorClients_.end());
    std::cout << "Removed sensor with client ID " << cid << std::endl << std::endl;
    clientIdMask_.reset(cid);
}

void SensorControlMenu::listActiveClients(std::vector<std::string> userInput) {
    for (auto s : sensorClients_) {
        if (s->isActive()) {
            s->printInfo();
        }
    }
}

void SensorControlMenu::startSelfTest(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    int selfTestType = -1;
    do {
        SensorUtils::getInput("Choose test type(0- Positive, 1- Negative, 2- All): ", selfTestType);
        if ((selfTestType >= 0) && (selfTestType <= 2)) {
            break;
        }
    } while (true);
    sensor->selfTest(static_cast<SelfTestType>(selfTestType));
}

void SensorControlMenu::startSelfTestEx(std::vector<std::string> userInput) {
    int cid = -1;
    SensorUtils::getInput("Enter Client ID: ", cid);
    std::shared_ptr<SensorClient> sensor = SensorUtils::getSensorClient(cid, sensorClients_);
    if (sensor == nullptr) {
        return;
    }
    int selfTestType = -1;
    do {
        SensorUtils::getInput("Choose test type(0- Positive, 1- Negative, 2- All): ", selfTestType);
        if ((selfTestType >= 0) && (selfTestType <= 2)) {
            break;
        }
    } while (true);
    sensor->selfTestEx(static_cast<SelfTestType>(selfTestType));
}

void SensorControlMenu::cleanup() {
    sensorClients_.clear();
    clientIdMask_.reset();
    sensorManager_ = nullptr;
}
