/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * ApSimProfileMenu provides menu options to interact with Local Profile Assistant(LPA) running
 * on application preocess(AP) to process modem requests for retreiving profile details, enabling
 * or disabling profile on the eUICC.
 */

#include <iostream>

#include "../../common/utils/Utils.hpp"
#include "ApSimProfileMenu.hpp"

ApSimProfileMenu::ApSimProfileMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
    apSimProfileClient_ = std::make_shared<ApSimProfileClient>();
}

ApSimProfileMenu::~ApSimProfileMenu() {
     apSimProfileClient_ = nullptr;
}

bool ApSimProfileMenu::init() {

    std::shared_ptr<ConsoleAppCommand> getProfilesListCommand =
        std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("1", "Retrieve_Available_Profile_List", {},
                std::bind(&ApSimProfileMenu::requestProfileList, this)));
    std::shared_ptr<ConsoleAppCommand> enableProfileCommand =
        std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("2", "Enable_Profile", {},
                std::bind(&ApSimProfileMenu::enableProfile, this)));
    std::shared_ptr<ConsoleAppCommand> disableProfileCommand =
        std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("3", "Disable_Profile", {},
                std::bind(&ApSimProfileMenu::disableProfile, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListApSimProfileMenu =
        {getProfilesListCommand, enableProfileCommand, disableProfileCommand};

    addCommands(commandsListApSimProfileMenu);
    if (!apSimProfileClient_) {
        std::cout << "Invalid ApSimProfile Manager" << std::endl;
        return false;
    }
    if (telux::common::Status::SUCCESS == apSimProfileClient_->init()) {
        ConsoleApp::displayMenu();
    } else {
        std::cout << "Failed to initialize ApSimProfile Manager" << std::endl;
    }
    return true;
}

void ApSimProfileMenu::requestProfileList() {
    std::cout << "\nRetrieve Available Profile list" << std::endl;

    if (!apSimProfileClient_) {
        std::cout << "Invalid ApSimProfile Manager, cannot request eUICC profile list "
            << std::endl;
        return;
    }

    auto ret = apSimProfileClient_->requestProfileList();
    if (ret == telux::common::Status::SUCCESS) {
        std::cout << "Retrieve available profile list sent successfully"
            << std::endl;
    } else {
        std::cout << "Retrieve available profile list failed, status:"
            << int(ret) << std::endl;
        Utils::printStatus(ret);
    }
}

void ApSimProfileMenu::enableProfile() {
    std::cout << "\nEnable Profile" << std::endl;
     if (!apSimProfileClient_) {
         std::cout << "Invalid ApSimProfile Manager, cannot request eUICC profile operation "
             << std::endl;
         return;
    }

    telux::common::Status status = apSimProfileClient_->enableProfile();
    if (status == Status::SUCCESS) {
        std::cout << "Enable profile request sent successfully" << std::endl;
    } else {
        std::cout << "ERROR - Failed to sendProfileOperationResponse, Status:"
            << static_cast<int>(status) << std::endl;
        Utils::printStatus(status);
    }
}

void ApSimProfileMenu::disableProfile() {
    std::cout << "\nDisable Profile" << std::endl;
     if (!apSimProfileClient_) {
        std::cout << "Invalid ApSimProfile Manager, cannot request eUICC profile operation "
            << std::endl;
        return;
    }

    telux::common::Status status = apSimProfileClient_->disableProfile();
    if (status == Status::SUCCESS) {
        std::cout << "Disable profile request sent successfully" << std::endl;
    } else {
        std::cout << "ERROR - Failed to sendProfileOperationResponse, Status:"
            << static_cast<int>(status) << std::endl;
        Utils::printStatus(status);
    }
}
