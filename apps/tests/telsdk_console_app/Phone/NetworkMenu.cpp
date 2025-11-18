/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <regex>
#include <bitset>

#include <telux/tel/PhoneFactory.hpp>

#include "MyNetworkSelectionHandler.hpp"
#include "NetworkMenu.hpp"
#include "Utils.hpp"

#define UNKNOWN 0
#define INVALID -1

NetworkMenu::NetworkMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

NetworkMenu::~NetworkMenu() {
   for (auto index = 0; index < networkManagers_.size(); index++) {
       networkManagers_[index]->deregisterListener(networkListener_);
       networkManagers_[index] = nullptr;
   }
   if (networkListener_) {
      networkListener_ = nullptr;
   }
}

bool NetworkMenu::init() {

   //  Get the PhoneFactory and NetworkManger instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   networkListener_ = std::make_shared<MyNetworkSelectionListener>();
   std::vector<int> phoneIds;
   std::promise<telux::common::ServiceStatus> prom;
   auto phoneManager = phoneFactory.getPhoneManager([&](telux::common::ServiceStatus status) {
      prom.set_value(status);
   });
   if (!phoneManager) {
      std::cout << "ERROR - Failed to get Phone Manager \n";
      return false;
   }
   telux::common::ServiceStatus phoneMgrStatus = phoneManager->getServiceStatus();
   if (phoneMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Phone Manager subsystem is not ready, Please wait \n";
   }
   phoneMgrStatus = prom.get_future().get();
   if ( phoneMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE ) {
      std::cout << "Phone Manager subsystem is ready \n";
      telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
      if (status == telux::common::Status::SUCCESS) {
         for (auto index = 1; index <= phoneIds.size(); index++) {
            std::promise<telux::common::ServiceStatus> prom;
            auto networkManager = phoneFactory.getNetworkSelectionManager( index,
                  [&](telux::common::ServiceStatus status) {
               prom.set_value(status);
            });
            if (!networkManager) {
               std::cout << "ERROR - Failed to get Network Selection Manager instance \n";
               return false;
            }
            std::cout << "Waiting for Network Selection Manager to be ready on slotId " << index
                      << "\n";
            telux::common::ServiceStatus networkSelMgrStatus = prom.get_future().get();
            if (networkSelMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
               std::cout << "Network Selection Manager is ready on slotId " << index << "\n";
               networkManagers_.emplace_back(networkManager);
            } else {
               std::cout << "ERROR - Unable to initialize,"
                         << " network selection manager subsystem on slotId "
                         << index << std::endl;
               return false;
            }
         }
      }
      for (auto index = 0; index < networkManagers_.size(); index++) {
         auto status = networkManagers_[index]->registerListener(networkListener_);
         if (status != telux::common::Status::SUCCESS) {
            std::cout << "Failed to registerListener for network Manager" << std::endl;
            return false;
         }
      }

      std::shared_ptr<ConsoleAppCommand> getNetworkSelectionModeCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "get_selection_mode", {},
      std::bind(&NetworkMenu::getNetworkSelectionMode, this, std::placeholders::_1)));
      std::shared_ptr<ConsoleAppCommand> setNetworkSelectionModeCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "set_selection_mode", {},
      std::bind(&NetworkMenu::setNetworkSelectionMode, this, std::placeholders::_1)));
      std::shared_ptr<ConsoleAppCommand> getPreferredNetworksCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "get_preferred_networks", {},
      std::bind(&NetworkMenu::getPreferredNetworks, this, std::placeholders::_1)));
      std::shared_ptr<ConsoleAppCommand> setPreferredNetworksCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "set_preferred_networks", {},
      std::bind(&NetworkMenu::setPreferredNetworks, this, std::placeholders::_1)));
      std::shared_ptr<ConsoleAppCommand> performNetworkScanCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "5", "perform_network_scan", {},
      std::bind(&NetworkMenu::performNetworkScan, this, std::placeholders::_1)));
      std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "select_sim_slot", {},
      std::bind(&NetworkMenu::selectSimSlot, this, std::placeholders::_1)));

      std::shared_ptr<ConsoleAppCommand> setLteDubiousCellCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "7", "set_lte_dubious_cell", {},
      std::bind(&NetworkMenu::setLteDubiousCell, this, std::placeholders::_1)));

      std::shared_ptr<ConsoleAppCommand> setNrDubiousCellCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "8", "set_nr_dubious_cell", {},
      std::bind(&NetworkMenu::setNrDubiousCell, this, std::placeholders::_1)));

      std::shared_ptr<ConsoleAppCommand> removeAllLteDubiousCellCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "9", "remove_all_lte_dubious_cell", {},
      std::bind(&NetworkMenu::removeAllLteDubiousCell, this, std::placeholders::_1)));

      std::shared_ptr<ConsoleAppCommand> removeAllNrDubiousCellCommand
         = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "10", "remove_all_nr_dubious_cell", {},
      std::bind(&NetworkMenu::removeAllNrDubiousCell, this, std::placeholders::_1)));

      std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListNetworkSubMenu
         = {getNetworkSelectionModeCommand, setNetworkSelectionModeCommand,
      getPreferredNetworksCommand, setPreferredNetworksCommand, performNetworkScanCommand};

      if (networkManagers_.size() > 1) {
         commandsListNetworkSubMenu.emplace_back(selectSimSlotCommand);
      }

      commandsListNetworkSubMenu.emplace_back(setLteDubiousCellCommand);
      commandsListNetworkSubMenu.emplace_back(setNrDubiousCellCommand);
      commandsListNetworkSubMenu.emplace_back(removeAllLteDubiousCellCommand);
      commandsListNetworkSubMenu.emplace_back(removeAllNrDubiousCellCommand);

      addCommands(commandsListNetworkSubMenu);
      ConsoleApp::displayMenu();
   } else {
      std::cout << "Phone Manager is NULL, failed to initialize NetworkMenu" << std::endl;
      return false;
   }
   return true;
}

