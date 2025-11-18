/*
 *  Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * CardServicesMenu provides menu options to invoke Card Services such as Transmit APDU.
 */

#include <chrono>
#include <iostream>
#include "utils/Utils.hpp"
#include <telux/tel/PhoneFactory.hpp>

#include "CardServicesMenu.hpp"
#include "CardFileMenu.hpp"
#include "CardRefreshMenu.hpp"

#define SIM_CARD_POWER_UP 1
#define SIM_CARD_POWER_DOWN 0
#define INVALID -1
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void ChangeCardPinResponseCb(int retryCount, telux::common::ErrorCode error) {
   if(error != telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Change Card Pin Request failed with errorCode: " << static_cast<int>(error)
               << ":" << Utils::getErrorCodeAsString(error) << " retry count: " <<
               retryCount << std::endl;
   } else {
      PRINT_CB << "Change Card Pin Request successful retryCount:" << retryCount << std::endl;
   }
}

void unlockCardByPinResponseCb(int retryCount, telux::common::ErrorCode error) {
   if(error != telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Unlock Card By Pin Request failed with errorCode: " << static_cast<int>(error)
               << ":" << Utils::getErrorCodeAsString(error) << " retry count: " <<
               retryCount << std::endl;
   } else {
      PRINT_CB << "Unlock Card By Pin Request successful retryCount:" << retryCount << std::endl;
   }
}

void unlockCardByPukResponseCb(int retryCount, telux::common::ErrorCode error) {
   if(error != telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Unlock Card By Puk Request failed with errorCode: " << static_cast<int>(error)
               << ":" << Utils::getErrorCodeAsString(error) << " retry count: " <<
               retryCount << std::endl;
   } else {
      PRINT_CB << "Unlock Card By Puk request successful retryCount:" << retryCount << std::endl;
   }
}

void setCardLockResponseCb(int retryCount, telux::common::ErrorCode error) {
   if(error != telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Set Card Lock Request failed with errorCode: " << static_cast<int>(error) << ":"
               << Utils::getErrorCodeAsString(error) << " retry count: " <<
               retryCount << std::endl;
   } else {
      PRINT_CB << "Set Card Lock request successful retryCount:" << retryCount << std::endl;
   }
}

void queryFdnLockResponseCb(bool isAvailable, bool isEnabled, telux::common::ErrorCode error) {
   if(error != telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Query FDN lock state request failed with errorCode: " << static_cast<int>(error)
               << ":" << Utils::getErrorCodeAsString(error) << std::endl;
   } else {
      PRINT_CB << "Query FDN lock state is successful" << std::endl;
      if(isAvailable) {
         PRINT_CB << "FDN lock is available and ";
         if(isEnabled) {
            std::cout << "enabled" << std::endl;
         } else {
            std::cout << "disabled" << std::endl;
         }
      } else {
         PRINT_CB << "FDN lock not available" << std::endl;
      }
   }
}

void queryPin1LockResponseCb(bool state, telux::common::ErrorCode error) {
   if(error != telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Query Pin1 Lock Request failed with errorCode: " << static_cast<int>(error)
               << ":" << Utils::getErrorCodeAsString(error) << std::endl;
   } else {
      PRINT_CB << "Query Pin1 Lock Request successful state:" << state << std::endl;
   }
}

CardServicesMenu::CardServicesMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

CardServicesMenu::~CardServicesMenu() {
   if(cardListener_) {
      cardManager_->removeListener(cardListener_);
      cardListener_ = nullptr;
   }
   myOpenLogicalChannelCb_ = nullptr;
   myTransmitApduCb_ = nullptr;
   myCloseLogicalChannelCb_ = nullptr;
   cardManager_ = nullptr;
}

