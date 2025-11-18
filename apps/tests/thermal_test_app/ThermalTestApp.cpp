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

#include <iostream>
#include <memory>
#include <iomanip>
#include <csignal>

#include <telux/common/Version.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/therm/ThermalFactory.hpp>
#include "../../common/utils/Utils.hpp"

#include "ThermalHelper.hpp"
#include "ThermalTestApp.hpp"
#include "ThermalListener.hpp"

using namespace telux::common;

#define PRINT_RESPONSE_SUCCESS \
    std::cout << std::endl << "\033[1;32mRESPONSE: SUCCESS\033[0m" << std::endl
#define PRINT_RESPONSE_FAILURE \
    std::cout << std::endl << "\033[1;31mRESPONSE: FAILURE\033[0m" << std::endl

#define PROC_TYPE_MSG " Enter operation type (0 - LOCAL, 1 - REMOTE): "
#define REG_DEREG_MSG " Enter operation (0 - DE-REGISTER, 1 - REGISTER): "
#define REG_TYPE_MSG " Enter registration type (0 - ALL, 1 - TRIP UPDATE, 2 - CDEV LEVEL CHANGE): "

std::shared_ptr<ThermalTestApp> thermalTestApp_ = nullptr;
auto sdkVersion = telux::common::Version::getSdkVersion();
std::string sdkReleaseName = telux::common::Version::getReleaseName();
std::string APP_NAME = "Thermal Test App v" + std::to_string(sdkVersion.major) + "."
                       + std::to_string(sdkVersion.minor) + "." + std::to_string(sdkVersion.patch)
                       + "\n" + "Release name: " + sdkReleaseName;

ThermalTestApp::ThermalTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

ThermalTestApp::~ThermalTestApp() {
    cleanup();
}

void signalHandler(int signum) {
    thermalTestApp_->signalHandler(signum);
}

void ThermalTestApp::signalHandler(int signum) {
    std::cout << APP_NAME << ": Interrupt signal (" << signum << ") received.." << std::endl;
    cleanup();
    exit(1);
}

void ThermalTestApp::cleanup() {
    for (auto thermalManager : thermalManagerMap_) {
        auto procType = thermalManager.first;
        auto manager = thermalManager.second;
        if (manager) {
            Status status = manager->deregisterListener(thermalListenerMap_[procType]);
            if (status == Status::SUCCESS) {
                std::cout << "Deregister for Thermal Listener succeed." << std::endl;
            } else {
                std::cout << "Deregister for Thermal Listener failed." << std::endl;
            }
        }
    }
    thermalManagerMap_.clear();
}

