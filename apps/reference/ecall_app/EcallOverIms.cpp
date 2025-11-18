/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file  EcallOverIms.cpp
 *
 * @brief EcallOverImsMenu class provides an user-interactive console to trigger an custom number
 *        eCall and update MSD over IMS.
 */

#include <iostream>
#include <algorithm>
#include <csignal>

#include <telux/tel/PhoneDefines.hpp>

#include "EcallOverIms.hpp"
#include "ECallApp.hpp"
#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"


EcallOverImsMenu::EcallOverImsMenu(std::shared_ptr<ECallManager> eCallManager, std::string appName,
                                   std::string cursor)
                                   : ConsoleApp(appName, cursor) , eCallMgr_(eCallManager) {
}

EcallOverImsMenu::~EcallOverImsMenu() {
}

/**
 * Initialize console commands and Display
 */
void EcallOverImsMenu::init() {

    std::shared_ptr<ConsoleAppCommand> customNumberECallOverImsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Custom_Number_ECall_Over_Ims",
            {}, std::bind(&EcallOverImsMenu::makeCustomNumberECallOverIms, this)));

    std::shared_ptr<ConsoleAppCommand> updateeCallMsdOverImsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2",
            "Update_MSD_Custom_Number_ECall_Over_Ims", {},
            std::bind(&EcallOverImsMenu::updateCustomNumberECallOverIms, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
        = {customNumberECallOverImsCommand, updateeCallMsdOverImsCommand};
    addCommands(commandsList);
    ConsoleApp::displayMenu();
}

/**
 * Trigger a Tps ecall over IMS
 *
 */
void EcallOverImsMenu::makeCustomNumberECallOverIms() {
    std::shared_ptr<ECallManager> eCallManager = eCallMgr_.lock();
    if (!eCallManager) {
        std::cout << "Invalid eCall Manager, cannot trigger eCall" << std::endl;
        return;
    }
    // Get input from user to make a TPS ecall over IMS or update the MSD
    char delimiter = '\n';
    std::string temp = "";
        // Get phone number from user
        std::string dialNumber = "";
        std::cout << "Enter phone number: ";
        std::getline(std::cin, dialNumber, delimiter);
        if (dialNumber.empty()) {
            std::cout << "No input, please provide a valid phone number" << std::endl;
            return;
        }
        // Get phoneId from user
        int phoneId = ECallApp::getPhoneId();
        // Get Optional SIP headers from user
        std::string contentType = "";
        std::string acceptInfo = "";
        getOptionalSIPHeader(contentType, acceptInfo);
        std::cout << "Custom number eCall over IMS Triggered" << std::endl;
        auto ret = eCallManager->triggerECall(phoneId, dialNumber, contentType, acceptInfo);
        if (ret != telux::common::Status::SUCCESS) {
            std::cout << "ECall request failed" << std::endl;
        } else {
            std::cout << "ECall request is successful" << std::endl;
        }
}
void EcallOverImsMenu::updateCustomNumberECallOverIms() {
    std::shared_ptr<ECallManager> eCallManager = eCallMgr_.lock();
    if (!eCallManager) {
        std::cout << "Invalid eCall Manager, cannot trigger eCall" << std::endl;
        return;
    }
    auto ret = eCallManager->updateEcallMSD();
    if (ret != telux::common::Status::SUCCESS) {
        std::cout << "ECall request failed" << std::endl;
    } else {
        std::cout << "ECall request is successful" << std::endl;
    }
}

/**
 * Executes any cleanup procedure if necessary
 */
void EcallOverImsMenu::cleanup() {
    std::cout << "Exiting the application.." << std::endl;
}

void EcallOverImsMenu::getOptionalSIPHeader(std::string &contentTypeHeader,
                                            std::string &acceptInfoHeader) {
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter Custom SIP Header for contentType (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        contentTypeHeader = temp;
    } else {
        std::cout << "No input, proceeding with contentType: " << std::endl;
    }
    temp = "";
    std::cout << "Enter Custom SIP Header for acceptInfo (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        acceptInfoHeader = temp;
    } else {
        std::cout << "No input, proceeding with contentType: " << std::endl;
    }
}

