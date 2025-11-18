/*
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * CardFileMenu provides menu options to read and write to different types of elementary files(EF).
 */

#include <chrono>
#include <iostream>
#include <ios>
#include <limits>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/CardApp.hpp>

#include "utils/Utils.hpp"
#include "CardFileMenu.hpp"
#include "MyCardListener.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void CardFileHandlerResponseCallback::EfReadLinearFixedResponseCb(telux::common::ErrorCode error,
    telux::tel::IccResult result) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Read Linear EF request failed with errorCode: " << static_cast<int>(error)
                << ":" << Utils::getErrorCodeAsString(error) << "\n IccResult "
            << result.toString() << "\n";
    } else {
        PRINT_CB << "Read Linear EF request successful " << "\n IccResult "
            << result.toString() << "\n";
    }
}

void CardFileHandlerResponseCallback::EfReadAllRecordsResponseCb(telux::common::ErrorCode error,
    std::vector<telux::tel::IccResult> records) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Read Linear EF All request failed with errorCode: " <<
            static_cast<int>(error) << ":" << Utils::getErrorCodeAsString(error) << " \n ";
    } else {
        PRINT_CB << "Read Linear EF All request successful " << " \n ";
        int recordNo = 1;
        for (auto iccResult: records) {
            std::cout << " Record" << recordNo << " " << iccResult.toString() << "\n";
            recordNo++;
        }
    }
}

void CardFileHandlerResponseCallback::EfReadTransparentResponseCb(telux::common::ErrorCode error,
    telux::tel::IccResult result) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Read Transparent EF request failed with errorCode: " <<
            static_cast<int>(error) << ":" << Utils::getErrorCodeAsString(error) << "\n IccResult "
            << result.toString() << "\n";
    } else {
        PRINT_CB << "Read Transparent EF request successful " << "\n IccResult "
            << result.toString() << "\n";
    }
}

void CardFileHandlerResponseCallback::EfWriteLinearFixedResponseCb(telux::common::ErrorCode error,
    telux::tel::IccResult result) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Write Linear EF request failed with errorCode: " <<
            static_cast<int>(error) << ":" << Utils::getErrorCodeAsString(error) << "\n IccResult "
            << result.toString() << "\n";
    } else {
        PRINT_CB << "Write Linear EF request successful " << "\n IccResult "
            << result.toString() << "\n";
    }
}

void CardFileHandlerResponseCallback::EfWriteTransparentResponseCb(telux::common::ErrorCode error,
    telux::tel::IccResult result) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Write Transparent EF request failed with errorCode: " <<
            static_cast<int>(error) << ":" << Utils::getErrorCodeAsString(error) << "\n IccResult "
            << result.toString() << "\n";
    } else {
        PRINT_CB << "Write Transparent EF request successful " << "\n IccResult "
            << result.toString() << "\n";
    }
}

void CardFileHandlerResponseCallback::EfGetFileAttributesCb(telux::common::ErrorCode error,
    telux::tel::IccResult result, telux::tel::FileAttributes attributes) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Get EF Attributes request failed with errorCode: " <<
            static_cast<int>(error) << ":" << Utils::getErrorCodeAsString(error) << "\n IccResult "
            << result.toString() << "\n";
    } else {
        PRINT_CB << "Get EF Attributes request successful " << "\n FileSize: "
            << attributes.fileSize <<  "\n RecordSize: " << attributes.recordSize
            << "\n RecordCount: " << attributes.recordCount <<"\n";
    }
}

CardFileMenu::CardFileMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

