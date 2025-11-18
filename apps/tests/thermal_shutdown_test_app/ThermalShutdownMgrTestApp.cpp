/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a sample program to register and receive auto-shutdown mode updates, send commands to
 * change the auto-shutdown mode
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <csignal>
#include <mutex>
#include <condition_variable>

extern "C" {
#include "unistd.h"
}

#include "ThermalShutdownMgrTestApp.hpp"
#include "../../common/utils/Utils.hpp"

void ThermalShutdownTestApp::printHelp() {

    std::cout << "Usage: " << APP_NAME << " options" << std::endl;
    std::cout << "   -l --listen            : listen to Auto shutdown mode updates" << std::endl;
    std::cout << "   -e --enable            : send ENABLE command" << std::endl;
    std::cout << "   -d --disable           : send DISABLE command" << std::endl;
    std::cout << "   -g --get-status        : get Auto shutdown mode" << std::endl;
    std::cout << "   -c --console-mode      : open interactive console" << std::endl;
    std::cout << "   -h --help              : print the help menu" << std::endl;
}

Status ThermalShutdownTestApp::parseArguments(int argc, char **argv) {
    int arg;

    while(1) {
        static struct option long_options[] = {
            {"listen",          no_argument, 0, 'l'},
            {"enable",          no_argument, 0, 'e'},
            {"disable",         no_argument, 0, 'd'},
            {"get-status",      no_argument, 0, 'g'},
            {"console-mode",    no_argument, 0, 'c'},
            {"help",            no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        int opt_index = 0;
        arg = getopt_long(argc, argv, "ledgch", long_options, &opt_index);
        if(arg == -1) {
            break;
        }
        switch(arg) {
            case 'l':
                listenerEnabled_ = true;
                break;
            case 'e':
                setCommand_ = AutoShutdownMode::ENABLE;
                break;
            case 'd':
                setCommand_ = AutoShutdownMode::DISABLE;
                break;
            case 'g':
                getCommand_ = true;
                break;
            case 'c':
                isConsole_ = true;
                break;
            case 'h':
                printHelp();
                break;
            default:
                printHelp();
                return Status::INVALIDPARAM;
        }
    }
    return Status::SUCCESS;
}

void ThermalShutdownTestApp::handleArguments() {

    if(setCommand_ != AutoShutdownMode::UNKNOWN) {
        myThermCmdMgr_->sendAutoShutdownModeCommand(setCommand_);
        myThermCmdMgr_->registerForUpdates();
        auto status = myThermCmdMgr_->cmdRspCb_->commandResponseStatus();

        if (status.get()) {
            std::cout << " Waiting for async task to complete " << std::endl;
            myThermCmdMgr_->waitForAsyncTaskToComplete();
        }
        myThermCmdMgr_->deregisterForUpdates();
    }
    if(getCommand_) {
        auto f = myThermCmdMgr_->getAutoShutdownModeCommand();
        f.wait();
    }
    if (isConsole_) {
        myThermCmdMgr_->registerForUpdates();
        consoleinit();
        mainLoop();
        myThermCmdMgr_->deregisterForUpdates();
    }
    if(listenerEnabled_) {
        myThermCmdMgr_->registerForUpdates();
        std::unique_lock<std::mutex> lock(mtx_);
        std::cout << APP_NAME << " Press CTRL+C to exit" << std::endl;
        cv_.wait(lock, [this](){return exiting_;});
        if(listenerEnabled_) {
            myThermCmdMgr_->deregisterForUpdates();
        }
    }
    return;
}

ThermalShutdownTestApp::ThermalShutdownTestApp()
    : ConsoleApp("Thermal Shutdown-Management Menu", "thrml-shtdwn-mgmt> ")
    , myThermCmdMgr_(nullptr)
    , exiting_(false)
    , listenerEnabled_(false)
    , setCommand_(AutoShutdownMode::UNKNOWN)
    , getCommand_(false)
    , isConsole_(false) {
}

ThermalShutdownTestApp::~ThermalShutdownTestApp() {
}

ThermalShutdownTestApp & ThermalShutdownTestApp::getInstance() {
    static ThermalShutdownTestApp instance;
    return instance;
}

bool ThermalShutdownTestApp::listnerEnableStatus() {
    return listenerEnabled_;
}

void signalHandler(int signum)
{
    ThermalShutdownTestApp::getInstance().signalHandler(signum);
}

void ThermalShutdownTestApp::signalHandler( int signum ) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    exiting_ = true;
    cv_.notify_all();
}

int ThermalShutdownTestApp::init() {

    myThermCmdMgr_ = std::make_shared<ThermalCommandMgr>();
    int rc = myThermCmdMgr_->init();
    if(rc) {
        return -1;
    }
    return 0;
}

void ThermalShutdownTestApp::consoleinit() {

   std::shared_ptr<ConsoleAppCommand> getAutoShutdownmode
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Get auto shutdown mode", {},
         std::bind(&ThermalCommandMgr::getAutoShutdownModeCommand, myThermCmdMgr_)));

   std::shared_ptr<ConsoleAppCommand> disableAutoShutdownCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Disable auto shutdown mode", {},
         std::bind(&ThermalCommandMgr::sendAutoShutdownModeCommand, myThermCmdMgr_,
                   AutoShutdownMode::DISABLE)));

   std::shared_ptr<ConsoleAppCommand> enableAutoShutdownCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Enable auto shutdown mode", {},
         std::bind(&ThermalCommandMgr::sendAutoShutdownModeCommand, myThermCmdMgr_,
                    AutoShutdownMode::ENABLE)));

   std::shared_ptr<ConsoleAppCommand> recurringdisableAutoShutdownmode
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Always disable auto shutdown mode", {},
         std::bind(&ThermalCommandMgr::sendRecurringDisableCommand, myThermCmdMgr_)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListThermalMenu
      = {getAutoShutdownmode, disableAutoShutdownCommand, enableAutoShutdownCommand,
          recurringdisableAutoShutdownmode};
   ConsoleApp::addCommands(commandsListThermalMenu);
   ConsoleApp::displayMenu();
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {

    Status ret = Status::FAILED;
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
    }
    auto &ThermMgmtTest = ThermalShutdownTestApp::getInstance();

    if( 0 != ThermMgmtTest.init()) {
        std::cout << APP_NAME <<
            " Failed to initialize the Thermal-Shutdown management service" << std::endl;
        return -1;
    }
    ret = ThermMgmtTest.parseArguments(argc, argv);
    if(ret !=Status::SUCCESS) {
        return -1;
    }
    signal(SIGINT, signalHandler);
    ThermMgmtTest.handleArguments();
    std::cout << "Exiting application..." << std::endl;
    return 0;
}