void NetworkMenu::getNetworkSelectionMode(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      auto ret = networkManager->requestNetworkSelectionMode(
         MySelectionModeResponseCallback::selectionModeResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet network selection mode request sent successfully\n";
      } else {
         std::cout << "\nGet network selection mode request failed \n";
      }
   }
}

void NetworkMenu::setNetworkSelectionMode(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      bool selectionMode = false;
      std::string mcc = "";
      std::string mnc = "";
      telux::common::Status retStatus = telux::common::Status::FAILED;
      std::cout << "Enter Network Selection Mode(0-AUTOMATIC,1-MANUAL): ";
      std::cin >> selectionMode;
      Utils::validateInput(selectionMode);
      if(selectionMode == 1) {
         telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::MANUAL;
         std::cout << "Enter MCC: ";
         std::cin >> mcc;
         Utils::validateInput(mcc);
         std::cout << "Enter MNC: ";
         std::cin >> mnc;
         Utils::validateInput(mnc);
         retStatus = networkManager->setNetworkSelectionMode(
            selectMode, mcc, mnc, &MyNetworkResponsecallback::setNetworkSelectionModeResponseCb);

      } else if(selectionMode == 0) {
         telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::AUTOMATIC;
         mcc = "0";
         mnc = "0";
         retStatus = networkManager->setNetworkSelectionMode(
            selectMode, mcc, mnc, &MyNetworkResponsecallback::setNetworkSelectionModeResponseCb);

      } else {
         std::cout << "Invalid network selection mode input, Valid values are 0 or 1";
      }
      if(retStatus == telux::common::Status::SUCCESS) {
         std::cout << "\nSet network selection mode request sent successfully\n";
      } else {
         std::cout << "\nSet network selection mode request failed \n";
      }
   }
}

