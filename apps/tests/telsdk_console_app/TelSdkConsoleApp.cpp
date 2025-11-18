/*
 *  Copyright (c) 2017-2020 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       TelSdkConsoleApp.cpp
 *
 * @brief      This is entry class for console application for Telematics SDK,
 *             It allows one to interactively invoke most of the public APIs in the Telematics SDK.
 */

#include <iostream>
#include <memory>

extern "C" {
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
}

#include <telux/common/Version.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "Call/CallMenu.hpp"
#include "ECall/ECallMenu.hpp"
#include "Phone/PhoneMenu.hpp"
#include "Sms/SmsMenu.hpp"
#include "Data/DataMenu.hpp"
#include "SimCardServices/SimCardServicesMenu.hpp"
#include "MultiSim/MultiSimMenu.hpp"
#include "Cellbroadcast/CellbroadcastMenu.hpp"
#include "Rsp/RspMenu.hpp"
#include "ImsSettings/ImsSettingsMenu.hpp"
#include "ImsServingSystem/ImsServingSystemMenu.hpp"
#include "ApSimProfile/ApSimProfileMenu.hpp"
#include "TelSdkConsoleApp.hpp"

#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

TelSdkConsoleApp::TelSdkConsoleApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

TelSdkConsoleApp::~TelSdkConsoleApp() {
}

/**
 * Used for creating a menus of high level features
 */
void TelSdkConsoleApp::init() {
    std::shared_ptr<ConsoleAppCommand> phoneMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Phone_Status", {},
            std::bind(&TelSdkConsoleApp::phoneMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> callMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Dialer", {},
            std::bind(&TelSdkConsoleApp::callMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> eCallMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "eCall", {},
            std::bind(&TelSdkConsoleApp::eCallMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> smsMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "SMS", {}, std::bind(&TelSdkConsoleApp::smsMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> simCardMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Card_Services", {},
            std::bind(&TelSdkConsoleApp::simCardMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> dataMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "Data", {}, std::bind(&TelSdkConsoleApp::dataMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> multiSimMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "MultiSim", {},
        std::bind(&TelSdkConsoleApp::multiSimMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> cbMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "CellBroadcast", {},
            std::bind(&TelSdkConsoleApp::cellbroadcastMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> rspMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "9", "Sim_Profile_Management", {}, std::bind(&TelSdkConsoleApp::rspMenu, this,
               std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> imssMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "IMS_Settings", {},
            std::bind(&TelSdkConsoleApp::imsSettingsMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> imsaMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "IMS_Serving_System", {},
            std::bind(&TelSdkConsoleApp::imsServingSystemMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> apSimProfileMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "12", "AP_Sim_Profile_Management", {}, std::bind(&TelSdkConsoleApp::apSimProfileMenu,
               this, std::placeholders::_1)));
    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
        = {phoneMenuCommand, callMenuCommand, eCallMenuCommand, smsMenuCommand, simCardMenuCommand,
             dataMenuCommand, multiSimMenuCommand, cbMenuCommand, rspMenuCommand, imssMenuCommand,
             imsaMenuCommand, apSimProfileMenuCommand};

    // This instance is needed to hold the audio for the voice call in case the user comes out of
    // dialer menu.
    static std::shared_ptr<AudioClient> audioClient_ = AudioClient::getInstance();
    addCommands(mainMenuCommands);
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::phoneMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    TelSdkConsoleApp::onModemAvailable();
    PhoneMenu phoneMenu("Phone Menu", "phone> ");
    if (phoneMenu.init()) {
       phoneMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::callMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    TelSdkConsoleApp::onModemAvailable();
    CallMenu callMenu("Dialer Menu", "dialer> ");
    if (callMenu.init()) {
       callMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::eCallMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    TelSdkConsoleApp::onModemAvailable();
    ECallMenu eCallMenu("eCall Menu", "eCall> ");
    if (eCallMenu.init()) {
       eCallMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::simCardMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    TelSdkConsoleApp::onModemAvailable();
    SimCardServicesMenu simCardServicesMenu("SIM Card Services Menu", "card_services> ");
    if (simCardServicesMenu.init()) {
       simCardServicesMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::smsMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    TelSdkConsoleApp::onModemAvailable();
    SmsMenu smsMenu("SMS Menu", "sms> ");
    if (smsMenu.init()) {
       smsMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::dataMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_DATA_ENABLED
    DataMenu dataMenu("Data Menu", "data> ");
    dataMenu.init();
    dataMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Data is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::multiSimMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    MultiSimMenu multiSimMenu("MultiSim Menu", "multisim> ");
    if (multiSimMenu.init()) {
       multiSimMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::cellbroadcastMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    CellbroadcastMenu cbMenu("Cellbroadcast Menu", "cb> ");
    if (cbMenu.init()) {
       cbMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::rspMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    RemoteSimProfileMenu rspMenu("Sim Profile Management Menu", "sim_profile_management> ");
    if (rspMenu.init()) {
       rspMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::imsSettingsMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    ImsSettingsMenu imsSettingsMenu("IMS Settings Menu", "ims_settings> ");
    if (imsSettingsMenu.init()) {
       imsSettingsMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::imsServingSystemMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    ImsServingSystemMenu imsaMenu("IMS Serving System Menu", "ims_serving_system> ");
    if (imsaMenu.init()) {
       imsaMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::apSimProfileMenu(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_TEL_ENABLED
    ApSimProfileMenu apSimProfileMenu("AP Sim Profile Management Menu",
        "ap_sim_profile_management> ");
    if (apSimProfileMenu.init()) {
        apSimProfileMenu.mainLoop();
    }
    TelSdkConsoleApp::displayMenu();
#else
    std::cout << "Telephony is unsupported" << std::endl;
#endif
}

void TelSdkConsoleApp::displayMenu() {
    ConsoleApp::displayMenu();
}

#ifdef TELSDK_FEATURE_TEL_ENABLED
void TelSdkConsoleApp::onModemAvailable() {
// Do not perform requestOperatingMode in CV2X machine
// since operating mode cannot be changed
    std::cout << "\n\nChecking telephony subsystem, Please wait!!!..." << std::endl;
    std::shared_ptr<ModemStatus> modemStatus = std::make_shared<ModemStatus>();
    if (modemStatus->init()) {
       modemStatus->printOperatingMode();
    }
}
#endif

// Main function that displays the console and processes user input
int main(int argc, char **argv) {

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        // We can call exit() here if no cleanups needed,
        // or maybe just set a flag, and let the main thread to decide
        // when to exit.
        exit(sig);
    };
    SignalHandler::registerSignalHandler(sigset, cb);
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "Telematics SDK v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch) +"\n" +
                          "Release name: " + sdkReleaseName;
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "radio", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    TelSdkConsoleApp telsdkConsoleApp(appName, "tel_sdk> ");

    telsdkConsoleApp.init();  // initialize commands and display

    return telsdkConsoleApp.mainLoop();  // Main loop to continuously read and execute commands
}