bool CardServicesMenu::init() {
   std::chrono::time_point<std::chrono::steady_clock> startTime, endTime;
   startTime = std::chrono::steady_clock::now();
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> cardMgrprom;
   cardManager_ = phoneFactory.getCardManager([&](telux::common::ServiceStatus status) {
       cardMgrprom.set_value(status);
   });

   if (!cardManager_) {
       std::cout <<  "Failed to get CardManager instance" << std::endl;
       return false;
   }
   //  Check if Card Manager subsystem is ready
   telux::common::ServiceStatus cardMgrStatus = cardManager_->getServiceStatus();

   //  If Card Manager subsystem is not ready, wait for it to be ready
   if (cardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Card Manager subsystem is not ready, Please wait" << std::endl;
   }

   cardMgrStatus = cardMgrprom.get_future().get();
   //  return the function, if SDK is unable to initialize telephony subsystems
   if (cardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      endTime = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "\nElapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                << std::endl;
      std::cout << "Card Manager subsystem is ready \n";
   } else {
      std::cout << "ERROR - Unable to initialize Card Manager subSystem" << std::endl;
      return false;
   }

   std::vector<int> slotIds;
   telux::common::Status status = cardManager_->getSlotIds(slotIds);
   if (status == telux::common::Status::SUCCESS) {
       for (auto index = 1; index <= slotIds.size(); index++) {
           auto card = cardManager_->getCard(index, &status);
           if (card != nullptr) {
              cards_.emplace_back(card);
           }
       }
   }

   // listener
   cardListener_ = std::make_shared<MyCardListener>();

   // callbacks
   myOpenLogicalChannelCb_ = std::make_shared<MyOpenLogicalChannelCallback>();
   myTransmitApduCb_ = std::make_shared<MyTransmitApduResponseCallback>();
   myCloseLogicalChannelCb_ = std::make_shared<MyCardCommandResponseCallback>();

   // registering Listener
   status = cardManager_->registerListener(cardListener_);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Unable to registerListener" << std::endl;
   }
   std::shared_ptr<ConsoleAppCommand> getCardStateCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "Get_card_state", {},
                        std::bind(&CardServicesMenu::getCardState, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getSupportedAppsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Get_supported_apps", {},
         std::bind(&CardServicesMenu::getSupportedApps, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> openLogicalChannelCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Open_logical_channel", {"aid"},
         std::bind(&CardServicesMenu::openLogicalChannel, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> closeLogicalChannelCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Close_logical_channel", {"channel"},
         std::bind(&CardServicesMenu::closeLogicalChannel, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> transmitApduCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "Transmit_APDU", {},
                        std::bind(&CardServicesMenu::transmitApdu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> basicTransmitApduCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "Basic_transmit_APDU", {},
         std::bind(&CardServicesMenu::basicTransmitApdu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> changeCardPinCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("7", "Change_card_pin", {},
                        std::bind(&CardServicesMenu::changeCardPin, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> unlockCardByPinCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("8", "Unlock_card_by_pin", {}, std::bind(&CardServicesMenu::unlockCardByPin,
                                                                 this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> unlockCardByPukCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("9", "Unlock_card_by_puk", {}, std::bind(&CardServicesMenu::unlockCardByPuk,
                                                                 this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> queryPin1LockStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "10", "Query_pin1_lockState", {},
         std::bind(&CardServicesMenu::queryPin1LockState, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> queryFdnLockStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "11", "Query_fdn_lockState", {},
         std::bind(&CardServicesMenu::queryFdnLockState, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setCardLockCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("12", "Set_card_lock", {},
                        std::bind(&CardServicesMenu::setCardLock, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> cardPowerCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "13", "Set_card_power", {},
            std::bind(&CardServicesMenu::cardPower, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> cardFileHandlerMenuCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "14", "Card_File_Handler", {},
            std::bind(&CardServicesMenu::cardFileMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> cardRefreshMenuCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "15", "Card_Refresh_Menu", {},
            std::bind(&CardServicesMenu::cardRefreshMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> checkNtnProfileActiveCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "16", "Check_NTN_Profile_Active", {},
            std::bind(&CardServicesMenu::checkNtnProfileActive, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> selectCardSlotCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("17", "Select_card_slot", {},
                        std::bind(&CardServicesMenu::selectCardSlot, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListCardServicesSubMenu
      = {getCardStateCommand,        getSupportedAppsCommand,  openLogicalChannelCommand,
         closeLogicalChannelCommand, transmitApduCommand,      basicTransmitApduCommand,
         changeCardPinCommand,       unlockCardByPinCommand,   unlockCardByPukCommand,
         queryPin1LockStateCommand,  queryFdnLockStateCommand, setCardLockCommand,
         cardPowerCommand, cardFileHandlerMenuCommand, cardRefreshMenuCommand,
         checkNtnProfileActiveCommand};

   if (cards_.size() > 1) {
       commandsListCardServicesSubMenu.emplace_back(selectCardSlotCommand);
   }

   addCommands(commandsListCardServicesSubMenu);
   ConsoleApp::displayMenu();
   return true;
}

std::string CardServicesMenu::cardStateToString(telux::tel::CardState state) {
   std::string cardState;
   switch(state) {
      case telux::tel::CardState::CARDSTATE_ABSENT:
         cardState = "Absent";
         break;
      case telux::tel::CardState::CARDSTATE_PRESENT:
         cardState = "Present";
         break;
      case telux::tel::CardState::CARDSTATE_ERROR:
         cardState = "Either error or absent";
         break;
      case telux::tel::CardState::CARDSTATE_RESTRICTED:
         cardState = "Restricted";
         break;
      default:
         cardState = "Unknown card state";
         break;
   }
   return cardState;
}

void CardServicesMenu::getCardState(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   if(card) {
      telux::tel::CardState cardState;
      card->getState(cardState);
      std::cout << "CardState : " << cardStateToString(cardState) << std::endl;
   }
}

std::string CardServicesMenu::appTypeToString(telux::tel::AppType appType) {
   std::string applicationType;
   switch(appType) {
      case telux::tel::AppType::APPTYPE_SIM:
         applicationType = "SIM";
         break;
      case telux::tel::AppType::APPTYPE_USIM:
         applicationType = "USIM";
         break;
      case telux::tel::AppType::APPTYPE_RUIM:
         applicationType = "RUIM";
         break;
      case telux::tel::AppType::APPTYPE_CSIM:
         applicationType = "CSIM";
         break;
      case telux::tel::AppType::APPTYPE_ISIM:
         applicationType = "ISIM";
         break;
      default:
         applicationType = "Unknown";
         break;
   }
   return applicationType;
}

std::string CardServicesMenu::appStateToString(telux::tel::AppState appState) {
   std::string applicationState;
   switch(appState) {
      case telux::tel::AppState::APPSTATE_DETECTED:
         applicationState = "Detected";
         break;
      case telux::tel::AppState::APPSTATE_PIN:
         applicationState = "PIN";
         break;
      case telux::tel::AppState::APPSTATE_PUK:
         applicationState = "PUK";
         break;
      case telux::tel::AppState::APPSTATE_SUBSCRIPTION_PERSO:
         applicationState = "Subscription Perso";
         break;
      case telux::tel::AppState::APPSTATE_READY:
         applicationState = "Ready";
         break;
      case telux::tel::AppState::APPSTATE_ILLEGAL:
         applicationState = "Illegal";
         break;
      default:
         applicationState = "Unknown";
         break;
   }
   return applicationState;
}

void CardServicesMenu::getSupportedApps(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   if(card) {
      std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
      applications = card->getApplications();
      if(applications.size() != 0)  {
         for(auto cardApp : applications) {
            std::cout << "App type: " << appTypeToString(cardApp->getAppType()) << std::endl;
            std::cout << "App state: " << appStateToString(cardApp->getAppState()) << std::endl;
            std::cout << "AppId : " << cardApp->getAppId() << std::endl;
         }
      } else {
         std::cout <<"No supported applications"<< std::endl;
         telux::tel::CardState cardState;
         card->getState(cardState);
         std::cout << "Card State : " << cardStateToString(cardState) << std::endl;
      }
   }  else {
      std::cout << "ERROR: Unable to get card instance";
   }
}

void CardServicesMenu::openLogicalChannel(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   if(card) {
      std::string aid = userInput[1];
      std::cout << "Open logical channel with aid:" << aid << std::endl;
      auto ret = card->openLogicalChannel(aid, myOpenLogicalChannelCb_);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "Open logical channel request sent successfully \n";
      } else {
         std::cout << "Open logical channel request failed \n";
      }
   }  else {
      std::cout << "ERROR: Unable to get card instance";
   }
}

void CardServicesMenu::transmitApdu(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   if(card) {
      int channel = INVALID;
      int cla, instruction, p1, p2, p3;
      std::vector<uint8_t> data;

      cla = 0;
      instruction = 0;
      p1 = 0;
      p2 = 0;
      p3 = 0;

      std::cout << std::endl;
      std::cout << "Enter the channel : ";
      std::cin >> channel;
      Utils::validateInput(channel);
      std::cout << "Enter CLA : ";
      std::cin >> cla;
      Utils::validateInput(cla);
      std::cout << "Enter INS : ";
      std::cin >> instruction;
      Utils::validateInput(instruction);
      std::cout << "Enter P1 : ";
      std::cin >> p1;
      Utils::validateInput(p1);
      std::cout << "Enter P2 : ";
      std::cin >> p2;
      Utils::validateInput(p2);
      std::cout << "Enter P3 : ";
      std::cin >> p3;
      Utils::validateInput(p3);
      int dataInput;
      for(int i = 0; i < p3; i++) {
         std::cout << "Enter DATA (" << i + 1 << ") :";
         std::cin >> dataInput;
         Utils::validateInput(dataInput);
         data.emplace_back((uint8_t)dataInput);
      }

      auto ret = card->transmitApduLogicalChannel(channel, (uint8_t)cla, (uint8_t)instruction,
                                                   (uint8_t)p1, (uint8_t)p2, (uint8_t)p3, data,
                                                   myTransmitApduCb_);
      std::cout << (ret == telux::common::Status::SUCCESS ?"Transmit APDU request sent successfully"
                                                          : "Transmit APDU request failed")
                << '\n';
   }  else {
      std::cout << "ERROR: Unable to get card instance";
   }
}

void CardServicesMenu::basicTransmitApdu(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   if(card) {
      int cla, instruction, p1, p2, p3;
      std::vector<uint8_t> data;

      cla = 0;
      instruction = 0;
      p1 = 0;
      p2 = 0;
      p3 = 0;

      std::string user_input;
      std::cout << std::endl;
      std::cout << "Enter CLA : ";
      std::cin >> cla;
      Utils::validateInput(cla);
      std::cout << "Enter INS : ";
      std::cin >> instruction;
      Utils::validateInput(instruction);
      std::cout << "Enter P1 : ";
      std::cin >> p1;
      Utils::validateInput(p1);
      std::cout << "Enter P2 : ";
      std::cin >> p2;
      Utils::validateInput(p2);
      std::cout << "Enter P3 : ";
      std::cin >> p3;
      Utils::validateInput(p3);
      int tmpInp;
      for(int i = 0; i < p3; i++) {
         std::cout << "Enter DATA (" << i + 1 << ") :";
         std::cin >> tmpInp;
         Utils::validateInput(tmpInp);
         data.emplace_back((uint8_t)tmpInp);
      }
      auto ret = card->transmitApduBasicChannel((uint8_t)cla, (uint8_t)instruction, (uint8_t)p1,
                                                 (uint8_t)p2, (uint8_t)p3, data, myTransmitApduCb_);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "Basic transmit APDU request sent successfully\n";
      } else {
         std::cout << "Basic transmit APDU request failed\n";
      }
   }  else {
      std::cout << "ERROR: Unable to get card instance";
   }
}

void CardServicesMenu::closeLogicalChannel(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   if(card) {
      int channel = std::stoi(userInput[1]);
      std::cout << "Close logical channel with channel:" << channel << std::endl;
      auto ret = card->closeLogicalChannel(channel, myCloseLogicalChannelCb_);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "Close logical channel request sent successfully \n";
      } else {
         std::cout << "Close logical channel request failed \n";
      }
   }  else {
      std::cout << "ERROR: Unable to get card instance";
   }
}

void CardServicesMenu::changeCardPin(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];

   std::string oldPin, newPin, lockType;
   telux::tel::CardLockType cardLockType;
   char delimiter = '\n';

   if(!card) {
      std::cout << "ERROR: Unable to get card instance";
      return;
   }
   std::cout << "Enter 1-PIN1/2-PIN2 : ";
   std::getline(std::cin, lockType, delimiter);
   // Validating user input
   if(!lockType.empty()) {
      try {
         std::stoi(lockType);
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << lockType
                   << std::endl;
      }
   } else {
      std::cout << "empty input going with default PIN1\n";
      cardLockType = telux::tel::CardLockType::PIN1;
   }

   if(lockType.compare("1") == 0) {
      cardLockType = telux::tel::CardLockType::PIN1;
   } else if(lockType.compare("2") == 0) {
      cardLockType = telux::tel::CardLockType::PIN2;
   } else {
      std::cout << "wrong input, going with default PIN1\n";
      cardLockType = telux::tel::CardLockType::PIN1;
   }
   std::cout << "Enter old PIN: ";
   std::getline(std::cin, oldPin, delimiter);

   std::cout << "Enter new PIN: ";
   std::getline(std::cin, newPin, delimiter);

   std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
   applications = card->getApplications();
   if(applications.size() != 0)  {
      for(auto cardApp : applications) {
         if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
            auto ret
               = cardApp->changeCardPassword(cardLockType, oldPin, newPin,
                  &ChangeCardPinResponseCb);
            if(ret == telux::common::Status::SUCCESS) {
               std::cout << "Change card PIN request sent successfully\n";
            } else {
               std::cout << "Change card PIN request failed\n";
            }
         }
      }
   } else {
         std::cout <<"Change card PIN request failed"<< std::endl;
         telux::tel::CardState cardState;
         card->getState(cardState);
         std::cout << "Card State : " << cardStateToString(cardState) << std::endl;
   }
}

void CardServicesMenu::unlockCardByPuk(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   std::string puk, newPin, lockType;
   telux::tel::CardLockType cardLockType;
   char delimiter = '\n';

   if(!card) {
      std::cout << "ERROR: Unable to get card instance";
      return;
   }
   std::cout << "Enter 1-PUK1/2-PUK2 : ";
   std::getline(std::cin, lockType, delimiter);
   if(!lockType.empty()) {
      try {
         std::stoi(lockType);
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << lockType
                   << std::endl;
      }
   } else {
      std::cout << "empty input going with default PUK1\n";
      cardLockType = telux::tel::CardLockType::PUK1;
   }

   if(lockType.compare("1") == 0) {
      cardLockType = telux::tel::CardLockType::PUK1;
   } else if(lockType.compare("2") == 0) {
      cardLockType = telux::tel::CardLockType::PUK2;
   } else {
      std::cout << "As the user entered wrong input, setting default lock type value 'PUK1'\n";
      cardLockType = telux::tel::CardLockType::PUK1;
   }
   std::cout << "Enter PUK: ";
   std::getline(std::cin, puk, delimiter);

   std::cout << "Enter New PIN: ";
   std::getline(std::cin, newPin, delimiter);

   std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
   applications = card->getApplications();
   if(applications.size() != 0)  {
      for(auto cardApp : applications) {
         if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
            auto ret = cardApp->unlockCardByPuk(cardLockType, puk, newPin,
               &unlockCardByPukResponseCb);
            if(ret == telux::common::Status::SUCCESS) {
               std::cout << "Unlock card by PUK request sent successfully\n";
            } else {
               std::cout << "Unlock card by PUK request failed\n";
            }
         }
      }
   } else {
         std::cout <<"Unlock card by PUK request failed"<< std::endl;
         telux::tel::CardState cardState;
         card->getState(cardState);
         std::cout << "Card State : " << cardStateToString(cardState) << std::endl;
   }
}

void CardServicesMenu::unlockCardByPin(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   std::string newPin, lockType;
   telux::tel::CardLockType cardLockType;
   char delimiter = '\n';

   if(!card) {
      std::cout << "ERROR: Unable to get card instance";
      return;
   }
   std::cout << "Enter the 1-PIN1/2-PIN2: ";
   std::getline(std::cin, lockType, delimiter);
   if(!lockType.empty()) {
      try {
         std::stoi(lockType);
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << lockType
                   << std::endl;
      }
   } else {
      std::cout << "empty input going with default PIN1\n";
      cardLockType = telux::tel::CardLockType::PIN1;
   }

   if(lockType.compare("1") == 0) {
      cardLockType = telux::tel::CardLockType::PIN1;
   } else if(lockType.compare("2") == 0) {
      cardLockType = telux::tel::CardLockType::PIN2;
   } else {
      std::cout << "As the user entered wrong option, setting default lock type value 'PIN1'\n";
      cardLockType = telux::tel::CardLockType::PIN1;
   }

   std::cout << "Enter PIN: ";
   std::getline(std::cin, newPin, delimiter);

   std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
   applications = card->getApplications();
   if(applications.size() != 0)  {
      for(auto cardApp : applications) {
         if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
            auto ret = cardApp->unlockCardByPin(cardLockType, newPin, &unlockCardByPinResponseCb);
            if(ret == telux::common::Status::SUCCESS) {
               std::cout << "Unlock card by pin request sent successfully\n";
            } else {
               std::cout << "Unlock card by pin request failed\n";
            }
         }
      }
   } else {
         std::cout <<"Unlock card by PIN request failed\n"<< std::endl;
         telux::tel::CardState cardState;
         card->getState(cardState);
         std::cout << "Card State : " << cardStateToString(cardState) << std::endl;
   }
}

void CardServicesMenu::queryPin1LockState(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];

   if(card) {
      std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
      applications = card->getApplications();
      if(applications.size() != 0)  {
         for(auto cardApp : applications) {
            if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
               auto ret = cardApp->queryPin1LockState(queryPin1LockResponseCb);
               if(ret == telux::common::Status::SUCCESS) {
                  std::cout << "Query pin1 lock state request sent successfully\n";
               } else {
                  std::cout << "Query pin1 lock state request failed\n";
               }
            }
         }
      } else {
         std::cout <<"Query pin1 lock state request failed"<< std::endl;
         telux::tel::CardState cardState;
         card->getState(cardState);
         std::cout << "Card State : " << cardStateToString(cardState) << std::endl;
      }
   } else {
      std::cout << "ERROR: Unable to get card instance";
   }
}

void CardServicesMenu::queryFdnLockState(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];

   if(card) {
      std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
      applications = card->getApplications();
      if(applications.size() != 0)  {
         for(auto cardApp : applications) {
            if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
               auto ret = cardApp->queryFdnLockState(&queryFdnLockResponseCb);
               if(ret == telux::common::Status::SUCCESS) {
                  std::cout << "Query FDN lock state request sent successfully\n";
               } else {
                  std::cout << "Query FDN lock state request failed\n";
               }
            }
         }
      } else {
         std::cout <<"Query FDN lock state request failed"<< std::endl;
         telux::tel::CardState cardState;
         card->getState(cardState);
         std::cout << "Card State : " << cardStateToString(cardState) << std::endl;
      }
   } else {
      std::cout << "ERROR: Unable to get card instance";
   }
}

void CardServicesMenu::setCardLock(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   std::string pwd, isEnable, lockType;
   telux::tel::CardLockType cardLockType;
   char delimiter = '\n';

   if(!card) {
      std::cout << "ERROR: Unable to get card instance";
      return;
   }
   std::cout << "Enter the 1-PIN1/2-FDN : ";
   std::getline(std::cin, lockType, delimiter);
   if(!lockType.empty()) {
      try {
         std::stoi(lockType);
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << lockType
                   << std::endl;
      }
   } else {
      std::cout << "empty input going with default PIN1\n";
      cardLockType = telux::tel::CardLockType::PIN1;
   }

   if(lockType.compare("1") == 0) {
      cardLockType = telux::tel::CardLockType::PIN1;
   } else if(lockType.compare("2") == 0) {
      cardLockType = telux::tel::CardLockType::FDN;
   } else {
      std::cout << "As the user entered wrong option, going with default card lock as PIN1\n";
      cardLockType = telux::tel::CardLockType::PIN1;
   }

   std::cout << "Enter PIN: ";
   std::getline(std::cin, pwd, delimiter);

   std::cout << "Enter 1-Enable/0-Disable: ";
   std::getline(std::cin, isEnable, delimiter);
   bool opt = false;
   if(!isEnable.empty()) {
      try {
         int value = std::stoi(isEnable);
         if(value != 0 && value != 1) {
            std::cout << "As user entered wrong input, taking default input as 'enable'\n";
            opt = true;
         } else {
            opt = value;
         }
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
      }
   } else {
      std::cout << "As user entered empty input, taking default input as 'enable'\n";
      opt = true;
   }

   std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
   applications = card->getApplications();
   if(applications.size() != 0)  {
      for(auto cardApp : applications) {
         if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
            auto ret = cardApp->setCardLock(cardLockType, pwd, opt, &setCardLockResponseCb);
            if(ret == telux::common::Status::SUCCESS) {
               std::cout << "Set card lock request sent successfully\n";
            } else {
               std::cout << "Set card lock request failed\n";
            }
         }
      }
   } else {
         std::cout <<"Set card lock request failed"<< std::endl;
         telux::tel::CardState cardState;
         card->getState(cardState);
         std::cout << "Card State : " << cardStateToString(cardState) << std::endl;
   }
}

void CardServicesMenu::cardPower(std::vector<std::string> userInput) {

   int cardPowerSelection = -1;
   if (cardManager_) {
      std::cout  << "Enter Card Power(1 - On, 0 - Off): ";
      std::cin >> cardPowerSelection;
      Utils::validateInput(cardPowerSelection);
      auto status = telux::common::Status::FAILED;
      if (cardPowerSelection == SIM_CARD_POWER_UP) {
         std::cout << "Card power up \n";
         status = cardManager_->cardPowerUp(static_cast<SlotId>(slot_),
              MyCardPowerResponseCallback::cardPowerUpResp);
      } else if (cardPowerSelection == SIM_CARD_POWER_DOWN) {
         std::cout << "Card power down \n";
         status = cardManager_->cardPowerDown(static_cast<SlotId>(slot_),
              MyCardPowerResponseCallback::cardPowerDownResp);
      } else {
         std::cout << "Invalid input \n";
      }
      if (status == telux::common::Status::SUCCESS) {
          std::cout << "Request sent successfully \n";
      } else {
          std::cout << "ERROR - Failed to send the request, Status:"
                    << static_cast<int>(status) << "\n";
      }
      Utils::printStatus(status);
   } else {
      std::cout << "ERROR - CardManager is null \n";
   }
}

void CardServicesMenu::checkNtnProfileActive(std::vector<std::string> userInput) {
   auto card = cards_[slot_ - 1];
   std::cout << "SlotId: " << slot_ << std::endl;
   if (card) {
       bool ntnProfileActive = false;
       ntnProfileActive = card->isNtnProfileActive();
       if (ntnProfileActive) {
           std::cout << "NTN profile is active" << std::endl;
       } else {
          std::cout << "No active NTN profile" << std::endl;
       }
   }
}

void CardServicesMenu::selectCardSlot(std::vector<std::string> userInput)
{
   std::string slotSelection;
   char delimiter = '\n';

   std::cout << "Enter the desired card slot (1-Primary, 2-Secondary): ";
   std::getline(std::cin, slotSelection, delimiter);

   if (!slotSelection.empty()) {
      try {
         int slot = std::stoi(slotSelection);
         if (slot > MAX_SLOT_ID  || slot < DEFAULT_SLOT_ID) {
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

void CardServicesMenu::cardFileMenu(std::vector<std::string> userInput) {
   CardFileMenu cardFileMenu("Card File Menu", "CardFile> ");
   if (cardFileMenu.init()) {
       cardFileMenu.mainLoop();
   }
   ConsoleApp::displayMenu();
}

void CardServicesMenu::cardRefreshMenu(std::vector<std::string> userInput) {
   CardRefreshMenu cardRefreshMenu("Card Refresh Menu", "CardRefresh> ");
   if (cardRefreshMenu.init()) {
       cardRefreshMenu.mainLoop();
   }
   ConsoleApp::displayMenu();
}
