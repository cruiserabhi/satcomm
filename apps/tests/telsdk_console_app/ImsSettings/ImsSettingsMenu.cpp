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
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * ImsSettingsMenu provides menu options to invoke IMS service configuration parameters
 * such as requestImsSettingsServiceConfig, setImsSettingsServiceConfig.
 */

#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include "utils/Utils.hpp"
#include "ImsSettingsMenu.hpp"
#include "MyImsSettingsHandler.hpp"

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2
#define INVALID_CONFIG_TYPE 0
#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"

using namespace telux::common;

ImsSettingsMenu::ImsSettingsMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

ImsSettingsMenu::~ImsSettingsMenu() {
    if (imsSettingsMgr_ && imsSettingsListener_) {
        imsSettingsMgr_->deregisterListener(imsSettingsListener_);
    }
    if (imsSettingsListener_) {
        imsSettingsListener_ = nullptr;
    }
    if (imsSettingsMgr_) {
        imsSettingsMgr_ = nullptr;
    }

}

bool ImsSettingsMenu::init() {
    if (imsSettingsMgr_ == nullptr) {
        std::promise<ServiceStatus> prom;
        //  Get the PhoneFactory and ImsSettingsManager instances.
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
        imsSettingsMgr_ = phoneFactory.getImsSettingsManager([&](ServiceStatus status) {
            if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
        if (!imsSettingsMgr_) {
            std::cout << "ERROR - Failed to get IMS settings instance \n";
            return false;
        }

        ServiceStatus immsMgrStatus = imsSettingsMgr_->getServiceStatus();
        if (immsMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "IMS settings subsystem is not ready, Please wait \n";
        }
        immsMgrStatus = prom.get_future().get();
        if (immsMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "IMS settings subsystem is ready \n";
            imsSettingsListener_ = std::make_shared<ImsSettingsListener>();
            Status status = imsSettingsMgr_->registerListener(imsSettingsListener_);
            if(status != Status::SUCCESS) {
                std::cout << "ERROR - Failed to register listener \n";
                return false;
            }

        } else {
            std::cout << "ERROR - Unable to initialize IMS Settings subsystem \n";
            return false;
        }
    } else {
        std::cout << "IMS settings manager is already initialized \n";
    }

    std::shared_ptr<ConsoleAppCommand> getImsServiceConfig
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Get_Service_Configurations",
        {}, std::bind(&ImsSettingsMenu::requestImsServiceConfig, this,
        std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setImsServiceConfig
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Set_Service_Configurations",
        {}, std::bind(&ImsSettingsMenu::setImsServiceConfig, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setImsUserAgentConfig
        = std::make_shared<ConsoleAppCommand>
        (ConsoleAppCommand("3", "Set_ImsUserAgent_Configuration",
        {}, std::bind(&ImsSettingsMenu::setImsUserAgentConfig, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getImsUserAgentConfig
        = std::make_shared<ConsoleAppCommand>
        (ConsoleAppCommand("4", "Get_ImsUserAgent_Configuration",
        {}, std::bind(&ImsSettingsMenu::requestImsUserAgentConfig, this,
        std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getImsVonr
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Request_IMS_VoNR_Status",
        {}, std::bind(&ImsSettingsMenu::requestImsVonr, this,
        std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setImsVonr
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Toggle_VoNR",
        {}, std::bind(&ImsSettingsMenu::setImsVonr, this, std::placeholders::_1)));


    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListImsSettingsMenu
        = { getImsServiceConfig, setImsServiceConfig,
            setImsUserAgentConfig, getImsUserAgentConfig,
            getImsVonr, setImsVonr };

    addCommands(commandsListImsSettingsMenu);
    ConsoleApp::displayMenu();
    return true;
}

void ImsSettingsMenu::requestImsUserAgentConfig(std::vector<std::string> userInput) {
   if (imsSettingsMgr_) {
      SlotId slotId = SlotId::DEFAULT_SLOT_ID;
      if (DeviceConfig::isMultiSimSupported()) {
         slotId = static_cast<SlotId>(Utils::getValidSlotId());
      }
      Status status = imsSettingsMgr_->requestSipUserAgent(slotId,
         MyImsSettingsCallback::onRequestImsSipUserAgentConfig);
      if (status == Status::SUCCESS) {
         std::cout << "IMS SIP user agent configuration request sent successfully \n";
      } else {
         std::cout << "ERROR - Failed to send IMS SIP user agent configuration request,"
                   << "Status:" << static_cast<int>(status) << "\n";
         Utils::printStatus(status);
      }
   } else {
      std::cout << "ERROR - ImsSettingsManger is null \n";
   }
}

void ImsSettingsMenu::requestImsServiceConfig(std::vector<std::string> userInput) {
   if (imsSettingsMgr_) {
      SlotId slotId = SlotId::DEFAULT_SLOT_ID;
      if (DeviceConfig::isMultiSimSupported()) {
         slotId = static_cast<SlotId>(Utils::getValidSlotId());
      }
      Status status = imsSettingsMgr_->requestServiceConfig(slotId,
         MyImsSettingsCallback::onRequestImsServiceConfig);
      if (status == Status::SUCCESS) {
         std::cout << "IMS service configuration request sent successfully \n";
      } else {
         std::cout << "ERROR - Failed to send IMS service config request,"
                   << "Status:" << static_cast<int>(status) << "\n";
         Utils::printStatus(status);
      }
   } else {
      std::cout << "ERROR - ImsSettingsManger is null \n";
   }
}

void ImsSettingsMenu::setImsServiceConfig(std::vector<std::string> userInput) {
   if (imsSettingsMgr_) {
      SlotId slotId = SlotId::DEFAULT_SLOT_ID;
      if (DeviceConfig::isMultiSimSupported()) {
         slotId = static_cast<SlotId>(Utils::getValidSlotId());
      }
      telux::tel::ImsServiceConfig config{};
      std::string configSelection = "";
      std::string enableSelection = "";
      char delimiter = '\n';
      int configType = INVALID_CONFIG_TYPE;
      bool enable = false;
      std::cout  << "Available IMS Service configurations \n 1 - VOIMS \n 2 - IMS Service \n "
                 << "3 - SMS \n 4 - RTT \n q - exit \n ";
      while(true) {
         std::cout << "\nSelect the configuration type: ";
         std::getline(std::cin, configSelection, delimiter);
         if (configSelection.empty()) {
            std::cout << "Configuration type input is empty \n";
            return;
         }
         if (configSelection == "q") {
            break;
         }

         try {
             configType = std::stoi(configSelection);
         } catch (const std::exception &e) {
             std::cout << "ERROR::Invalid input, please enter a numerical value \n";
             return;
         }
         std::cout << "Enable/Disable config(1 - Enable, 0 - Disable) :";
         std::getline(std::cin, enableSelection, delimiter);
         if (enableSelection.empty()) {
            std::cout << " Enable/Disable selection is empty \n";
            return;
         }
         try {
             enable = std::stoi(enableSelection);
         } catch (const std::exception &e) {
             std::cout << "ERROR::Invalid input, please enter a numerical value \n";
             return;
         }
         switch (configType){
             case telux::tel::ImsServiceConfigType::IMSSETTINGS_VOIMS:
                 config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_VOIMS);
                 config.voImsEnabled = enable;
                 break;
             case telux::tel::ImsServiceConfigType::IMSSETTINGS_IMS_SERVICE:
                 config.configValidityMask.set(
                     telux::tel::ImsServiceConfigType::IMSSETTINGS_IMS_SERVICE);
                 config.imsServiceEnabled = enable;
                 break;
             case telux::tel::ImsServiceConfigType::IMSSETTINGS_SMS:
                 config.configValidityMask.set(
                     telux::tel::ImsServiceConfigType::IMSSETTINGS_SMS);
                 config.smsEnabled = enable;
                 break;
             case telux::tel::ImsServiceConfigType::IMSSETTINGS_RTT:
                 config.configValidityMask.set(
                     telux::tel::ImsServiceConfigType::IMSSETTINGS_RTT);
                 config.rttEnabled = enable;
                 break;
             default:
                 std::cout << "Invalid configuration selection \n";
                 return;
         }
      }
      if (config.configValidityMask.any()) {
          Status status = imsSettingsMgr_->setServiceConfig(slotId, config,
             MyImsSettingsCallback::onResponseCallback);
          if (status == Status::SUCCESS) {
             std::cout << "Set IMS service request sent successfully \n";
          } else {
             std::cout << "ERROR - Failed to send set IMS service config request, Status:"
                       << static_cast<int>(status) << "\n";
             Utils::printStatus(status);
          }
      }
   } else {
      std::cout << "ERROR - ImsSettingsManger is null \n";
   }
}

void ImsSettingsMenu::setImsUserAgentConfig(std::vector<std::string> userInput) {
   if (imsSettingsMgr_) {
      SlotId slotId = SlotId::DEFAULT_SLOT_ID;
      if (DeviceConfig::isMultiSimSupported()) {
         slotId = static_cast<SlotId>(Utils::getValidSlotId());
      }
      std::string userAgent = "";
      char delimiter = '\n';
      std::cout  << "Input SIP User Agent \n ";
      std::getline(std::cin, userAgent, delimiter);
      Status status = imsSettingsMgr_->setSipUserAgent(slotId, userAgent,
            MyImsSettingsCallback::onResponseCallback);
      if (status == Status::SUCCESS) {
          std::cout << "Set IMS user agent request sent successfully \n";
      } else {
          std::cout << "ERROR - Failed to send set IMS user agent request, Status:"
                    << static_cast<int>(status) << "\n";
          Utils::printStatus(status);
      }
   } else {
      std::cout << "ERROR - ImsSettingsManger is null \n";
   }
}

void ImsSettingsMenu::requestImsVonr(std::vector<std::string> userInput) {
   if (imsSettingsMgr_) {
      SlotId slotId = SlotId::DEFAULT_SLOT_ID;
      if (DeviceConfig::isMultiSimSupported()) {
         slotId = static_cast<SlotId>(Utils::getValidSlotId());
      }
      Status status = imsSettingsMgr_->requestVonrStatus(slotId,
         MyImsSettingsCallback::onRequestImsVonr);
      if (status == Status::SUCCESS) {
         std::cout << "IMS VoNR request sent successfully \n";
      } else {
         std::cout << "ERROR - Failed to send IMS VoNR request,"
                   << "Status:" << static_cast<int>(status) << "\n";
         Utils::printStatus(status);
      }
   } else {
      std::cout << "ERROR - ImsSettingsManger is null \n";
   }
}

void ImsSettingsMenu::setImsVonr(std::vector<std::string> userInput) {
   if (imsSettingsMgr_) {
      SlotId slotId = SlotId::DEFAULT_SLOT_ID;
      if (DeviceConfig::isMultiSimSupported()) {
         slotId = static_cast<SlotId>(Utils::getValidSlotId());
      }

      std::string enableSelection = "";
      char delimiter = '\n';
      bool enable = false;

      std::cout << "Enable/Disable IMS VoNR(1 - Enable, 0 - Disable) :";
      std::getline(std::cin, enableSelection, delimiter);
      if (enableSelection.empty()) {
         std::cout << " Enable/Disable selection is empty \n";
         return;
      }
      try {
         enable = std::stoi(enableSelection);
         if (enable != 0 && enable != 1) {
            std::cout << "Invalid input " << std::endl;
            return;
         }
      } catch (const std::exception &e) {
         std::cout << "ERROR::Invalid input, please enter a numerical value \n";
         return;
      }
      Status status = imsSettingsMgr_->toggleVonr(slotId, enable,
         MyImsSettingsCallback::onResponseCallback);
      if (status == Status::SUCCESS) {
         std::cout << "Set IMS VoNR request sent successfully \n";
      } else {
         std::cout << "ERROR - Failed to send set IMS VoNR request, Status:"
                   << static_cast<int>(status) << "\n";
         Utils::printStatus(status);
      }
   } else {
      std::cout << "ERROR - ImsSettingsManger is null \n";
   }
}
