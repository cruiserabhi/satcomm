/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * This is a test application to register and receive Antenna related events
 * The application can can be controlled via console
 */

#include <getopt.h>
#include <iostream>
#include <vector>
#include <csignal>

extern "C" {
#include "unistd.h"
}

#include "AntennaTestApp.hpp"
#include "Utils.hpp"

std::shared_ptr<AntennaTestApp> antennaTestApp_ = nullptr;

AntennaTestApp::AntennaTestApp()
   : ConsoleApp("Antenna Management Menu", "ant-mgmt> ") {
}

AntennaTestApp::~AntennaTestApp() {
    cleanup();
}

void signalHandler(int signum) {
    antennaTestApp_->signalHandler(signum);
}

void AntennaTestApp::signalHandler(int signum) {
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    cleanup();
    exit(1);
}

int AntennaTestApp::init() {
    // Get platform factory instance
    auto &platFormFactory = PlatformFactory::getInstance();
    // Get antenna manager object
    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
    antMgr_ = platFormFactory.getAntennaManager(
        [&](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (antMgr_ == NULL) {
        std::cout << APP_NAME << " *** ERROR - Failed to get antenna manager" << std::endl;
        return -1;
    }
    // Check antenna management service status
    std::cout << " Waiting for antenna manager to be ready " << std::endl;
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << APP_NAME << " antenna manager is ready !" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Unable to initialize antenna manager"
                  << std::endl;
        return -1;
    }
    antListener_ = std::make_shared<MyAntennaListener>();
    if (!antMgr_) {
        std::cout << APP_NAME << "*** ERROR - Invalid instance of antenna manager !"
                  << std::endl;
        return -1;
    }
    telux::common::Status status = antMgr_->registerListener(antListener_);
    if ((status == telux::common::Status::SUCCESS) || (status == telux::common::Status::ALREADY)) {
        std::cout << APP_NAME << " Registered for antenna events" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Failed to register for antenna events: ";
        Utils::printStatus(status);
    }
    return 0;
}

void AntennaTestApp::cleanup() {
    // De-registering a listener from antenna operation updates
    if (!antMgr_) {
        std::cout << APP_NAME << "*** ERROR - Invalid instance of antenna manager !"
                  << std::endl;
        return;
    }
    telux::common::Status status = antMgr_->deregisterListener(antListener_);
    if ((status == telux::common::Status::SUCCESS) || (status == telux::common::Status::NOSUCH)) {
        std::cout << APP_NAME << " Deregistered antenna listener successfully" << std::endl;
    } else {
        std::cout << APP_NAME
                  << " *** ERROR - Failed to deregister antenna listener: " << std::endl;
        Utils::printStatus(status);
    }
}

void AntennaTestApp::consoleinit() {
   std::shared_ptr<ConsoleAppCommand> setAntConfigCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "Set_antenna_config", {},
                        std::bind(&AntennaTestApp::setAntConfig, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> getAntConfigCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "Get_antenna_config", {},
                        std::bind(&AntennaTestApp::getAntConfig, this, std::placeholders::_1)));
    std::vector<std::shared_ptr<ConsoleAppCommand>> antTestAppCommands
        = {setAntConfigCommand, getAntConfigCommand};

    ConsoleApp::addCommands(antTestAppCommands);
    ConsoleApp::displayMenu();
}

void AntennaTestApp::setAntConfig(std::vector<std::string> userInput) {
    if(antMgr_) {
        int antIndex = -1;
        char delimiter = '\n';
        std::string temp = "";
        std::cout << "Enter antenna index: ";
        std::getline(std::cin, temp, delimiter);
        if(!temp.empty()) {
            try {
                antIndex = std::stoi(temp);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values." << std::endl;
            }
        } else {
            std::cout << "No input" << std::endl;
            return;
        }
        std::promise<telux::common::ErrorCode> p;
        auto status = antMgr_->setActiveAntenna(
            antIndex, [&p, this](telux::common::ErrorCode error) { p.set_value(error); });
        if (status == Status::SUCCESS) {
            std::cout << APP_NAME << "Set antenna config sent successfully\n";
            telux::common::ErrorCode error = p.get_future().get();
            std::cout << "Set antenna config request executed with result: "
                << Utils::getErrorCodeAsString(error) << std::endl;
        } else {
            std::cout << APP_NAME << "ERROR - Failed to set antenna config,"
                      << "Status:" << static_cast<int>(status) << "\n";
            Utils::printStatus(status);
        }
    } else {
        std::cout << "*** ERROR - Invalid instance of antenna manager !";
    }
}

void AntennaTestApp::getAntConfig(std::vector<std::string> userInput) {
    if(antMgr_) {
        std::promise<telux::common::ErrorCode> p;
        int index = -1;
        auto status = antMgr_->getActiveAntenna(
            [&p, &index, this](int antIndex, telux::common::ErrorCode error)
            { index = antIndex; p.set_value(error); });
        if (status == Status::SUCCESS) {
            std::cout << APP_NAME << "Get antenna config sent successfully\n";
            telux::common::ErrorCode error = p.get_future().get();
            std::cout << "Get antenna config request executed with result: "
                << Utils::getErrorCodeAsString(error)
                << ", current active antenna index: " << index << std::endl;
        } else {
            std::cout << APP_NAME << "ERROR - Failed to get antenna config,"
                      << "Status:" << static_cast<int>(status) << "\n";
            Utils::printStatus(status);
        }
    } else {
        std::cout << "*** ERROR - Invalid instance of antenna manager !";
    }
}

/**
 * Main routine
 */
int main(int argc, char **argv) {
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
    }
    antennaTestApp_ = std::make_shared<AntennaTestApp>();
    if (0 != antennaTestApp_->init()) {
        std::cout << APP_NAME << " Failed to initialize the antenna management service"
                  << std::endl;
        return -1;
    }
    signal(SIGINT, signalHandler);
    antennaTestApp_->consoleinit();
    antennaTestApp_->mainLoop();
    std::cout << "Exiting application..." << std::endl;
    antennaTestApp_->cleanup();
    antennaTestApp_ = nullptr;
    return 0;
}