void NetworkMenu::getPreferredNetworks(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      auto ret = networkManager->requestPreferredNetworks(
         MyPreferredNetworksResponseCallback::preferredNetworksResponse);
      if(ret != telux::common::Status::SUCCESS) {
         std::cout << "\nGet preferred networks request failed \n";
      }
   }
}

int NetworkMenu::convertToRatType(int input) {
   switch(input) {
      case 1:
         return telux::tel::RatType::GSM;
      case 2:
         return telux::tel::RatType::LTE;
      case 3:
         return telux::tel::RatType::UMTS;
      case 4:
         return telux::tel::RatType::NR5G;
      default:
         return UNKNOWN;
   }
}

telux::tel::PreferredNetworkInfo NetworkMenu::getNetworkInfoFromUser() {
   telux::tel::PreferredNetworkInfo networkInfo = {};
   telux::tel::RatMask rat;
   uint16_t mcc;
   uint16_t mnc;
   std::string preference;
   std::vector<int> options;
   std::cout << "Enter MCC: ";
   std::cin >> mcc;
   Utils::validateInput(mcc);
   networkInfo.mcc = mcc;
   std::cout << "Enter MNC: ";
   std::cin >> mnc;
   Utils::validateInput(mnc);
   networkInfo.mnc = mnc;
   std::cout << "Select RAT types (1-GSM, 2-LTE, 3-UMTS, 4-NR5G) \n";
   std::cout << "Enter RAT types\n(For example: enter 1,2 to set GSM & "
                "LTE RAT type): ";
   std::cin >> preference;
   Utils::validateNumericString(preference);
   std::stringstream ss(preference);
   int pref = INVALID;
   while(ss >> pref) {
      options.push_back(pref);
      if(ss.peek() == ',' || ss.peek() == ' ')
         ss.ignore();
   }
   for(auto &opt : options) {
      if((opt == 1) || (opt == 2) || (opt == 3) || (opt == 4)) {
         rat.set(convertToRatType(opt));
      } else {
         std::cout << "Preference should not be out of range" << std::endl;
      }
   }
   options.clear();
   networkInfo.ratMask = rat;
   return networkInfo;
}

void NetworkMenu::setPreferredNetworks(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo;
      int numOfNetworks = UNKNOWN;
      bool clearPrevPreferredNetworks = false;
      std::cout << "Enter number of preferred networks: ";
      std::cin >> numOfNetworks;
      Utils::validateInput(numOfNetworks);

      for(int index = 0; index < numOfNetworks; index++) {
         auto nwInfo = getNetworkInfoFromUser();
         preferredNetworksInfo.emplace_back(nwInfo);
      }

      std::cout << "Clear previous preferred network(1 - Yes, 0 - No)?: ";
      std::cin >> clearPrevPreferredNetworks;
      Utils::validateInput(clearPrevPreferredNetworks);
      auto ret = networkManager->setPreferredNetworks(
         preferredNetworksInfo, clearPrevPreferredNetworks,
         MyNetworkResponsecallback::setPreferredNetworksResponseCb);

      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nSet preferred networks request sent successfully\n";
      } else {
         std::cout << "\nSet preferred networks request failed \n";
      }
   }
}

