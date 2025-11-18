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

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "SocksMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

SocksMenu::SocksMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    socksManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

SocksMenu::~SocksMenu() {
}

bool SocksMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (socksManager_ == nullptr) {
        auto initCb = std::bind(&SocksMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();
        auto localSocksMgr = dataFactory.getSocksManager(
            telux::data::OperationType::DATA_LOCAL, initCb);
        if(localSocksMgr) {
            socksManager_ = localSocksMgr;
        }
        auto remoteSocksMgr = dataFactory.getSocksManager(
            telux::data::OperationType::DATA_REMOTE, initCb);
        if(remoteSocksMgr) {
            socksManager_ = remoteSocksMgr;
        }
        if(socksManager_ == nullptr ) {
            std::cout << "\nUnable to create Socks Manager ... " << std::endl;
            return false;
        }
        socksManager_->registerListener(shared_from_this());
        {
            std::unique_lock<std::mutex> lck(mtx_);
            //Socks Manager is guaranteed to be valid pointer at this point. If manager
            //initialization fails and factory invalidated it's own pointer to Socks manager before
            //reaching this point, reference count of Socks manager should still be 1
            telux::common::ServiceStatus subSystemStatus = socksManager_->getServiceStatus();
            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
                std::cout << "\nInitializing Socks Manager, Please wait ..." << std::endl;
                cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
                subSystemStatus = socksManager_->getServiceStatus();
            }
            //At this point, initialization should be either AVAILABLE or FAIL
            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                std::cout << "\nSocks Manager is ready" << std::endl;
            }
            else {
                std::cout << "\nSocks Manager initialization failed" << std::endl;
                socksManager_ = nullptr;
                return false;
            }
        }
    }
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> enableSocks
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "socks_enablement",
                {}, std::bind(&SocksMenu::enableSocks, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {enableSocks};

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void SocksMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void SocksMenu::enableSocks(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int enableEntry;
    bool subSystemStatus = false;

    std::cout << "Enable/Disable Socks Proxy\n";

    std::cout << "Enter Enablement Type (0-Disable, 1-Enable): ";
    std::cin >> enableEntry;
    Utils::validateInput(enableEntry);
    if (enableEntry < 0 || enableEntry >1) {
        std::cout << "Invalid Entry. Please try again ...\n";
        return;
    }
    bool enablement = (enableEntry == 0 ? false : true);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "enableSocks Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = socksManager_->enableSocks(enablement,respCb);
    Utils::printStatus(retStat);
}
