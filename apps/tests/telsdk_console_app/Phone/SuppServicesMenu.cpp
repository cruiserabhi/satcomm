/*
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <future>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "SuppServicesMenu.hpp"
#include "SuppServicesHandler.hpp"
#include "Utils.hpp"
#include "../../common/utils/Utils.hpp"

#define INPUT_ACTIVATE 1
#define INPUT_DEACTIVATE 2
#define INPUT_REGISTER 3
#define INPUT_ERASE 4
#define INPUT_UNCONDITIONAL 1
#define INPUT_BUSY 2
#define INPUT_NO_REPLY 3
#define INPUT_NOT_REACHABLE 4
#define INPUT_NOT_LOGGED_IN 23
#define SLOT_COUNT_1 1
#define SLOT_COUNT_2 2
#define MAX_INPUT_NO_REPLY 3
#define MIN_NO_REPLY_TIMER 0
#define MAX_NO_REPLY_TIMER 255

using namespace telux::common;
using namespace telux::tel;

SuppServicesMenu::SuppServicesMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

SuppServicesMenu::~SuppServicesMenu() {
    for (auto mgr : suppServicesManagers_) {
        mgr = nullptr;
    }
}

bool SuppServicesMenu::init() {
    //  Get the PhoneFactory and Supplementary Services Manager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    int slotCount = SLOT_COUNT_1;
    if (DeviceConfig::isMultiSimSupported()) {
        slotCount = SLOT_COUNT_2;
    }
    for (auto index = 1; index <= slotCount; index++) {
        std::promise<telux::common::ServiceStatus> prom;
        auto suppServicesManager = phoneFactory.getSuppServicesManager(static_cast<SlotId>(index),
            [&](telux::common::ServiceStatus status) {
            prom.set_value(status);
        });
        if (!suppServicesManager) {
            std::cout << "ERROR - Failed to get supplementary service manager instance \n";
            return false;
        }
        std::cout << " Waiting for supplementary service manager to be ready on slot id "
            << index << std::endl;
        telux::common::ServiceStatus subSystemStatus = prom.get_future().get();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "supplemetary subsystem is ready on slot " << index << std::endl;
            suppServicesManagers_.emplace_back(suppServicesManager);
        }
    }

    std::shared_ptr<ConsoleAppCommand> setCallWaitingPrefCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "1", "Set_call_waiting_pref", {},
            std::bind(&SuppServicesMenu::setCallWaitingPref, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getCallWaitingPrefCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "2", "Get_call_waiting_pref", {},
            std::bind(&SuppServicesMenu::getCallWaitingPref, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setCallForwardingPrefCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "3", "Set_call_forwarding_pref", {},
            std::bind(&SuppServicesMenu::setCallForwardingPref, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getCallForwardingPrefCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "Get_call_forwarding_pref", {},
            std::bind(&SuppServicesMenu::getCallForwardingPref, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setOirPrefCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "Set_OIR_pref", {},
            std::bind(&SuppServicesMenu::setOirPref, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getOirPrefCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "Get_OIR_pref", {},
            std::bind(&SuppServicesMenu::getOirPref, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "7", "Select_sim_slot", {},
            std::bind(&SuppServicesMenu::selectSimSlot, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListSuppServicesMenu
          = {setCallWaitingPrefCmd, getCallWaitingPrefCmd,
             setCallForwardingPrefCmd, getCallForwardingPrefCmd,
             setOirPrefCmd, getOirPrefCmd};
    if (suppServicesManagers_.size() > 1) {
        commandsListSuppServicesMenu.emplace_back(selectSimSlotCommand);
    }
    addCommands(commandsListSuppServicesMenu);
    ConsoleApp::displayMenu();
    return true;
}

void SuppServicesMenu::setCallWaitingPref(std::vector<std::string> userInput) {
    auto suppServicesManager = suppServicesManagers_[slot_ - 1];
    int pref = -1;
    std::cout << "Enter Call Waiting Pref (1-Enable, 2-Disable) :";
    std::cin >> pref;
    Utils::validateInput(pref);
    if (pref == 1 || pref == 2) {
        if (suppServicesManager) {
            auto ret = suppServicesManager->setCallWaitingPref(
                static_cast<SuppServicesStatus>(pref),
                SetSuppSvcResponseCallback::setSuppSvcResp);
            if (ret == telux::common::Status::SUCCESS) {
                std::cout << "\nSet call waiting preference request sent successfully\n";
            } else {
                std::cout << "\nSet call waiting preference request failed \n";
            }
        } else {
            std::cout << "Invalid Manager Object" << std::endl;
        }
    } else {
        std::cout << "Invalid Input" << std::endl;
    }
}

void SuppServicesMenu::getCallWaitingPref(std::vector<std::string> userInput) {
    auto suppServicesManager = suppServicesManagers_[slot_ - 1];
    if (suppServicesManager) {
        auto ret = suppServicesManager->requestCallWaitingPref(
            GetSuppSvcResponseCallback::getCallWaitingPrefResp);
        if (ret == telux::common::Status::SUCCESS) {
            std::cout << "\nGet call waiting preference request sent successfully\n";
        } else {
            std::cout << "\nGet call waiting preference request failed \n";
        }
    } else {
            std::cout << "Invalid Manager Object" << std::endl;
    }
}

void SuppServicesMenu::setCallForwardingPref(std::vector<std::string> userInput) {
    auto suppServicesManager = suppServicesManagers_[slot_ - 1];
    ForwardReq req;
    ServiceClass serviceClass;
    int command = -1;
    std::cout << "Enter reason for call forwarding: \n\
    1 - Unconditional\n\
    2 - Busy\n\
    3 - Noreply\n\
    4 - NotReachable\n\
    23 - NotLoggedIn\n";
    std::cin >> command;
    Utils::validateInput(command);
    if (command == INPUT_UNCONDITIONAL || command == INPUT_BUSY || command == INPUT_NO_REPLY ||
        command == INPUT_NOT_REACHABLE || command == INPUT_NOT_LOGGED_IN) {
        req.reason = static_cast<ForwardReason>(command);
    } else {
        std::cout << "Invalid input" << std::endl;
        return;
    }

    std::cout << "\nEnter Call forwarding Pref (1-Activate, 2-Deactivate, 3-Register, 4-Erase) : ";
    std::cin >> command;
    Utils::validateInput(command);
    if (command == INPUT_ACTIVATE || command == INPUT_DEACTIVATE ||
        command == INPUT_REGISTER || command == INPUT_ERASE) {
        req.operation = static_cast<ForwardOperation>(command);
        if (req.operation == ForwardOperation::REGISTER) {
            std::cout << "\nEnter mobile number : " ;
            std::string userInput = "";
            std::cin >> userInput;
            req.number = userInput;
            if (req.reason == ForwardReason::NOREPLY) {
                bool invalidTimer = false;
                std::string noReplyTimer = "";
                do {
                   std::cout << "\nEnter no reply timer value(0-255) : ";
                   std::cin >> noReplyTimer;
                   if(Utils::validateDigitString(noReplyTimer)) {
                      if(noReplyTimer.size() <= MAX_INPUT_NO_REPLY) {
                         int tmp = stoi(noReplyTimer);
                         if((tmp >= MIN_NO_REPLY_TIMER) && (tmp <= MAX_NO_REPLY_TIMER)) {
                            req.noReplyTimer = tmp;
                            invalidTimer = false;
                         } else {
                            std::cout <<"No reply timer value not in range (0-255)" << std::endl;
                            invalidTimer = true;
                         }
                      } else {
                         std::cout <<"No reply timer value not in range (0-255)" << std::endl;
                         invalidTimer = true;
                      }
                    } else {
                       std::cout <<" Invalid input " << std::endl;
                       return;
                    }
                } while(invalidTimer);
            }
        }
        if (suppServicesManager) {
            serviceClass.set(static_cast<uint8_t>(ServiceClassType::VOICE));
            req.serviceClass = serviceClass;
            auto ret = suppServicesManager->setForwardingPref(req,
                SetSuppSvcResponseCallback::setSuppSvcResp);
            if (ret == telux::common::Status::SUCCESS) {
                std::cout << "\nSet forwarding preference request sent successfully\n";
            } else {
                std::cout << "\nSet forwarding preference request failed \n";
            }
        } else {
                std::cout << "Invalid Manager Object" << std::endl;
        }
    }
}

void SuppServicesMenu::getCallForwardingPref(std::vector<std::string> userInput) {
    auto suppServicesManager = suppServicesManagers_[slot_ - 1];
    ServiceClass serviceClass;
    int command = -1;
    ForwardReason reason = ForwardReason::UNCONDITIONAL;
    std::cout << "Enter reason for call forwarding: \n\
    1 - Unconditional\n\
    2 - Busy\n\
    3 - Noreply\n\
    4 - NotReachable\n\
    23 - NotLoggedIn\n";
    std::cin >> command;
    Utils::validateInput(command);
    if (command == INPUT_UNCONDITIONAL || command == INPUT_BUSY || command == INPUT_NO_REPLY ||
        command == INPUT_NOT_REACHABLE || command == INPUT_NOT_LOGGED_IN) {
        reason = static_cast<ForwardReason>(command);
    } else {
        std::cout << "Invalid input" << std::endl;
        return;
    }

    if (suppServicesManager) {
        serviceClass.set(static_cast<uint8_t>(ServiceClassType::VOICE));
        auto ret = suppServicesManager->requestForwardingPref(
            serviceClass, reason,
            GetSuppSvcResponseCallback::getForwardingPrefResp);
        if (ret == telux::common::Status::SUCCESS) {
            std::cout << "\nGet forwarding preference request sent successfully\n";
        } else {
            std::cout << "\nGet forwarding preference request failed \n";
        }
    } else {
        std::cout << "Invalid Manager Object" << std::endl;
    }
}

void SuppServicesMenu::selectSimSlot(std::vector<std::string> userInput) {
   std::string slotSelection;
   char delimiter = '\n';

   std::cout << "Enter the desired SIM slot (1-Primary, 2-Secondary): ";
   std::getline(std::cin, slotSelection, delimiter);

   if (!slotSelection.empty()) {
      try {
         int slot = std::stoi(slotSelection);
         if (slot > MAX_SLOT_ID  || slot < DEFAULT_SLOT_ID) {
            std::cout << "Invalid slot entered, using default slot" << std::endl;
            slot_ = DEFAULT_SLOT_ID;
         } else {
            slot_ = static_cast<SlotId>(slot);
            std::cout << "Successfully changed to slot " << slot << std::endl;
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

void SuppServicesMenu::setOirPref(std::vector<std::string> userInput) {
    auto suppServicesManager = suppServicesManagers_[slot_ - 1];
    int command = -1;
    ServiceClass serviceClass;

    std::cout << "Enter originating identification restriction Pref(1-Enable, 2-Disable) : ";
    std::cin >> command;
    Utils::validateInput(command);
    if (command == 1 || command == 2 ) {
        if (suppServicesManager) {
            serviceClass.set(static_cast<uint8_t>(ServiceClassType::VOICE));
            auto ret = suppServicesManager->setOirPref(serviceClass,
                static_cast<SuppServicesStatus>(command),
                SetSuppSvcResponseCallback::setSuppSvcResp);
            if (ret == telux::common::Status::SUCCESS) {
                std::cout << "\nSet OIR request sent successfully" << std::endl;
            } else {
                std::cout << "\nSet OIR request failed" << std::endl;
            }
        } else {
                std::cout << "Invalid Manager Object" << std::endl;
        }
    }
}

void SuppServicesMenu::getOirPref(std::vector<std::string> userInput) {
    auto suppServicesManager = suppServicesManagers_[slot_ - 1];
    ServiceClass serviceClass;

    if (suppServicesManager) {
        serviceClass.set(static_cast<uint8_t>(ServiceClassType::VOICE));
        auto ret = suppServicesManager->requestOirPref(serviceClass,
            GetSuppSvcResponseCallback::getOirStatusResp);
        if (ret == telux::common::Status::SUCCESS) {
            std::cout << "\nGet OIR request sent successfully\n";
        } else {
            std::cout << "\nGet OIR request failed \n";
        }
    } else {
            std::cout << "Invalid Manager Object" << std::endl;
    }
}
