/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <cstdio>
#include <future>

#include <telux/platform/diag/DiagnosticsFactory.hpp>
#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"

#include "DiagODLApp.hpp"
#include "CallbackMethod.hpp"
#include "FileMethod.hpp"

/*
 * Constructor.
 */
DiagODLApp::DiagODLApp(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

/*
 * Destructor.
 */
DiagODLApp::~DiagODLApp() {
}

/*
 * Prepare file method menu and display on the console.
 */
void DiagODLApp::fileMethodMenu() {
    std::shared_ptr<FileMethod> fileMenu;

    try {
        fileMenu = std::make_shared<FileMethod>("File method", "file> ", diagMgr_);
    } catch (const std::exception& e) {
        std::cout << "can't allocate FileMethod" << std::endl;
        return;
    }

    fileMenu->showFileMenu();
}

/*
 *  Prepare callback method menu and display on the console.
 */
void DiagODLApp::callbackMethodMenu() {
    int ret;
    std::shared_ptr<CallbackMethod> cbMenu;

    try {
        cbMenu = std::make_shared<CallbackMethod>("Callback method", "callback> ", diagMgr_);
    } catch (const std::exception& e) {
        std::cout << "can't allocate CallbackMethod" << std::endl;
        return;
    }

    ret = cbMenu->initCallbackMethod();
    if (ret < 0) {
        return;
    }

    cbMenu->showCallbackMenu();
}

/*
 * Prepare main menu and display on the console.
 */
void DiagODLApp::showMainMenu() {

    std::shared_ptr<ConsoleAppCommand> fileMethod = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "File method", {},
        std::bind(&DiagODLApp::fileMethodMenu, this)));

    std::shared_ptr<ConsoleAppCommand> callbackMethod = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Callback method", {},
        std::bind(&DiagODLApp::callbackMethodMenu, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        fileMethod, callbackMethod
    };

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

/*
 * Allocate IDiagLogManager and initialize service.
 */
int DiagODLApp::init() {
    telux::common::ServiceStatus srvStatus{};
    std::promise<telux::common::ServiceStatus> p{};

    auto &diagFactory = telux::platform::diag::DiagnosticsFactory::getInstance();

    diagMgr_ = diagFactory.getDiagLogManager(
        [&p](telux::common::ServiceStatus srvStatus) {
            p.set_value(srvStatus);
    });

    if (!diagMgr_) {
        std::cout << "Can't get IDiagLogManager" << std::endl;
        return -ENOMEM;
    }

    srvStatus = p.get_future().get();
    if (srvStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Diag service unavailable, status " <<
            static_cast<int>(srvStatus) << std::endl;
        return -EIO;
    }

    return 0;
}

/*
 * Application entry.
 */
int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();

    std::string appName = "Diag ODL console app - SDK v"
                            + std::to_string(sdkVersion.major) + "."
                            + std::to_string(sdkVersion.minor) + "."
                            + std::to_string(sdkVersion.patch) + "\n"
                            + "Release name: " + sdkReleaseName;

    auto diagOdlApp = std::make_shared<DiagODLApp>(appName, "diag> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};

    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    diagOdlApp->init();

    diagOdlApp->showMainMenu();
    return diagOdlApp->mainLoop();
}
