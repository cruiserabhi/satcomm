/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
 * ImsServingSystemMenu provides menu options to invoke IMS Serving System APIs
 * such as requestImsRegStatus.
 */

#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include "utils/Utils.hpp"
#include "ImsServingSystemMenu.hpp"

#define INVALID_CONFIG_TYPE 0

using namespace telux::common;

ImsServingSystemMenu::ImsServingSystemMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

ImsServingSystemMenu::~ImsServingSystemMenu() {
    for( int i = 1; i <= slotCount_; i++ )
    {
        SlotId slotId = static_cast<SlotId>(i);
        if(imsServingSystemMgrs_[slotId] && imsServSysListeners_[slotId]) {
            imsServingSystemMgrs_[slotId]->deregisterListener(imsServSysListeners_[slotId]);
        }
    }
    imsServingSystemMgrs_.clear();
    imsServSysListeners_.clear();
}

bool ImsServingSystemMenu::init() {
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotCount_= MULTI_SIM_NUM_SLOTS;
    }
    for( int i = 1; i <= slotCount_; i++ ) {
        //  Get the PhoneFactory and ImsServingSystemManager instances.
        if (imsServingSystemMgrs_.find(static_cast<SlotId>(i)) != imsServingSystemMgrs_.end()) {
            std::cout << "IMS Serving System manager is already initialized on slotId "
                << i << std::endl;
        } else {
            std::promise<ServiceStatus> prom;
            //  Get the PhoneFactory and ImsServingSystemManager instances.
            auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
            auto imsServingSystemMgr = phoneFactory.getImsServingSystemManager(
                static_cast<SlotId>(i),[&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                    prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    prom.set_value(ServiceStatus::SERVICE_FAILED);
                }
            });
            if (!imsServingSystemMgr) {
                std::cout << "ERROR - Failed to get IMS Serving System instance \n";
                return false;
            }

            ServiceStatus imsServSysMgrStatus = imsServingSystemMgr->getServiceStatus();
            if (imsServSysMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
                std::cout << "IMS Serving System subsystem is not ready on slotId " << i
                    << ", Please wait " << std::endl;
            }
            imsServSysMgrStatus = prom.get_future().get();
            if (imsServSysMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
                std::cout << "IMS Serving System subsystem is ready on slotId " << i << std::endl;
                auto listener = std::make_shared<MyImsServSysListener>(static_cast<SlotId>(i));
                Status status = imsServingSystemMgr->registerListener(listener);
                imsServSysListeners_.emplace(static_cast<SlotId>(i), listener);
                if(status != Status::SUCCESS) {
                    std::cout << "ERROR - Failed to register listener \n";
                    return false;
                }

            } else {
                std::cout << "ERROR - Unable to initialize IMS Serving System subsystem on slotId "
                    << i << std::endl;
                return false;
            }
            imsServingSystemMgrs_.emplace(static_cast<SlotId>(i), imsServingSystemMgr);
        }
    }
    std::shared_ptr<ConsoleAppCommand> queryImsRegStateCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Get_Registration_Status",
        {}, std::bind(&ImsServingSystemMenu::requestImsRegStatus, this,
        std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> queryServiceStatusOverImsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Get_Service_Status",
        {}, std::bind(&ImsServingSystemMenu::requestServiceStatusOverIms, this,
        std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> queryPdpStatusOverImsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get_Pdp_Status",
        {}, std::bind(&ImsServingSystemMenu::requestPdpStatusOverIms, this,
        std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListImsServSysMenu
        = { queryImsRegStateCommand, queryServiceStatusOverImsCommand,
            queryPdpStatusOverImsCommand };

    addCommands(commandsListImsServSysMenu);
    ConsoleApp::displayMenu();
    return true;
}

void ImsServingSystemMenu::requestImsRegStatus(std::vector<std::string> userInput) {
    SlotId slotId = SlotId::DEFAULT_SLOT_ID;
    if (slotCount_ > DEFAULT_NUM_SLOTS) {
        slotId = static_cast<SlotId>(Utils::getValidSlotId());
    }

    if (imsServingSystemMgrs_[slotId]) {
        auto response = [this, slotId](telux::tel::ImsRegistrationInfo status, ErrorCode error) {
            MyImsServSysCallback::imsRegStateResponse(slotId, status, error);
        };
        Status status = imsServingSystemMgrs_[slotId]->requestRegistrationInfo(response);
        if (status == Status::SUCCESS) {
            std::cout << "IMS registration status request sent successfully " << std::endl;
        } else {
            std::cout << "ERROR - Failed to send registration status request,"
                    << "Status:" << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - ImsServingSystemManger on slot " << slotId
            << " is null" << std::endl;
    }
}

void ImsServingSystemMenu::requestServiceStatusOverIms(std::vector<std::string> userInput) {
    SlotId slotId = SlotId::DEFAULT_SLOT_ID;
    if (slotCount_ > DEFAULT_NUM_SLOTS) {
        slotId = static_cast<SlotId>(Utils::getValidSlotId());
    }

    if (imsServingSystemMgrs_[slotId]) {
        auto response = [this, slotId](telux::tel::ImsServiceInfo service, ErrorCode error) {
            MyImsServSysCallback::imsServiceStatusResponse(slotId, service, error);
        };
        Status status = imsServingSystemMgrs_[slotId]->requestServiceInfo(response);
        if (status == Status::SUCCESS) {
            std::cout << "IMS service status request sent successfully " << std::endl;
        } else {
            std::cout << "ERROR - Failed to send service status request,"
                    << "Status:" << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - ImsServingSystemManger on slot " << slotId
            << " is null" << std::endl;
    }
}

void ImsServingSystemMenu::requestPdpStatusOverIms(std::vector<std::string> userInput) {
    SlotId slotId = SlotId::DEFAULT_SLOT_ID;
    if (slotCount_ > DEFAULT_NUM_SLOTS) {
        slotId = static_cast<SlotId>(Utils::getValidSlotId());
    }

    if (imsServingSystemMgrs_[slotId]) {
        auto response = [this, slotId](telux::tel::ImsPdpStatusInfo status, ErrorCode error) {
            MyImsServSysCallback::imsPdpStatusResponse(slotId, status, error);
        };
        Status status = imsServingSystemMgrs_[slotId]->requestPdpStatus(response);
        if (status == Status::SUCCESS) {
            std::cout << "IMS pdp status request sent successfully " << std::endl;
        } else {
            std::cout << "ERROR - Failed to send service status request,"
                    << "Status:" << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - ImsServingSystemManger on slot " << slotId
            << " is null" << std::endl;
    }
}
