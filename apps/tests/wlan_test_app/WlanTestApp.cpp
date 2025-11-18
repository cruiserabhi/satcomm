/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanTestApp.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanTestApp::WlanTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

WlanTestApp::~WlanTestApp() {
    wlanDeviceManagerMenu_ = nullptr;
    wlanApInterfaceManagerMenu_ = nullptr;
}

bool WlanTestApp::initWlan() {
    if (wlanDeviceManagerMenu_ == nullptr) {
        wlanDeviceManagerMenu_ =
            std::make_shared<WlanDeviceManagerMenu>("Device Manager Menu", "device> ");
    }
    if (wlanDeviceManagerMenu_->isSubSystemReady()) {
        std::cout << "Wlan Subsystem is Ready" << std::endl;
        wlanApInterfaceManagerMenu_ =
            std::make_shared<WlanApInterfaceManagerMenu>("Ap Interface Manager Menu", "ap> ");
        if(wlanApInterfaceManagerMenu_) {
            wlanApInterfaceManagerMenu_->init() ;
        }
        wlanStaInterfaceManagerMenu_ =
            std::make_shared<WlanStaInterfaceManagerMenu>("Station Interface Manager Menu", "sta> ");
        if(wlanStaInterfaceManagerMenu_) {
            wlanStaInterfaceManagerMenu_->init();
        }
        return true;
    }
    return false;
}

bool WlanTestApp::init() {
    if (initWlan()) {
        std::shared_ptr<ConsoleAppCommand> wlanDeviceManagerMenu =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "device_manager_Menu",
            {}, std::bind(&WlanTestApp::wlanDeviceManagerMenu, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> wlanApInterfaceManagerMenu =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "ap_interface_manager_menu",
            {}, std::bind(&WlanTestApp::wlanApInterfaceManagerMenu, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> wlanStaInterfaceManagerMenu =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "sta_interface_manager_menu",
            {}, std::bind(&WlanTestApp::wlanStaInterfaceManagerMenu, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {wlanDeviceManagerMenu,
            wlanApInterfaceManagerMenu, wlanStaInterfaceManagerMenu};

        addCommands(commandsList);
        ConsoleApp::displayMenu();
        return true;
    }
    else {
        return false;
    }
}

void WlanTestApp::wlanDeviceManagerMenu(std::vector<std::string> inputCommand) {
    if (wlanDeviceManagerMenu_->init()) {
        wlanDeviceManagerMenu_->mainLoop();
    }
    ConsoleApp::displayMenu();
}

void WlanTestApp::wlanApInterfaceManagerMenu(std::vector<std::string> inputCommand) {
    if (wlanApInterfaceManagerMenu_) {
        wlanApInterfaceManagerMenu_->showMenu();
        wlanApInterfaceManagerMenu_->mainLoop();
    }
    ConsoleApp::displayMenu();
}

void WlanTestApp::wlanStaInterfaceManagerMenu(std::vector<std::string> inputCommand) {
    if (wlanStaInterfaceManagerMenu_) {
        wlanStaInterfaceManagerMenu_->showMenu();
        wlanStaInterfaceManagerMenu_->mainLoop();
    }
    ConsoleApp::displayMenu();
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Wlan Test App: Adding supplementary groups failed!" << std::endl;
    }
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string appName = "Wlan Test App v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch);
    WlanTestApp wlanTestApp(appName, "Wlan> ");
    if (wlanTestApp.init()) {       // initialize commands and display
        wlanTestApp.mainLoop();     // Main loop to continuously read and execute commands
    }
    return 0;
}
