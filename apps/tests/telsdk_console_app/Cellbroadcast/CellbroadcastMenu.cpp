/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * The reference application to demonstrate Cellbroadcast features
 * like update message filters, request message filters, set and get
 * activation status and receive cell broadcast message
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include <telux/tel/PhoneFactory.hpp>

#include "CellbroadcastMenu.hpp"
#include "CellbroadcastHandler.hpp"
#include "Utils.hpp"
#include <telux/common/DeviceConfig.hpp>

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2
#define INVALID_MSG_ID -1
#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"

CellbroadcastMenu::CellbroadcastMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

CellbroadcastMenu::~CellbroadcastMenu() {
    if (cbListener_) {
        for (auto index = 0; index < cbManagers_.size(); index++) {
            cbManagers_[index]->deregisterListener(cbListener_);
        }
        cbListener_ = nullptr;
    }

    for (auto index = 0; index < cbManagers_.size(); index++) {
        cbManagers_[index] = nullptr;
    }
}

bool CellbroadcastMenu::init() {

    //  Get the PhoneFactory and PhoneManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    int noOfSlots = MIN_SIM_SLOT_COUNT;
    if(telux::common::DeviceConfig::isMultiSimSupported()) {
        noOfSlots = MAX_SIM_SLOT_COUNT;
    }

    cbListener_ = std::make_shared<CellbroadcastListener>();
    for (int index = DEFAULT_SLOT_ID; index <= noOfSlots; index++) {
        std::promise<telux::common::ServiceStatus> cbMgrprom;
        auto cbMgr = phoneFactory.getCellBroadcastManager(static_cast<SlotId>(index),
            [&](telux::common::ServiceStatus status) {
             cbMgrprom.set_value(status);
        });
        if (cbMgr == nullptr) {
            std::cout << "Failed to get CellBroadcast Manager" << std::endl;
            return false;
        }
        //  Check if cellbroadcast subsystem is ready
        telux::common::ServiceStatus cbMgrStatus = cbMgr->getServiceStatus();
        if (cbMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
           std::cout <<
               "CellBroadcast subsystem is not ready, wait for it to be ready on slotId "
               << index << std::endl;
        }
        //  If cellbroadcast subsystem is not ready, wait for it to be ready
        cbMgrStatus = cbMgrprom.get_future().get();
        if (cbMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
           std::cout << "Cellbroadcast Subsystem is ready on slotId " << index << std::endl;
           cbManagers_.emplace_back(cbMgr);
       } else {
           std::cout << " ERROR - Unable to intialize,"
                << " Cellbroadcast Manager subsystem on slotId "
                << index << std::endl;
           return false;
      }
    }

    for (auto index = 0; index < cbManagers_.size(); index++) {
        // add listeners for incoming SMS notification
        auto status = cbManagers_[index]->registerListener(cbListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Unable to register Listener for slotId" << index << std::endl;
            return false;
        }
    }

    std::shared_ptr<ConsoleAppCommand> requestMessageFiltersCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Request_Message_Filters", {},
        std::bind(&CellbroadcastMenu::requestMessageFilters, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> updateMessageFiltersCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Update_Message_Filters", {},
        std::bind(&CellbroadcastMenu::updateMessageFilters, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestActivationCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Request_Activation_Status", {},
        std::bind(&CellbroadcastMenu::requestActivationStatus, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setActivationStatusCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set_Activation_Status", {},
        std::bind(&CellbroadcastMenu::setActivationStatus, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Select_SIM_Slot", {},
        std::bind(&CellbroadcastMenu::selectSimSlot, this, std::placeholders::_1)));
    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListCbMenu
        = { requestMessageFiltersCommand, updateMessageFiltersCommand, requestActivationCommand,
        setActivationStatusCommand
        };

    if (noOfSlots > MIN_SIM_SLOT_COUNT) {
        commandsListCbMenu.emplace_back(selectSimSlotCommand);
    }
    addCommands(commandsListCbMenu);
    ConsoleApp::displayMenu();
    std::cout << "Device is listening for any incoming cell broadcast messages" << std::endl;
    return true;
}

void CellbroadcastMenu::requestMessageFilters(std::vector<std::string> userInput) {

    auto cbManager = cbManagers_[slot_ - 1];

    if(cbManager) {
        telux::common::Status status = cbManager->requestMessageFilters(
            CellbroadcastCallbackHandler::requestMsgFilterResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request for message filters sent successfully" << std::endl;
        } else {
            std::cout << "Request for message filters failed, status:" <<
                static_cast<int>(status) << std::endl;
        }
    } else {
        std::cout << "ERROR - CellBroadcastManger is null for slot: " << slot_ << std::endl;
    }
}

void CellbroadcastMenu::updateMessageFilters(std::vector<std::string> userInput) {

    auto cbManager = cbManagers_[slot_ - 1];

    std::string noOfFilters = "";
    std::string fromId = "";
    std::string toId = "";
    int noOfMsgIds = 0;
    int fromMsgId = INVALID_MSG_ID;
    int toMsgId = INVALID_MSG_ID;
    std::vector<telux::tel::CellBroadcastFilter> filterList = {};
    char delimiter = '\n';

    std::cout << "Enter number of message filters to update: ";
    std::getline(std::cin, noOfFilters, delimiter);
    if(!noOfFilters.empty()) {
        try {
            noOfMsgIds = std::stoi(noOfFilters);
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << noOfMsgIds
                    << std::endl;
        }
    }
    for(int i = 0; i < noOfMsgIds; i++) {
        std::cout << "Enter FROM message ID: ";
        std::getline(std::cin, fromId, delimiter);
        if(!fromId.empty()) {
            try {
                fromMsgId = std::stoi(fromId);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << fromMsgId
                        << std::endl;
            }
        }
        std::cout << "Enter TO message ID: ";
        std::getline(std::cin, toId, delimiter);
        if(!toId.empty()) {
            try {
                toMsgId = std::stoi(toId);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << toMsgId
                        << std::endl;
            }
        }

        telux::tel::CellBroadcastFilter filter = {};
        filter.startMessageId = fromMsgId;
        filter.endMessageId = toMsgId;
        filterList.emplace_back(filter);
    }

    if(cbManager) {
        telux::common::Status status = cbManager->updateMessageFilters(filterList,
            CellbroadcastCallbackHandler::updateMsgFilterResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Update message filters sent successfully" << std::endl;
        } else {
            std::cout << "Update message filters  failed, status:" <<
                static_cast<int>(status) << std::endl;
        }
    } else {
        std::cout << "ERROR - CellBroadcastManger is null for slot: " << slot_ << std::endl;
    }
}

void CellbroadcastMenu::setActivationStatus(std::vector<std::string> userInput) {

    auto cbManager = cbManagers_[slot_ - 1];

    char delimiter = '\n';
    std::string isActivate = "";
    bool activate = false;
    std::cout << "Activate message ids: (1-Activate 0-De-activate) ";
        std::getline(std::cin, isActivate, delimiter);
        if(!isActivate.empty()) {
            try {
                activate = std::stoi(isActivate);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << activate
                        << std::endl;
            }
        }

    if(cbManager) {
        telux::common::Status status = cbManager->setActivationStatus(activate,
            CellbroadcastCallbackHandler::setActivationStatusResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Set Activation status request sent successfully" << std::endl;
        } else {
            std::cout << "Set Activation status request failed, status:" <<
                static_cast<int>(status) << std::endl;
        }
    } else {
        std::cout << "ERROR - CellBroadcastManger is null for slot: " << slot_ << std::endl;
    }
}

void CellbroadcastMenu::requestActivationStatus(std::vector<std::string> userInput) {
    auto cbManager = cbManagers_[slot_ - 1];

    if(cbManager) {
        telux::common::Status status = cbManager->requestActivationStatus(
            CellbroadcastCallbackHandler::requestActivationStatusResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request for activation status sent successfully" << std::endl;
        } else {
            std::cout << "Request for activation status failed, status: " <<
                static_cast<int>(status) << std::endl;
        }
    } else {
        std::cout << "ERROR - CellBroadcastManger is null for slot: " << slot_ << std::endl;
    }
}

void CellbroadcastMenu::selectSimSlot(std::vector<std::string> userInput) {
   std::string slotSelection;
   char delimiter = '\n';

   std::cout << "Enter the desired SIM slot (1-Primary, 2-Secondary): ";
   std::getline(std::cin, slotSelection, delimiter);

   if (!slotSelection.empty()) {
      try {
         int slot = std::stoi(slotSelection);
         if (slot > MAX_SIM_SLOT_COUNT || slot < MIN_SIM_SLOT_COUNT) {
            std::cout << "Invalid slot entered, using default slot" << std::endl;
            slot_ = DEFAULT_SLOT_ID;
         } else {
            slot_ = slot;
         }
      } catch (const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
            << slotSelection << std::endl;
         return;
      }
   } else {
      std::cout << "Empty input, enter the correct slot" << std::endl;
   }
}