void NetworkMenu::performNetworkScan(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if (networkManager) {
      char delimiter = '\n';
      std::string ratPref = "";
      std::string networkScanTypeSelection = "";
      int networkScanType = UNKNOWN;
      telux::tel::NetworkScanInfo info {} ;
      telux::tel::RatMask rat(0);

      std::cout << "Enter the network scan type \n"
                << "(1 - RAT_Preference, 2 - Specify_RAT(s), 3 - All_RATs): ";
      std::getline(std::cin, networkScanTypeSelection, delimiter);
      if (networkScanTypeSelection.empty()) {
            std::cout << "ERROR - Network Scan type is empty \n";
            return;
      }
      try {
         networkScanType = std::stoi(networkScanTypeSelection);
         if ( networkScanType <= 0 || networkScanType > 3) {
             std::cout << "ERROR - Invalid network scan type\n";
             return;
         }

         info.scanType = static_cast<telux::tel::NetworkScanType>(networkScanType);
         if (info.scanType == telux::tel::NetworkScanType::USER_SPECIFIED_RAT) {
            std::cout << "\nSelect RAT types (1-GSM, 2-LTE, 3-UMTS, 4-NR5G) \n";
            std::cout << "(For example: enter 1,2 to scan GSM, LTE RATs): ";
            std::cin >> ratPref;
            //Regular expression to check if the input is in RAT type range ie.1-4 and
            //comma or space seperated values.
            //For example, returns true in case of 1,2,3 and 2
            //returns false in case of 1:2 and a,b.
            std::regex rgx("([1-4][, ])*[1-4]$");
            if (std::regex_match(ratPref.begin(),ratPref.end(), rgx)) {
                //Regular expresssion to find only the digit for rat type
                std::regex subMatchRgx ("([1-4])");
                std::smatch ratOption {};
                //Searches in input string for a digit in range 1-4 and stores in ratOption variable
                while (std::regex_search (ratPref, ratOption, subMatchRgx)) {
                    rat.set(convertToRatType(std::stoi(ratOption[0])));
                    ratPref = ratOption.suffix().str();
                }
                info.ratMask = rat;
            } else {
                std::cout << "ERROR::Invalid input \n";
                return;
            }
         }
      } catch (const std::exception &e) {
         std::cout << "ERROR::Invalid input, please enter a numerical value \n";
         return;
      }
      auto ret = networkManager->performNetworkScan(info,
         MyPerformNetworkScanCallback::performNetworkScanResponseCb);
      if (ret == telux::common::Status::SUCCESS) {
         std::cout << "\nPerform network scan request sent successfully\n";
      } else {
         std::cout << "\nPerform network scan request failed \n";
      }
   } else {
      std::cout << " ERROR - Network manager is NULL\n";
   }
}