bool ThermalTestApp::init() {
    bool initStatus = false;
    int cid = -1;

    do {
        ThermalTestApp::getInput(
            "Select the application processor for operations(1-LOCAL/2-REMOTE/3-BOTH): ", cid);
        if (cid == 1) {
            initWithProc_ = LOCAL;
            initStatus = initThermalManager(telux::common::ProcType::LOCAL_PROC);
        } else if (cid == 2) {
            initWithProc_ = REMOTE;
            initStatus = initThermalManager(telux::common::ProcType::REMOTE_PROC);
        } else if (cid == 3) {
            initWithProc_ = BOTH;
            initThermalManager(telux::common::ProcType::LOCAL_PROC);
            initStatus |= initThermalManager(telux::common::ProcType::REMOTE_PROC);
        } else {
            std::cout << " Invalid input:  " << cid << ", please re-enter" << std::endl;
        }
    } while ((cid != 1) && (cid != 2) && (cid != 3));

    if (!initStatus) {
        return false;
    }

    std::shared_ptr<ConsoleAppCommand> thermalZonesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "thermal_zones", {},
            std::bind(&ThermalTestApp::getThermalZones, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> coolingDevicesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "cooling_devices", {},
            std::bind(&ThermalTestApp::getCoolingDevices, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> thermalZoneByIdCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "thermal_zone_by_id", {},
            std::bind(&ThermalTestApp::getThermalZoneById, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> coolingDeviceByIdCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "cooling_device_by_id", {},
            std::bind(&ThermalTestApp::getCoolingDeviceById, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> controlRegistrationCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "control_registration", {},
            std::bind(&ThermalTestApp::controlRegistration, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListThermalSubMenu
        = {thermalZonesCommand, coolingDevicesCommand, thermalZoneByIdCommand,
            coolingDeviceByIdCommand, controlRegistrationCommand};
    addCommands(commandsListThermalSubMenu);
    ConsoleApp::displayMenu();
    return true;
}

Status ThermalTestApp::manageIndication(telux::common::ProcType procType,
    bool registerInd, telux::therm::ThermalNotificationMask mask) {

    Status status = Status::FAILED;
    if (thermalListenerMap_.find(procType) == thermalListenerMap_.end()) {
        std::cout << " Creating thermal listener for proc type: " << static_cast<int>(procType)
                  << std::endl;
        thermalListenerMap_.emplace(procType, std::make_shared<ThermalListener>());
    }

    std::cout << " thermal listener : " << thermalListenerMap_[procType] << std::endl;
    if (registerInd) {
        status
            = thermalManagerMap_[procType]->registerListener(thermalListenerMap_[procType], mask);
    } else {
        status
            = thermalManagerMap_[procType]->deregisterListener(thermalListenerMap_[procType], mask);
    }

    if (status != Status::SUCCESS) {
        std::cout << ((registerInd) ? "Register" : "De-register")
                  << " for Thermal Listener failed, mask - " << mask.to_string() << std::endl;
        return status;
    }
    std::cout << ((registerInd) ? "Register" : "De-register") << " for Thermal Listener, mask - "
              << mask.to_string() << std::endl;
    return status;
}

bool ThermalTestApp::initThermalManager(telux::common::ProcType procType) {
    // Get thermal factory instance
    auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
    // Get thermal manager instance
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    auto thermalManager = thermalFactory.getThermalManager(
        [&](ServiceStatus status) { prom.set_value(status); }, procType);

    if (thermalManager == nullptr) {
        std::cout << " ERROR - Failed to get thermal manager instance \n";
        return false;
    }

    // Wait for thermal manager to be ready
    ServiceStatus mgrStatus = prom.get_future().get();
    if (mgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Thermal Subsystem is ready " << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize Thermal subsystem" << std::endl;
        return false;
    }
    std::cout << " thermal manager instance returned for proc type:" << static_cast<int>(procType)
              << std::endl;
    thermalManagerMap_.emplace(procType, thermalManager);
    if (manageIndication(procType, true) != Status::SUCCESS) {
        return false;
    }
    return true;
}

int ThermalTestApp::readAndValidate(std::string msg, int minRange, int maxRange) {
    int input = -1;
    do {
        ThermalTestApp::getInput(msg.c_str(), input);
        if ((input < minRange) || (input > maxRange)) {
            std::cout << " Invalid input:  " << input << ", please re-enter" << std::endl;
        }
    } while ((input < minRange) || (input > maxRange));
    return input;
}

telux::common::ProcType ThermalTestApp::getProcType() {
    int operationType = -1;
    telux::common::ProcType procType = telux::common::ProcType::LOCAL_PROC;
    if (initWithProc_ == REMOTE) {
        procType = telux::common::ProcType::REMOTE_PROC;
    } else if (initWithProc_ == BOTH) {
        operationType = readAndValidate(PROC_TYPE_MSG, 0, 1);
        procType = static_cast<telux::common::ProcType>(operationType);
    }
    return procType;
}

void ThermalTestApp::getThermalZones(std::vector<std::string> userInput) {

    telux::common::ProcType procType = getProcType();
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout << " Thermal manager is not ready for proc type: " << static_cast<int>(procType)
                  << std::endl;
        return;
    }
    std::vector<std::shared_ptr<telux::therm::IThermalZone>> zoneInfo
        = thermalManagerMap_[procType]->getThermalZones();
    if (zoneInfo.size() > 0) {
        ThermalHelper::printThermalZoneHeader();
        for (size_t index = 0; index < zoneInfo.size(); index++) {
            if (zoneInfo[index]) {
                ThermalHelper::printThermalZoneInfo(zoneInfo[index]);
            } else {
                std::cout << "No thermal zone found at index: " << index << std::endl;
            }
        }
    } else {
        std::cout << "No thermal zones found!" << std::endl;
    }
}

void ThermalTestApp::getThermalZoneById(std::vector<std::string> userInput) {
    int thermalZoneId = -1;
    ThermalTestApp::getInput("Enter thermal zone id: ", thermalZoneId);
    telux::common::ProcType procType = getProcType();
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout
            << " Thermal manager is not ready for operation type: " << static_cast<int>(procType)
            << std::endl;
        return;
    }
    std::cout << "Thermal zone Id: " << thermalZoneId << std::endl;
    std::shared_ptr<telux::therm::IThermalZone> tzInfo
        = thermalManagerMap_[procType]->getThermalZone(thermalZoneId);
    if (tzInfo != nullptr) {
        ThermalHelper::printThermalZoneHeader();
        ThermalHelper::printThermalZoneInfo(tzInfo);
        ThermalHelper::printBindingInfo(tzInfo);
    } else {
        std::cout << "No thermal zone found for Id: " << thermalZoneId << std::endl;
    }
}

void ThermalTestApp::getCoolingDevices(std::vector<std::string> userInput) {
    telux::common::ProcType procType = getProcType();
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout
            << " Thermal manager is not ready for operation type: " << static_cast<int>(procType)
            << std::endl;
        return;
    }
    std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevice
        = thermalManagerMap_[procType]->getCoolingDevices();
    if (coolingDevice.size() > 0) {
        ThermalHelper::printCoolingDeviceHeader();
        for (size_t index = 0; index < coolingDevice.size(); index++) {
            if (coolingDevice[index]) {
                ThermalHelper::printCoolingDevInfo(coolingDevice[index]);
            } else {
                std::cout << "No cooling devices found at index: " << index << std::endl;
            }
        }
    } else {
        std::cout << "No cooling devices found!" << std::endl;
    }
}

void ThermalTestApp::getCoolingDeviceById(std::vector<std::string> userInput) {
    int coolingDevId = -1;
    ThermalTestApp::getInput("Enter cooling device Id: ", coolingDevId);
    telux::common::ProcType procType = getProcType();
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout
            << " Thermal manager is not ready for operation type: " << static_cast<int>(procType)
            << std::endl;
        return;
    }
    std::cout << "Cooling device Id: " << coolingDevId << std::endl;
    std::shared_ptr<telux::therm::ICoolingDevice> cdev
        = thermalManagerMap_[procType]->getCoolingDevice(coolingDevId);
    if (cdev != nullptr) {
        ThermalHelper::printCoolingDeviceHeader();
        ThermalHelper::printCoolingDevInfo(cdev);
    } else {
        std::cout << "No cooling device found for Id: " << coolingDevId << std::endl;
    }
}

void ThermalTestApp::controlRegistration(std::vector<std::string> userInput) {
    int operation = -1, type = -1;
    auto procType = static_cast<telux::common::ProcType>(readAndValidate(PROC_TYPE_MSG, 0, 1));
    operation = readAndValidate(REG_DEREG_MSG, 0, 1);
    type = readAndValidate(REG_TYPE_MSG, 0, 2);
    (this->*memberFunArr[type])(procType, operation);
}

void ThermalTestApp::handleTripUpdateEvent(telux::common::ProcType procType, bool isRegister) {
    telux::therm::ThermalNotificationMask mask;
    Status status = Status::FAILED;
    mask.set(TNT_TRIP_UPDATE);
    status = manageIndication(procType, isRegister, mask);
    if (status != Status::SUCCESS) {
        PRINT_RESPONSE_FAILURE;
        return;
    }
    PRINT_RESPONSE_SUCCESS;
}

void ThermalTestApp::handleCdevLevelUpdateEvent(telux::common::ProcType procType, bool isRegister) {
    telux::therm::ThermalNotificationMask mask;
    Status status = Status::FAILED;
    mask.set(TNT_CDEV_LEVEL_UPDATE);
    status = manageIndication(procType, isRegister, mask);
    if (status != Status::SUCCESS) {
        PRINT_RESPONSE_FAILURE;
        return;
    }
    PRINT_RESPONSE_SUCCESS;
}

void ThermalTestApp::handleAllUnSolicitedEvents(telux::common::ProcType procType, bool isRegister) {
    telux::therm::ThermalNotificationMask mask;
    Status status = Status::FAILED;
    mask.set();
    status = manageIndication(procType, isRegister, mask);
    if (status != Status::SUCCESS) {
        PRINT_RESPONSE_FAILURE;
        return;
    }
    PRINT_RESPONSE_SUCCESS;
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    thermalTestApp_ = std::make_shared<ThermalTestApp>(APP_NAME, "Therm> ");
    signal(SIGINT, signalHandler);
    // initialize commands and display
    if (!thermalTestApp_->init()) {
        return 1;
    }
    return thermalTestApp_->mainLoop();  // Main loop to continuously read and execute commands
}
