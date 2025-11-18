/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "ServingSystemMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

#define PRINT_NOTIFICATION std::cout << "\n\033[1;35mNOTIFICATION: \033[0m"

DataServingSystemMenu::DataServingSystemMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    dataServingSystemManagers_.clear();
    addMenuCmds_ = false;
    subSystemStatusUpdated_ = false;
    dataServingSystemListeners_[DEFAULT_SLOT_ID] =
        std::make_shared<ServingSystemListener>(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        dataServingSystemListeners_[SLOT_ID_2] =
            std::make_shared<ServingSystemListener>(SLOT_ID_2);
    }
}

DataServingSystemMenu::~DataServingSystemMenu() {
}

bool DataServingSystemMenu::init() {
    bool initStat = initServingSystemManagerAndListener(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        initStat |= initServingSystemManagerAndListener(SLOT_ID_2);
    }

    if (addMenuCmds_ == false) {
        addMenuCmds_ = true;
        std::shared_ptr<ConsoleAppCommand> getDrbStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "get_drb_status", {},
            std::bind(&DataServingSystemMenu::getDrbStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestServiceStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "request_service_status", {},
            std::bind(&DataServingSystemMenu::requestServiceStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestRoamingStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "request_roaming_status", {},
            std::bind(&DataServingSystemMenu::requestRoamingStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestNrIconType =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "request_nr_icon_type", {},
            std::bind(&DataServingSystemMenu::requestNrIconType, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> makeDormant =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "make_dormant", {},
            std::bind(&DataServingSystemMenu::makeDormant, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {getDrbStatus,
            requestServiceStatus, requestRoamingStatus, requestNrIconType, makeDormant};
        addCommands(commandsList);
    }

    ConsoleApp::displayMenu();
    return initStat;
}

bool DataServingSystemMenu::initServingSystemManagerAndListener(SlotId slotId) {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    subSystemStatusUpdated_ = false;
    auto initCb = std::bind(&DataServingSystemMenu::onInitCompleted, this,
        std::placeholders::_1);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto ServingSystemMgr =
        dataFactory.getServingSystemManager(slotId, initCb);
    if(ServingSystemMgr) {
        ServingSystemMgr->registerListener(
            dataServingSystemListeners_[slotId]);

        std::cout << "\nInitializing Serving Manager on Slot "
                    << static_cast<int>(slotId) << ", Please wait..." << std::endl;
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
        subSystemStatus = ServingSystemMgr->getServiceStatus();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nServing System Manager on slot "
                      << static_cast<int>(slotId)  << " is ready" << std::endl;
            retValue = true;
        }
        else {
            std::cout << "\nServing System Manager on slot "
                      << static_cast<int>(slotId)  << " is not ready" << std::endl;
            //If manager exist - deregister and remove it
            if (dataServingSystemManagers_.find(slotId) != dataServingSystemManagers_.end()) {
                dataServingSystemManagers_[slotId]->deregisterListener(
                    dataServingSystemListeners_[slotId]);
                dataServingSystemManagers_.erase(slotId);
            }
            retValue = false;
        }

        //If it is new manager and initialization passed
        if ((retValue == true) &&
            (dataServingSystemManagers_.find(slotId) == dataServingSystemManagers_.end())) {
            dataServingSystemManagers_.emplace(slotId, ServingSystemMgr);
        }
    }
    return retValue;
}

void DataServingSystemMenu::onInitCompleted(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DataServingSystemMenu::getDrbStatus(std::vector<std::string> inputCommand) {
    std::cout << "Get DRB Status\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    if (dataServingSystemManagers_.find(static_cast<SlotId>(slotId)) ==
        dataServingSystemManagers_.end()) {
        std::cout << "Serving System Manager on SlotId: " << slotId << " is not ready" << std::endl;
        return;
    }

    telux::data::DrbStatus stat =
        dataServingSystemManagers_[static_cast<SlotId>(slotId)]->getDrbStatus();
    std::cout << "Current Drb Status is : " << DataUtils::drbStatusToString(stat) << std::endl;
}

void DataServingSystemMenu::requestServiceStatus(std::vector<std::string> inputCommand) {
    std::cout << "Request Service Status\n";

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    if (dataServingSystemManagers_.find(static_cast<SlotId>(slotId)) ==
        dataServingSystemManagers_.end()) {
        std::cout << "Serving System Manager on SlotId: " << slotId << " is not ready" << std::endl;
        return;
    }

    // Callback
    auto respCb = [slotId](telux::data::ServiceStatus serviceStatus,
                           telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "requestServiceStatus Response on slotid " << static_cast<int>(slotId);
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            if(serviceStatus.serviceState == telux::data::DataServiceState::OUT_OF_SERVICE) {
                std::cout << "Current Status is Out Of Service" << std::endl;
            } else {
                std::cout << "Current Status is In Service" << std::endl;
                std::cout << "Preferred Rat is "
                            << DataUtils::serviceRatToString(serviceStatus.networkRat) << std::endl;
            }
        }
        else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error)
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        }
    };

    telux::common::Status retStat =
        dataServingSystemManagers_[static_cast<SlotId>(slotId)]->requestServiceStatus(respCb);
    Utils::printStatus(retStat);
}

void DataServingSystemMenu::requestRoamingStatus(std::vector<std::string> inputCommand) {
    std::cout << "Request Roaming Status\n";
    telux::common::Status retStat;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    if (dataServingSystemManagers_.find(static_cast<SlotId>(slotId)) ==
        dataServingSystemManagers_.end()) {
        std::cout << "Serving System Manager on SlotId: " << slotId << " is not ready" << std::endl;
        return;
    }

    // Callback
    auto respCb = [slotId](
            telux::data::RoamingStatus roamingStatus, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "requestRoamingStatus Response on slotid " << static_cast<int>(slotId);
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            bool isRoaming = roamingStatus.isRoaming;
            if(isRoaming) {
                std::cout << "System is in Roaming State" << std::endl;
                std::cout << "Roaming Type: ";
                switch(roamingStatus.type)  {
                    case telux::data::RoamingType::INTERNATIONAL:
                        std::cout << "International" << std::endl;
                    break;
                    case telux::data::RoamingType::DOMESTIC:
                        std::cout << "Domestic" << std::endl;
                    break;
                    default:
                        std::cout << "Unknown" << std::endl;
                }
            } else {
                std::cout << "System is not in Roaming State" << std::endl;
            }
        }
        else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error)
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        }
    };

    retStat =
        dataServingSystemManagers_[static_cast<SlotId>(slotId)]->requestRoamingStatus(respCb);
    Utils::printStatus(retStat);
}