void NetworkMenu::selectSimSlot(std::vector<std::string> userInput) {
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
            slot_ = slot;
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

void NetworkMenu::setLteDubiousCell(std::vector<std::string> userInput) {

    std::vector<telux::tel::LteDubiousCell> lteDbCellList;
    bool addConfig = false, isInvalid = false;
    std::string mcc, mnc;
    unsigned int arfcn = 0, pci = 0, maskInt = 0, cgi = 0;
    int activeBandInt = 0;
    telux::tel::RFBand activeBand;
    telux::tel::DbCellCauseCodeMask mask;

    do {
        telux::tel::LteDubiousCell lteDbCell;

        std::cout << "Enter MCC: " << std::endl;
        std::cin >> mcc;
        std::cout << std::endl;
        Utils::validateInput(mcc);
        lteDbCell.ci.mcc = mcc;

        std::cout << "Enter MNC: " << std::endl;
        std::cin >> mnc;
        std::cout << std::endl;
        Utils::validateInput(mnc);
        lteDbCell.ci.mnc = mnc;

        std::cout << "Enter arfcn: " << std::endl;
        std::cin >> arfcn;
        std::cout << std::endl;
        Utils::validateInput(arfcn);
        lteDbCell.ci.arfcn = arfcn;

        std::cout << "Enter pci: " << std::endl;
        std::cin >> pci;
        std::cout << std::endl;
        Utils::validateInput(pci);
        lteDbCell.ci.pci = pci;

        isInvalid = false;

        do {
            std::cout << "Enter band: (Valid int range corresponds to RFBand 0...19, 40...48, "
                << "80...88, 90, 91, 120...179, 200...205, 250...301)" << std::endl;
            std::cin >> activeBandInt;
            std::cout << std::endl;
            Utils::validateInput(activeBandInt);

            isInvalid = false;

            if ( ((activeBandInt >= 20)     && (activeBandInt<=39))  ||
                    ((activeBandInt >= 49)  && (activeBandInt<=79))  ||
                    (activeBandInt == 89)                            ||
                    ((activeBandInt >= 92)  && (activeBandInt<=119)) ||
                    ((activeBandInt >= 180) && (activeBandInt<=199)) ||
                    ((activeBandInt >= 206) && (activeBandInt<=249)) ||
                    ((activeBandInt > 301)) ) {
                std::cout << "Invalid RFBand, retry .." << std::endl;
                isInvalid = true;
            }

            activeBand = static_cast<telux::tel::RFBand>(activeBandInt);
            lteDbCell.ci.activeBand = activeBand;
        } while(isInvalid);

        do {
            std::cout << "Enter dubious cell cause code (0 to 15)" << std::endl;
            std::cin >> maskInt;

            isInvalid = false;

            if ( (maskInt < 0) || (maskInt > 15) ) {
                std::cout << "Invalid dubious cause code, retry .." << std::endl;
                isInvalid = true;
            }
            mask = std::bitset<32>(maskInt);
            lteDbCell.ci.causeCodeMask = mask;
        } while (isInvalid);

        do {
            std::cout << "Enter cgi: " << std::endl;
            std::cin >> cgi;
            std::cout << std::endl;
            Utils::validateInput(cgi);
            isInvalid = false;

            if ((cgi < 0) || (cgi > std::numeric_limits<unsigned int>::max())) {
                std::cout << "Invalid cgi, retry .." << std::endl;
                isInvalid= true;
            }
        } while(isInvalid);

        lteDbCell.cgi = cgi;

        lteDbCellList.emplace_back(lteDbCell);
        std::cout << "Do you want to add another dubious cell ? (0-NO, 1-YES)" << std::endl;
        std::cin >> addConfig;

    } while(addConfig);

    auto networkManager = networkManagers_[slot_ - 1];
    auto err = networkManager->setLteDubiousCell(lteDbCellList);
    if (err == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\nSet LTE dubious cell succeed" << std::endl;
    } else {
        std::cout << "\nSet LTE dubious cell failed, err: " << Utils::getErrorCodeAsString(err)
            << std::endl;
    }
}

void NetworkMenu::setNrDubiousCell(std::vector<std::string> userInput) {

    std::vector<telux::tel::NrDubiousCell> nrDbCellList;
    bool addConfig = false, isInvalid=false;;
    std::string mcc, mnc;
    unsigned int arfcn=0, pci=0, maskInt=0, nrScsInt=0;
    unsigned long long cgi;
    int activeBandInt=0;
    telux::tel::RFBand activeBand;
    telux::tel::DbCellCauseCodeMask mask;

    do {
        telux::tel::NrDubiousCell nrDbCell;

        std::cout << "Enter MCC: " << std::endl;
        std::cin >> mcc;
        std::cout << std::endl;
        Utils::validateInput(mcc);
        nrDbCell.ci.mcc = mcc;

        std::cout << "Enter MNC: " << std::endl;
        std::cin >> mnc;
        std::cout << std::endl;
        Utils::validateInput(mnc);
        nrDbCell.ci.mnc = mnc;

        std::cout << "Enter arfcn: " << std::endl;
        std::cin >> arfcn;
        std::cout << std::endl;
        Utils::validateInput(arfcn);
        nrDbCell.ci.arfcn = arfcn;

        std::cout << "Enter pci: " << std::endl;
        std::cin >> pci;
        std::cout << std::endl;
        Utils::validateInput(pci);
        nrDbCell.ci.pci = pci;

        isInvalid = false;

        do {
            std::cout << "Enter band: (Valid int range corresponds to RFBand 0...19, 40...48, "
                << "80...88, 90, 91, 120...179, 200...205, 250...301)" << std::endl;
            std::cin >> activeBandInt;
            std::cout << std::endl;
            Utils::validateInput(activeBandInt);

            isInvalid = false;

            if ( ((activeBandInt >= 20)     && (activeBandInt<=39))  ||
                    ((activeBandInt >= 49)  && (activeBandInt<=79))  ||
                    (activeBandInt == 89)                            ||
                    ((activeBandInt >= 92)  && (activeBandInt<=119)) ||
                    ((activeBandInt >= 180) && (activeBandInt<=199)) ||
                    ((activeBandInt >= 206) && (activeBandInt<=249)) ||
                    ((activeBandInt > 301)) ) {
                std::cout << "Invalid RFBand, retry .." << std::endl;
                isInvalid = true;
            }

            activeBand = static_cast<telux::tel::RFBand>(activeBandInt);
            nrDbCell.ci.activeBand = activeBand;
        } while(isInvalid);


        do {
            std::cout << "Enter dubious cell cause code (0 to 15)" << std::endl;
            std::cin >> maskInt;

            isInvalid = false;

            if ( (maskInt < 0) || (maskInt > 15) ) {
                std::cout << "Invalid dubious cause code, retry .." << std::endl;
                isInvalid = true;
            }
            std::cout << std::endl;
            mask = std::bitset<32>(maskInt);
            nrDbCell.ci.causeCodeMask = mask;
        } while (isInvalid);

        do {
            std::cout << "Enter cgi: " << std::endl;
            std::cin >> cgi;
            std::cout << std::endl;
            Utils::validateInput(cgi);
            isInvalid = false;

            if ((cgi < 0) || (cgi > std::numeric_limits<unsigned long long>::max())) {
                std::cout << "Invalid cgi, retry .." << std::endl;
                isInvalid= true;
            }
        } while(isInvalid);
        nrDbCell.cgi = cgi;

        do {
            std::cout << "Enter NR subcarrier spacing: (0-SCS_15, 1-SCS_30, 2-SCS_60"
                << ", 3-SCS_120, 4-SCS_240)" << std::endl;
            std::cin >> nrScsInt;
            std::cout << std::endl;
            Utils::validateInput(nrScsInt);
            isInvalid = false;

            if ((nrScsInt < 0) || (nrScsInt > 5)) {
                std::cout << "Invalid sub carrier spacing, retry .." << std::endl;
                isInvalid= true;
            }
        } while(isInvalid);
        nrDbCell.spacing = static_cast<telux::tel::NrSubcarrierSpacing>(nrScsInt);

        nrDbCellList.emplace_back(nrDbCell);
        std::cout << "Do you want to add another dubious cell ? (0-NO, 1-YES)" << std::endl;
        std::cin >> addConfig;
        std::cout << std::endl;
    } while(addConfig);

    auto networkManager = networkManagers_[slot_ - 1];
    auto err = networkManager->setNrDubiousCell(nrDbCellList);
    if (err == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\nSet NR dubious cell succeed" << std::endl;
    } else {
        std::cout << "\nSet NR dubious cell failed, err: " << Utils::getErrorCodeAsString(err)
            << std::endl;
    }
}

void NetworkMenu::removeAllLteDubiousCell(std::vector<std::string> userInput) {

    std::vector<telux::tel::LteDubiousCell> lteDbCellList;
    auto networkManager = networkManagers_[slot_ - 1];

    auto err = networkManager->setLteDubiousCell(lteDbCellList);
    if (err == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\nRemove all LTE dubious cell succeed" << std::endl;
    } else {
        std::cout << "\nRemove all LTE dubious cell failed, err: "
            << Utils::getErrorCodeAsString(err)
            << std::endl;
    }
}

void NetworkMenu::removeAllNrDubiousCell(std::vector<std::string> userInput) {

    std::vector<telux::tel::NrDubiousCell> nrDbCellList;
    auto networkManager = networkManagers_[slot_ - 1];

    auto err = networkManager->setNrDubiousCell(nrDbCellList);
    if (err == telux::common::ErrorCode::SUCCESS) {
        std::cout << "\nRemove all NR dubious cell succeed" << std::endl;
    } else {
        std::cout << "\nRemove all NR dubious cell failed, err: "
            << Utils::getErrorCodeAsString(err)
            << std::endl;
    }

}