CardFileMenu::~CardFileMenu() {
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

bool CardFileMenu::init() {
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

    std::shared_ptr<ConsoleAppCommand> getSupportedAppsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "1", "Get_Supported_Apps", {},
         std::bind(&CardFileMenu::getSupportedApps, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> readEfLinearFixedCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "2", "Read_Linear_Fixed_EF", {},
            std::bind(&CardFileMenu::readEFLinearFixed, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> readEFLinearFixedAllCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "3", "Read_Linear_Fixed_EF_All", {},
            std::bind(&CardFileMenu::readEFLinearFixedAll, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> readEFTransparentCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "Read_Transparent_EF", {},
            std::bind(&CardFileMenu::readEFTransparent, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> writeEFLinearFixedCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "Write_Linear_Fixed_EF", {},
            std::bind(&CardFileMenu::writeEFLinearFixed, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> writeEFTransparentCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "Write_Transparent_EF", {},
            std::bind(&CardFileMenu::writeEFTransparent, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestEFAttributesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "7", "Request_EF_Attributes", {},
            std::bind(&CardFileMenu::requestEFAttributes, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> selectCardSlotCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "8", "Select_Card_Slot", {},
            std::bind(&CardFileMenu::selectCardSlot, this, std::placeholders::_1)));
    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListCardFileSubMenu
        = { getSupportedAppsCommand, readEfLinearFixedCommand,
            readEFLinearFixedAllCommand, readEFTransparentCommand, writeEFLinearFixedCommand,
            writeEFTransparentCommand, requestEFAttributesCommand };
    if (cards_.size() > 1) {
        commandsListCardFileSubMenu.emplace_back(selectCardSlotCommand);
    }
    addCommands(commandsListCardFileSubMenu);
    ConsoleApp::displayMenu();
    return true;
}

std::string CardFileMenu::cardStateToString(telux::tel::CardState state) {
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

std::string CardFileMenu::appTypeToString(telux::tel::AppType appType) {
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

std::string CardFileMenu::appStateToString(telux::tel::AppState appState) {
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

void CardFileMenu::getSupportedApps(std::vector<std::string> userInput) {
    auto card = cards_[slot_ - 1];
    if (card) {
        std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
        applications = card->getApplications();
        if (applications.size() != 0)  {
            for(auto cardApp : applications) {
            std::cout << "App type: " << appTypeToString(cardApp->getAppType()) << " \n ";
            std::cout << "App state: " << appStateToString(cardApp->getAppState()) << " \n ";
            std::cout << "AppId : " << cardApp->getAppId() << " \n ";
            }
        } else {
            std::cout <<"No supported applications"<< " \n ";
            telux::tel::CardState cardState;
            card->getState(cardState);
            std::cout << "Card State : " << cardStateToString(cardState) << " \n ";
        }
    }  else {
        std::cout << "ERROR: Unable to get card instance";
    }
}

void CardFileMenu::readEFLinearFixed(std::vector<std::string> userInput) {
    auto card = cards_[slot_ - 1];
    if (card) {
        uint16_t fileId;
        int recordNum;
        std::string aid = "";
        std::string filepath = "";
        char delimiter = '\n';
        std::cout << "Enter filepath: ";
        std::getline(std::cin, filepath, delimiter);
        std::cout << "Enter fileId : ";
        std::cin >> fileId;
        Utils::validateInput(fileId);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter recordNum : ";
        std::cin >> recordNum;
        Utils::validateInput(recordNum);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter AID: ";
        std::getline(std::cin, aid, delimiter);
        std::shared_ptr<telux::tel::ICardFileHandler> fileHandler = card->getFileHandler();
        if (fileHandler) {
            auto ret = fileHandler->readEFLinearFixed(filepath, fileId, recordNum, aid,
                CardFileHandlerResponseCallback::EfReadLinearFixedResponseCb);
            std::cout <<
                (ret == telux::common::Status::SUCCESS ?
                    "Read linear fixed file request sent successfully \n"
                    : "Read linear fixed file request failed \n");
        } else {
            std::cout << "ERROR: Card File Handler is null \n";
        }
    }  else {
        std::cout << "ERROR: Unable to get card instance";
    }
}

void CardFileMenu::readEFLinearFixedAll(std::vector<std::string> userInput) {
    auto card = cards_[slot_ - 1];
    if (card) {
        uint16_t fileId;
        std::string aid = "";
        std::string filepath = "";
        char delimiter = '\n';
        std::cout << "Enter filepath: ";
        std::getline(std::cin, filepath, delimiter);
        std::cout << "Enter fileId : ";
        std::cin >> fileId;
        Utils::validateInput(fileId);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter AID: ";
        std::getline(std::cin, aid, delimiter);
        std::shared_ptr<telux::tel::ICardFileHandler> fileHandler = card->getFileHandler();
        if (fileHandler) {
            auto ret = fileHandler->readEFLinearFixedAll(filepath, fileId, aid,
                CardFileHandlerResponseCallback::EfReadAllRecordsResponseCb);
            std::cout <<
                (ret == telux::common::Status::SUCCESS ?
                    "Read linear fixed file all request sent successfully \n"
                    : "Read linear fixed file all request failed \n");
        } else {
            std::cout << "ERROR: Card File Handler is null \n";
        }
    }  else {
        std::cout << "ERROR: Unable to get card instance";
    }
}

void CardFileMenu::readEFTransparent(std::vector<std::string> userInput) {
    auto card = cards_[slot_ - 1];
    if (card) {
        uint16_t fileId;
        int size;
        std::string aid = "";
        std::string filepath = "";
        char delimiter = '\n';
        std::cout << "Enter filepath: ";
        std::getline(std::cin, filepath, delimiter);
        std::cout << "Enter fileId : ";
        std::cin >> fileId;
        Utils::validateInput(fileId);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter size : ";
        std::cin >> size;
        Utils::validateInput(size);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter AID: ";
        std::getline(std::cin, aid, delimiter);
        std::shared_ptr<telux::tel::ICardFileHandler> fileHandler = card->getFileHandler();
        if (fileHandler) {
            auto ret = fileHandler->readEFTransparent(filepath, fileId, size, aid,
                CardFileHandlerResponseCallback::EfReadTransparentResponseCb);
            std::cout <<
                (ret == telux::common::Status::SUCCESS ?
                    "Read transparent file request sent successfully \n"
                    : "Read transparent file request failed \n");
        } else {
            std::cout << "ERROR: Card File Handler is null \n";
        }
    }  else {
        std::cout << "ERROR: Unable to get card instance";
    }
}

void CardFileMenu::writeEFLinearFixed(std::vector<std::string> userInput) {
    auto card = cards_[slot_ - 1];
    if (card) {
        uint16_t fileId;
        int dataLength;
        std::string aid = "";
        std::string pin2 = "";
        std::string filepath = "";
        char delimiter = '\n';
        std::cout << "Enter filepath: ";
        std::getline(std::cin, filepath, delimiter);
        int recordNum;
        std::vector<uint8_t> data;
        std::cout << "Enter fileId : ";
        std::cin >> fileId;
        Utils::validateInput(fileId);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter recordNum : ";
        std::cin >> recordNum;
        Utils::validateInput(recordNum);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter Pin2 : ";
        std::getline(std::cin, pin2, delimiter);
        std::cout << "Enter AID: ";
        std::getline(std::cin, aid, delimiter);
        std::cout << "Enter Data Length : ";
        std::cin >> dataLength;
        Utils::validateInput(dataLength);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        int tmpInp;
        for(int i = 0; i < dataLength; i++) {
            std::cout << "Enter DATA (" << i + 1 << ") :";
            std::cin >> tmpInp;
            Utils::validateInput(tmpInp);
            data.emplace_back((uint8_t)tmpInp);
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::shared_ptr<telux::tel::ICardFileHandler> fileHandler = card->getFileHandler();
        if (fileHandler) {
            auto ret = fileHandler->writeEFLinearFixed(filepath, fileId, recordNum, data, pin2, aid,
                CardFileHandlerResponseCallback::EfWriteLinearFixedResponseCb);
            std::cout <<
                (ret == telux::common::Status::SUCCESS ?
                    "Write linear fixed request sent successfully \n"
                    : "Write linear fixed request failed \n");
        } else {
            std::cout << "ERROR: Card File Handler is null \n";
        }
    }  else {
        std::cout << "ERROR: Unable to get card instance";
    }
}

void CardFileMenu::writeEFTransparent(std::vector<std::string> userInput) {
    auto card = cards_[slot_ - 1];
    if (card) {
        uint16_t fileId;
        int dataLength;
        std::string aid = "";
        std::string filepath = "";
        char delimiter = '\n';
        std::cout << "Enter filepath: ";
        std::getline(std::cin, filepath, delimiter);
        std::vector<uint8_t> data;
        std::cout << "Enter fileId : ";
        std::cin >> fileId;
        Utils::validateInput(fileId);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter AID: ";
        std::getline(std::cin, aid, delimiter);
        std::cout << "Enter Data Length : ";
        std::cin >> dataLength;
        Utils::validateInput(dataLength);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        int tmpInp;
        for(int i = 0; i < dataLength; i++) {
            std::cout << "Enter DATA (" << i + 1 << ") :";
            std::cin >> tmpInp;
            Utils::validateInput(tmpInp);
            data.emplace_back((uint8_t)tmpInp);
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::shared_ptr<telux::tel::ICardFileHandler> fileHandler = card->getFileHandler();
        if (fileHandler) {
            auto ret = fileHandler->writeEFTransparent(filepath, fileId, data, aid,
                CardFileHandlerResponseCallback::EfWriteTransparentResponseCb);
            std::cout <<
                (ret == telux::common::Status::SUCCESS ?
                    "Write transparent request sent successfully \n"
                    : "Write transparent request failed \n");
        } else {
            std::cout << "ERROR: Card File Handler is null \n";
        }
    }  else {
        std::cout << "ERROR: Unable to get card instance";
    }
}

void CardFileMenu::requestEFAttributes(std::vector<std::string> userInput) {
    auto card = cards_[slot_ - 1];
    if (card) {
        int efTypeIn;
        uint16_t fileId;
        std::string aid = "";
        std::string filepath = "";
        char delimiter = '\n';
        std::cout << "Enter filepath: ";
        std::getline(std::cin, filepath, delimiter);
        std::cout << "Enter EF Type ( 1-Transparent 2-LinearFixed ) : ";
        std::cin >> efTypeIn;
        Utils::validateInput(efTypeIn);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        telux::tel::EfType efType = static_cast<telux::tel::EfType>(efTypeIn);
        if ( efType <= telux::tel::EfType::UNKNOWN || efType > telux::tel::EfType::LINEAR_FIXED) {
            std::cout << "ERROR: Invalid EF type \n";
            return;
        }
        std::cout << "Enter fileId : ";
        std::cin >> fileId;
        Utils::validateInput(fileId);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::cout << "Enter AID: ";
        std::getline(std::cin, aid, delimiter);
        std::shared_ptr<telux::tel::ICardFileHandler> fileHandler = card->getFileHandler();
        if (fileHandler) {
            auto ret = fileHandler->requestEFAttributes(efType, filepath, fileId, aid,
                CardFileHandlerResponseCallback::EfGetFileAttributesCb);
            std::cout <<
                (ret == telux::common::Status::SUCCESS ?
                    "EF attributes request sent successfully \n"
                    : "EF attributes request failed \n");
        } else {
            std::cout << "ERROR: Card File Handler is null \n";
        }
    }  else {
        std::cout << "ERROR: Unable to get card instance";
    }
}

void CardFileMenu::selectCardSlot(std::vector<std::string> userInput) {
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
