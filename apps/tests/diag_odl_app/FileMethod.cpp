/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <errno.h>

#include <iostream>

#include "common/console_app_framework/ConsoleApp.hpp"
#include "FileMethod.hpp"

/*
 * Constructor.
 */
FileMethod::FileMethod(std::string menuTitle, std::string cursor,
    std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr)
    : CollectionMethod(menuTitle, cursor, diagMgr) {
}

/*
 * Destructor.
 */
FileMethod::~FileMethod() {
}

/*
 * Prepare options applicable for file method collection and display them.
 */
void FileMethod::showFileMenu() {
    std::shared_ptr<ConsoleAppCommand> setConfig = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Set configuration", {},
        std::bind(&FileMethod::setConfig, this,
        telux::platform::diag::LogMethod::FILE)));

    std::shared_ptr<ConsoleAppCommand> getConfig = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Get configuration", {},
        std::bind(&FileMethod::getConfig, this)));

    std::shared_ptr<ConsoleAppCommand> startCollection = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("3", "Start log collection", {},
        std::bind(&FileMethod::startCollection, this)));

    std::shared_ptr<ConsoleAppCommand> stopCollection = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("4", "Stop log collection", {},
        std::bind(&FileMethod::stopCollection, this)));

    std::shared_ptr<ConsoleAppCommand> getSrvStatus = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("5", "Get service status", {},
        std::bind(&FileMethod::getServiceStatus, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> fileCmds = {setConfig,
        getConfig, startCollection, stopCollection, getSrvStatus};

    ConsoleApp::addCommands(fileCmds);
    ConsoleApp::displayMenu();
    shared_from_this()->mainLoop();
}
