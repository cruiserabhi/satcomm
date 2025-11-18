/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstdio>

#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"

#include "CryptoAcceleratorApp.hpp"
#include "CryptoOperationMenu.hpp"

CryptoAcceleratorApp::CryptoAcceleratorApp(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

CryptoAcceleratorApp::~CryptoAcceleratorApp() {
}

void CryptoAcceleratorApp::cryptoOperationMenu(telux::sec::Mode mode) {

    telux::common::ErrorCode ec;
    std::shared_ptr<CryptoOperationMenu> operationMenu;

    switch (mode) {
        case telux::sec::Mode::MODE_SYNC:
            try {
                operationMenu = std::make_shared<
                    CryptoOperationMenu>("Crypto Operation", "sync> ");
            } catch (const std::exception& e) {
                std::cout << "can't create CryptoOperationMenu" << std::endl;
                return;
            }
            break;
        case telux::sec::Mode::MODE_ASYNC_POLL:
            try {
                operationMenu = std::make_shared<
                    CryptoOperationMenu>("Crypto Operation", "async poll> ");
            } catch (const std::exception& e) {
                std::cout << "can't create CryptoOperationMenu" << std::endl;
                return;
            }
            break;
        case telux::sec::Mode::MODE_ASYNC_LISTENER:
            try {
                operationMenu = std::make_shared<
                    CryptoOperationMenu>("Crypto Operation", "async listener> ");
            } catch (const std::exception& e) {
                std::cout << "can't create CryptoOperationMenu" << std::endl;
                return;
            }
            break;
        default:
            std::cout << "invalid mode " << static_cast<int>(mode) << std::endl;
            return;
    }

    ec = operationMenu->init(mode);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't init, err: " << static_cast<int>(ec) << std::endl;
        return;
    }

    operationMenu->mainLoop();
}

/*
 *  Prepare menu and display on console.
 */
void CryptoAcceleratorApp::init() {

    std::shared_ptr<ConsoleAppCommand> syncMode = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Sync mode", {},
        std::bind(&CryptoAcceleratorApp::cryptoOperationMenu, this,
        telux::sec::Mode::MODE_SYNC)));

    std::shared_ptr<ConsoleAppCommand> asyncMode = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Async listener mode", {},
        std::bind(&CryptoAcceleratorApp::cryptoOperationMenu, this,
        telux::sec::Mode::MODE_ASYNC_LISTENER)));

    std::shared_ptr<ConsoleAppCommand> asyncPollMode = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("3", "Async poll mode", {},
        std::bind(&CryptoAcceleratorApp::cryptoOperationMenu, this,
        telux::sec::Mode::MODE_ASYNC_POLL)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        syncMode, asyncMode, asyncPollMode
    };

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "Crypto accelerator console app - SDK v"
                            + std::to_string(sdkVersion.major) + "."
                            + std::to_string(sdkVersion.minor) + "."
                            + std::to_string(sdkVersion.patch) + "\n"
                            + "Release name: " + sdkReleaseName;

    auto cryptApp = std::make_shared<CryptoAcceleratorApp>(appName, "crptoaccelerator> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "mvm", "logd", "dlt"};

    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    cryptApp->init();

    return cryptApp->mainLoop();
}
