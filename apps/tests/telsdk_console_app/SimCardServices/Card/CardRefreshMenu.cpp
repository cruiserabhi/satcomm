/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * CardRefreshMenu provides menu options to control the SIM refresh procedure.
 */

#include <chrono>
#include <iostream>
#include <ios>
#include <limits>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/CardApp.hpp>

#include "utils/Utils.hpp"
#include "CardRefreshMenu.hpp"
#include "MyCardListener.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void CardRefreshResponseCallback::RefreshLastEventResponseCb(
    telux::tel::RefreshStage stage, telux::tel::RefreshMode mode,
    std::vector<telux::tel::IccFile> efFiles, telux::tel::RefreshParams config,
    telux::common::ErrorCode error) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request Last refresh event failed with errorCode: " <<
            static_cast<int>(error) << ":" << Utils::getErrorCodeAsString(error)
            << " \n ";
    } else {
        PRINT_CB << "Request Last refresh event successful " << " \n ";
        int fileNo = 1;
        PRINT_CB << "Refresh Stage is "
            << MyCardListener::refreshStageToString(stage)
            << " ,Refresh Mode is "
            << MyCardListener::refreshModeToString(mode)
            << " ,Session Type is "
            << MyCardListener::sessionTypeToString(config.sessionType)
            << ((!config.aid.empty()) ? " ,AID is " : "")
            << ((!config.aid.empty()) ? config.aid : "") << " \n ";
        for (auto file: efFiles) {
            std::cout << " EF file" << fileNo << " path is " << file.filePath
                << " ID is " << file.fileId << "\n";
            fileNo++;
        }
    }
}

void CardRefreshResponseCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Refresh command successful." << std::endl;
   } else {
      PRINT_CB << "Refresh command failed\n error: " << static_cast<int>(error)
             << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

CardRefreshMenu::CardRefreshMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

CardRefreshMenu::~CardRefreshMenu() {
    if (cardManager_ && cardListener_) {
       cardManager_->removeListener(cardListener_);
    }
    for (auto index = 0; index < cards_.size() ; index++) {
        cards_[index] = nullptr;
    }
    if (cardListener_) {
       cardListener_ = nullptr;
    }
    if (cardManager_) {
       cardManager_ = nullptr;
    }
}

bool CardRefreshMenu::init() {
    //  Get the PhoneFactory and PhoneManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    std::promise<telux::common::ServiceStatus> cardMgrprom;
    cardManager_ = phoneFactory.getCardManager([&](telux::common::ServiceStatus status) {
        cardMgrprom.set_value(status);
    });

    if (!cardManager_) {
       std::cout << "Failed to get CardManager instance \n";
       return false;
    }

    //  Check if call manager subsystem is ready
    telux::common::ServiceStatus cardMgrStatus = cardManager_->getServiceStatus();
    //  If call manager subsystem is not ready, wait for it to be ready
    if (cardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Card Manager subsystem is not ready, Please wait \n";
    }
    cardMgrStatus = cardMgrprom.get_future().get();
    //  If call manager subsystem is ready
    if (cardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Card Manager subsystem is ready \n" << std::endl;
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
        cardListener_ = std::make_shared<MyCardListener>();
        // registering Listener
        status = cardManager_->registerListener(cardListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Unable to registerListener" << " \n ";
        }
    } else {
        std::cout << "ERROR - Unable to initialize Call Manager subSystem" << "\n";
        return false;
    }

    std::shared_ptr<ConsoleAppCommand> configureRefreshVoteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "1", "Configure_Refresh_Vote", {},
         std::bind(&CardRefreshMenu::configureRefreshVote, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> allowRefreshCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "2", "Allow_Refresh", {},
            std::bind(&CardRefreshMenu::allowRefresh, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> refreshCompleteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "3", "Complete_Refresh", {},
            std::bind(&CardRefreshMenu::refreshComplete, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestLastEventCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "Request_Last_Event", {},
            std::bind(&CardRefreshMenu::requestLastEvent, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> selectCardSlotCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "Select_Card_Slot", {},
            std::bind(&CardRefreshMenu::selectCardSlot, this, std::placeholders::_1)));
    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListCardRefreshSubMenu
        = { configureRefreshVoteCommand, allowRefreshCommand, refreshCompleteCommand,
            requestLastEventCommand};
    if (cards_.size() > 1) {
        commandsListCardRefreshSubMenu.emplace_back(selectCardSlotCommand);
    }
    addCommands(commandsListCardRefreshSubMenu);
    ConsoleApp::displayMenu();
    return true;
}

telux::tel::RefreshParams CardRefreshMenu::enterRefreshParams
    (std::vector<std::string> userInput) {
    int type = 0;
    char delimiter = '\n';
    telux::tel::RefreshParams config = {};
    std::cout << "Enter Card Refresh session type(0 - PRIMARY, 2 - SECONDARY,\n"
        << "4 - NONPROVISIONING_SLOT_1, 5 - NONPROVISIONING_SLOT_2,\n"
        << "6 - CARD_ON_SLOT_1, 7 - CARD_ON_SLOT_2):";
    std::cin >> type;
    Utils::validateInput(type);
    if(type == static_cast<int>(telux::tel::SessionType::PRIMARY) ||
        type == static_cast<int>(telux::tel::SessionType::SECONDARY) ||
        (type >= static_cast<int>(telux::tel::SessionType::NONPROVISIONING_SLOT_1) &&
        type <= static_cast<int>(telux::tel::SessionType::CARD_ON_SLOT_2))) {
        config.sessionType = static_cast<telux::tel::SessionType>(type);
    } else {
        std::cout << "Invalid session type input, try again" << std::endl;
        return config;
    }
    if(config.sessionType == telux::tel::SessionType::NONPROVISIONING_SLOT_1 ||
        config.sessionType == telux::tel::SessionType::NONPROVISIONING_SLOT_2) {
        std::cout << "Enter AID: ";
        std::getline(std::cin, config.aid, delimiter);
    }

    return config;
}

void CardRefreshMenu::configureRefreshVote(std::vector<std::string> userInput) {
    if (cardManager_) {
        bool voteRefresh = false;
        auto status = telux::common::Status::FAILED;
        int temp = 0;
        char delimiter = '\n';
        std::vector<telux::tel::IccFile> efFiles;
        std::cout << "Enter Card Refresh vote state(1 - Vote, 0 - No Vote): ";
        std::cin >> temp;
        Utils::validateInput(temp);
        if(temp == 1) {
            voteRefresh = true;
        } else if(temp == 0) {
            voteRefresh = false;
        } else {
            std::cout << "Invalid state input, try again" << std::endl;
            return;
        }
        std::string filepath = "";
        uint16_t fileId;
        std::cout << "Registered file list (q - exit)\n";
        while(true) {
            std::cout << "\nEnter file path: ";
            std::getline(std::cin, filepath, delimiter);
            if (filepath.empty()) {
                std::cout << "File path input is empty \n";
                return;
            }
            if (filepath == "q") {
                break;
            }

            std::cout << "Enter fileId :";
            std::cin >> fileId;
            Utils::validateInput(fileId);
            efFiles.push_back({fileId, filepath});
        }
        telux::tel::RefreshParams config = enterRefreshParams(userInput);
        status = cardManager_->setupRefreshConfig(
            static_cast<SlotId>(slot_), true, voteRefresh, efFiles, config,
            CardRefreshResponseCallback::commandResponse);
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

void CardRefreshMenu::allowRefresh(std::vector<std::string> userInput) {
    if (cardManager_) {
        bool allowRefresh = false;
        auto status = telux::common::Status::FAILED;
        int temp = 0;

        std::cout << "Enter Card Refresh allow state(1 - Allow, 0 - Disallow): ";
        std::cin >> temp;
        Utils::validateInput(temp);
        if(temp == 1) {
            allowRefresh = true;
        } else if(temp == 0) {
            allowRefresh = false;
        } else {
            std::cout << "Invalid state input, try again" << std::endl;
            return;
        }
        telux::tel::RefreshParams config = enterRefreshParams(userInput);
        status = cardManager_->allowCardRefresh(static_cast<SlotId>(slot_),
            allowRefresh, config,
            CardRefreshResponseCallback::commandResponse);
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

void CardRefreshMenu::refreshComplete(std::vector<std::string> userInput) {
    if (cardManager_) {
        bool completeRefresh = false;
        auto status = telux::common::Status::FAILED;
        int temp = 0;

        std::cout << "Enter Card Refresh complete state(1 - Complete, 0 - Incomplete): ";
        std::cin >> temp;
        Utils::validateInput(temp);
        if(temp == 1) {
            completeRefresh = true;
        } else if(temp == 0) {
            completeRefresh = false;
        } else {
            std::cout << "Invalid state input, try again" << std::endl;
            return;
        }
        telux::tel::RefreshParams config = enterRefreshParams(userInput);
        status = cardManager_->confirmRefreshHandlingCompleted(static_cast<SlotId>(slot_),
            completeRefresh, config,
            CardRefreshResponseCallback::commandResponse);
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

void CardRefreshMenu::requestLastEvent(std::vector<std::string> userInput) {
    if (cardManager_) {
        auto status = telux::common::Status::FAILED;
        telux::tel::RefreshParams config = enterRefreshParams(userInput);
        status = cardManager_->requestLastRefreshEvent(static_cast<SlotId>(slot_),
            config, CardRefreshResponseCallback::RefreshLastEventResponseCb);
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

void CardRefreshMenu::selectCardSlot(std::vector<std::string> userInput) {
    std::string slotSelection;
    char delimiter = '\n';
    std::cout << "Enter the desired card slot (1-Primary, 2-Secondary): ";
    std::getline(std::cin, slotSelection, delimiter);
    if (!slotSelection.empty()) {
        try {
            int slot = std::stoi(slotSelection);
            if (slot > MAX_SLOT_ID  || slot < DEFAULT_SLOT_ID) {
                std::cout << "Invalid slot entered, using default slot" << " \n ";
                slot_ = DEFAULT_SLOT_ID;
            } else {
                slot_ = slot;
            }
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
            << slotSelection << " \n ";
            return;
        }
    } else {
        std::cout << "Empty input, enter the correct slot" << " \n ";
    }
}
