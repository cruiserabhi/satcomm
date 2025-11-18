/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstdio>
#include <iostream>
#include <cstdint>

#include "UserUtils.hpp"

/*
 * Returns true if the user agrees to take given action, else false.
 */
bool UserUtils::getYesNoFromUser(std::string choiceToDisplay) {

    std::string usrInput = "";
    size_t choiceLength;

    while(1) {
        std::cout << choiceToDisplay + " (yes/no): ";

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            return false;
        } else if (!usrInput.compare("yes")) {
            return true;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
        }
    }
}

/*
 *  Returns true if user wants to monitor local subsystem otherwise false.
 */
bool UserUtils::getLocalRemoteFromUser() {

    uint32_t numFromUsr;
    std::string usrInput = "";

    while(1) {
        std::cout << "Enter location (0-local, 1-remote) : ";

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input" << std::endl;
            continue;
        }

        try {
            numFromUsr = (std::stoul(usrInput) & 0xFFFFFFFF);
        } catch (const std::exception& e) {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }

        if ((numFromUsr > 1) || (numFromUsr < 0)) {
            std::cout << "invalid input " << numFromUsr << std::endl;
            continue;
        }

        return (numFromUsr == 0) ? true : false;
    }
}