void DataServingSystemMenu::requestNrIconType(std::vector<std::string> inputCommand) {
    std::cout << "Request Nr Icon Type\n";
    telux::common::Status retStat;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    if (dataServingSystemManagers_.find(static_cast<SlotId>(slotId)) ==
        dataServingSystemManagers_.end()) {
        std::cout << "Serving System Manager on SlotId: " << slotId << " is not ready" << std::endl;
        return;
    }

    // Callback
    auto respCb = [slotId](
            telux::data::NrIconType type, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "requestNrIconType Response on slotid " << static_cast<int>(slotId);
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            std::cout << "Nr Icon Type: ";
            switch(type)  {
                case telux::data::NrIconType::BASIC:
                    std::cout << "Basic" << std::endl;
                break;
                case telux::data::NrIconType::UWB:
                    std::cout << "Ultrawide Band" << std::endl;
                break;
                default:
                    std::cout << "Unknown" << std::endl;
            }
        }
        else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error)
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        }
    };

    retStat =
        dataServingSystemManagers_[static_cast<SlotId>(slotId)]->requestNrIconType(respCb);
    Utils::printStatus(retStat);
}

void DataServingSystemMenu::makeDormant(std::vector<std::string> inputCommand) {
    std::cout << "Make Dormant\n";
    int slotId = DEFAULT_SLOT_ID;
    auto respCb = [](telux::common::ErrorCode errCode) {
        std::cout << std::endl << std::endl;
        std::cout << "Callback: "
                  << "makeDormant Response "
                  <<((errCode == telux::common::ErrorCode::SUCCESS)? "is Successful":"failed")
                  << ". ErrorCode = " << static_cast<int>(errCode)
                  << ", Descrition: " << Utils::getErrorCodeAsString(errCode) << std::endl;
    };
    telux::common::Status status =
        dataServingSystemManagers_[static_cast<SlotId>(slotId)]->makeDormant(respCb);
    Utils::printStatus(status);
}

