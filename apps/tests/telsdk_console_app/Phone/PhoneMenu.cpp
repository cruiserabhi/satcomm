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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * PhoneMenu provides menu options to invoke Phone functions such as
 * requestSignalStrength.
 */

#include <chrono>
#include <iostream>

#include "MyCellInfoHandler.hpp"
#include <telux/tel/PhoneFactory.hpp>
#include "../../common/utils/Utils.hpp"

#include "NetworkMenu.hpp"
#include "PhoneMenu.hpp"
#include "ServingSystemMenu.hpp"
#include "SuppServicesMenu.hpp"

#define INVALID -1
#define CONFIGURE_SIGNAL_STRENGTH_RAT_GSM 0
#define CONFIGURE_SIGNAL_STRENGTH_RAT_WCDMA 1
#define CONFIGURE_SIGNAL_STRENGTH_RAT_LTE 2
#define CONFIGURE_SIGNAL_STRENGTH_RAT_NR5G 3
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSSI 0
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_ECIO 1
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSCP 2
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSSI 0
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSRP 1
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSRQ 2
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_SNR  3
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_RSRP 0
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_RSRQ 1
#define CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_SNR 2

PhoneMenu::PhoneMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

PhoneMenu::~PhoneMenu() {
   if (phoneManager_ && phoneListener_) {
      phoneManager_->removeListener(phoneListener_);
   }
   if (phoneListener_) {
      phoneListener_ = nullptr;
   }
   if (subscriptionMgr_ && subscriptionListener_) {
      subscriptionMgr_->removeListener(subscriptionListener_);
   }
   if (subscriptionListener_) {
      subscriptionListener_ = nullptr;
   }
   mySignalStrengthCb_ = nullptr;
   myVoiceSrvStateCb_ = nullptr;
   myCellularCapabilityCb_ = nullptr;
   myGetOperatingModeCb_ = nullptr;
   mySetOperatingModeCb_ = nullptr;
   for (auto index = 0; index < phones_.size() ; index++) {
       phones_[index] = nullptr;
   }
   subscriptionMgr_ = nullptr;
   phoneManager_ = nullptr;
}

bool PhoneMenu::init() {
   std::promise<ServiceStatus> phoneMgrprom;
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   phoneManager_ = phoneFactory.getPhoneManager([&](ServiceStatus status) {
      phoneMgrprom.set_value(status);
   });
   if (!phoneManager_) {
      std::cout << "ERROR - Failed to get PhoneManager instance \n";
      return false;
   }
   ServiceStatus phoneMgrStatus = phoneManager_->getServiceStatus();
   if (phoneMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "PhoneManager subsystem is not ready, Please wait \n";
   }
   phoneMgrStatus = phoneMgrprom.get_future().get();
   if (phoneMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "Elapsed Time for Subsystem to ready : " << elapsedTime.count() << "s\n"
               << std::endl;
      std::cout << "PhoneManager subsystem is ready \n";
   } else {
      std::cout << "ERROR - Unable to initialize subsystem" << std::endl;
      return false;
   }

   if(phoneMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
      std::vector<int> phoneIds;
      telux::common::Status status = phoneManager_->getPhoneIds(phoneIds);
      if (status == telux::common::Status::SUCCESS) {
         for (auto index = 1; index <= phoneIds.size(); index++) {
            auto phone = phoneManager_->getPhone(index);
            if (phone != nullptr) {
               phones_.emplace_back(phone);
            }
         }
      }
   // Turn on the radio if it's not available
   for (auto index = 0; index < phones_.size(); index++) {
      if(phones_[index]->getRadioState() != telux::tel::RadioState::RADIO_STATE_ON) {
         phones_[index]->setRadioPower(true);
      }
   }

   phoneListener_ = std::make_shared<MyPhoneListener>();
   status = phoneManager_->registerListener(phoneListener_);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to registerListener" << std::endl;
   }
   } else {
      std::cout << "ERROR - Unable to initialize PhoneManager subsystem \n";
      return false;
   }
   std::promise<ServiceStatus> subscriptionMgrprom;
   subscriptionMgr_ = telux::tel::PhoneFactory::getInstance().getSubscriptionManager(
                    [&](ServiceStatus status) {
      subscriptionMgrprom.set_value(status);
   });
   if (!subscriptionMgr_) {
      std::cout << "ERROR - Failed to get SubscriptionManager instance \n";
      return false;
   }
   ServiceStatus subscriptionMgrStatus = subscriptionMgr_->getServiceStatus();
   if (subscriptionMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "SubscriptionManager subsystem is not ready, Please wait \n";
   }
   subscriptionMgrStatus = subscriptionMgrprom.get_future().get();
   if (subscriptionMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "SubscriptionManager subsystem is ready \n";
      subscriptionListener_ = std::make_shared<MySubscriptionListener>();
      telux::common::Status status = subscriptionMgr_->registerListener(subscriptionListener_);
      if(status != telux::common::Status::SUCCESS) {
         std::cout << "Failed to registerListener" << std::endl;
      }

      mySignalStrengthCb_ = std::make_shared<MySignalStrengthCallback>();
      myVoiceSrvStateCb_ = std::make_shared<MyVoiceServiceStateCallback>();
      myCellularCapabilityCb_ = std::make_shared<MyCellularCapabilityCallback>();
      myGetOperatingModeCb_ = std::make_shared<MyGetOperatingModeCallback>();
      mySetOperatingModeCb_ = std::make_shared<MySetOperatingModeCallback>();
   } else {
      std::cout << "ERROR - Unable to initialize SubscriptionManager subsystem \n";
      return false;
   }
   std::shared_ptr<ConsoleAppCommand> getSignalStrengthCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Get_signal_strength", {},
            std::bind(&PhoneMenu::requestSignalStrength, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestVoiceServiceStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Request_voice_service_state", {},
            std::bind(&PhoneMenu::requestVoiceServiceState, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestCellularCapabilitiesCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Request_cellular_capabilities", {},
            std::bind(&PhoneMenu::requestCellularCapabilities, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> getSubscriptionCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("4", "Get_subscription", {}, std::bind(&PhoneMenu::getSubscription, this,
         std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getOperatingModeCommand =
      std::make_shared<ConsoleAppCommand>( ConsoleAppCommand("5", "Get_operating_mode", {},
         std::bind(&PhoneMenu::getOperatingMode, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setOperatingModeCommand =
      std::make_shared<ConsoleAppCommand>( ConsoleAppCommand("6", "Set_operating_mode", {},
         std::bind(&PhoneMenu::setOperatingMode, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestCellInfoListCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "7", "Request_cell_info_list", {},
            std::bind(&PhoneMenu::requestCellInfoList, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setCellInfoListRateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "8", "Set_cell_info_list_rate", {},
            std::bind(&PhoneMenu::setCellInfoListRate, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> networkMenuCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("9", "Network_Selection", {},
            std::bind(&PhoneMenu::networkMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> servingSystemMenuCommand
      = std::make_shared<ConsoleAppCommand>(
         ConsoleAppCommand("10", "Serving_System", {},
            std::bind(&PhoneMenu::servingSystemMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setECallOperatingModeCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "11", "Set_eCall_operating_mode", {},
            std::bind(&PhoneMenu::setECallOperatingMode, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestECallOperatingModeCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "12", "Request_eCall_operating_mode", {},
            std::bind(&PhoneMenu::requestECallOperatingMode, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestOperatorNameCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13", "Get_operator_name", {},
         std::bind(&PhoneMenu::requestOperatorName, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> suppServicesMenuCommand =
      std::make_shared<ConsoleAppCommand>(
         ConsoleAppCommand("14", "Supp_Services_Menu", {},
            std::bind(&PhoneMenu::suppServicesMenu, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> resetWwanCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("15", "Reset_Wwan", {},
                        std::bind(&PhoneMenu::resetWwan, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureSignalStrengthCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "16", "Configure_Signal_Strength", {},
            std::bind(&PhoneMenu::configureSignalStrength, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureSignalStrengthExCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "17", "Configure_Signal_Strength_Ex", {},
            std::bind(&PhoneMenu::configureSignalStrengthEx, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("18", "Select_sim_slot", {},
                        std::bind(&PhoneMenu::selectSimSlot, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListPhoneSubMenu
      = {getSignalStrengthCommand,
         requestVoiceServiceStateCommand,
         requestCellularCapabilitiesCommand,
         getSubscriptionCommand,
         getOperatingModeCommand,
         setOperatingModeCommand,
         requestCellInfoListCommand,
         setCellInfoListRateCommand,
         networkMenuCommand,
         servingSystemMenuCommand,
         setECallOperatingModeCommand,
         requestECallOperatingModeCommand,
         requestOperatorNameCommand,
         suppServicesMenuCommand,
         resetWwanCommand,
         configureSignalStrengthCommand,
         configureSignalStrengthExCommand};

   if (phones_.size() > 1) {
      commandsListPhoneSubMenu.emplace_back(selectSimSlotCommand);
   }

   addCommands(commandsListPhoneSubMenu);
   ConsoleApp::displayMenu();
   return true;
}

void PhoneMenu::requestSignalStrength(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestSignalStrength(mySignalStrengthCb_);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Request Signal strength is successful \n"
                                                   : "Request Signal strength failed")
         << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

std::string PhoneMenu::getServiceStateAsString(telux::tel::ServiceState serviceState) {
   std::string serviceStateString = "";
   switch(serviceState) {
      case telux::tel::ServiceState::EMERGENCY_ONLY:
         serviceStateString = "Emergency Only";
         break;
      case telux::tel::ServiceState::IN_SERVICE:
         serviceStateString = "In Service";
         break;
      case telux::tel::ServiceState::OUT_OF_SERVICE:
         serviceStateString = "Out Of Service";
         break;
      case telux::tel::ServiceState::RADIO_OFF:
         serviceStateString = "Radio Off";
         break;
      default:
         break;
   }
   return serviceStateString;
}

void PhoneMenu::requestVoiceServiceState(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestVoiceServiceState(myVoiceSrvStateCb_);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Request Voice Service state is successful \n"
                                                   : "Request Voice Service state failed")
         << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

void PhoneMenu::getSubscription(std::vector<std::string> userInput) {

   telux::common::Status status;
   auto subscription = subscriptionMgr_->getSubscription(slot_, &status);
   if(subscription) {
      std::cout << "CarrierName : " << subscription->getCarrierName()
                << "\nPhoneNumber : " << subscription->getPhoneNumber()
                << "\nIccId : " << subscription->getIccId()
                << "\nMcc: " << subscription->getMobileCountryCode()
                << "\nMnc: " << subscription->getMobileNetworkCode()
                << "\nSlotId : " << subscription->getSlotId()
                << "\nImsi : " << subscription->getImsi()
                << "\nGID1 : " << subscription->getGID1()
                << "\nGID2 : " << subscription->getGID2() << std::endl;
   } else {
      std::cout << "Subscription is empty" << std::endl;
   }
}

void PhoneMenu::requestCellularCapabilities(std::vector<std::string> userInput) {
   if(phoneManager_) {
      auto ret = phoneManager_->requestCellularCapabilityInfo(myCellularCapabilityCb_);
      std::cout << (ret == telux::common::Status::SUCCESS
                       ? "Cellular capabilities request is successful \n"
                       : "Cellular capabilities request failed")
                << '\n';
   }
}
void PhoneMenu::getOperatingMode(std::vector<std::string> userInput) {
   if(phoneManager_) {
      auto ret = phoneManager_->requestOperatingMode(myGetOperatingModeCb_);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Get Operating mode request is successful \n"
                                                   : "Get Operating mode request failed")
         << '\n';
   }
}

void PhoneMenu::setOperatingMode(std::vector<std::string> userInput) {
   if(phoneManager_) {
      int operatingMode = INVALID;
      std::cout << "Enter Operating Mode (0-Online, 1-Airplane, 2-Factory Test,\n"
                << "3-Offline, 4-Resetting, 5-Shutting Down, 6-Persistent Low "
                   "Power) : ";
      std::cin >> operatingMode;
      Utils::validateInput(operatingMode);
      if(operatingMode >= 0 && operatingMode <= 6) {

         auto responseCb = std::bind(&MySetOperatingModeCallback::setOperatingModeResponse,
                                     mySetOperatingModeCb_, std::placeholders::_1);
         auto ret = phoneManager_->setOperatingMode(
            static_cast<telux::tel::OperatingMode>(operatingMode), responseCb);
         std::cout << (ret == telux::common::Status::SUCCESS
                          ? "Set Operating mode request is successful \n"
                          : "Set Operating mode request failed")
                   << '\n';
      } else {
         std::cout << " Invalid input " << std::endl;
      }
   }
}

void PhoneMenu::requestCellInfoList(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestCellInfo(MyCellInfoCallback::cellInfoListResponse);
      std::cout << (ret == telux::common::Status::SUCCESS ? "CellInfo list request is successful \n"
                                                          : "CellInfo list request failed")
                << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

void PhoneMenu::setCellInfoListRate(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      char delimiter = '\n';
      std::string timeIntervalInput;
      std::cout
         << "Enter time interval in Milliseconds(0 for default or notify when any changes): ";
      std::getline(std::cin, timeIntervalInput, delimiter);
      uint32_t opt = -1;
      if(!timeIntervalInput.empty()) {
         try {
            opt = std::stoi(timeIntervalInput);
         } catch(const std::exception &e) {
            std::cout << "ERROR: Invalid input, Enter numerical value " << opt << std::endl;
         }
      } else {
         opt = 0;
      }
      auto ret = phone->setCellInfoListRate(opt, MyCellInfoCallback::cellInfoListRateResponse);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Set cell info rate request is successful \n"
                                                   : "Set cell info rate request failed")
         << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

void PhoneMenu::servingSystemMenu(std::vector<std::string> userInput) {
   ServingSystemMenu servingSystemMenu("Serving System Menu", "ServingSystem> ");
   if (servingSystemMenu.init()) {
      servingSystemMenu.mainLoop();
   }
   ConsoleApp::displayMenu();
}

void PhoneMenu::networkMenu(std::vector<std::string> userInput) {
   NetworkMenu networkMenu("Network Menu", "Network> ");
   if (networkMenu.init()) {
      networkMenu.mainLoop();
   }
   ConsoleApp::displayMenu();
}

void PhoneMenu::setECallOperatingMode(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      int eCallMode = INVALID;
      std::cout << std::endl;
      std::cout << "Enter eCall Operating Mode(0-NORMAL, 1-ECALL_ONLY): ";
      std::cin >> eCallMode;
      Utils::validateInput(eCallMode);

      if(eCallMode == 0 || eCallMode == 1) {
         auto ret = phone->setECallOperatingMode(
            static_cast<telux::tel::ECallMode>(eCallMode),
            MySetECallOperatingModeCallback::setECallOperatingModeResponse);
         if(ret == telux::common::Status::SUCCESS) {
            std::cout << "Set eCall operating mode request sent successfully \n";
         } else {
            std::cout << "Set eCall operating mode request failed \n";
         }
      } else {
         std::cout << "Invalid input \n";
      }
   } else {
      std::cout << "No phone found corresponding to default phoneId" << std::endl;
   }
}

void PhoneMenu::requestECallOperatingMode(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestECallOperatingMode(
         MyGetECallOperatingModeCallback::getECallOperatingModeResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "Get eCall Operating mode request sent successfully\n";
      } else {
         std::cout << "Get eCall Operating mode request failed \n";
      }
   } else {
      std::cout << "No phone found corresponding to default phoneId" << std::endl;
   }
}

void PhoneMenu::selectSimSlot(std::vector<std::string> userInput) {
   std::string slotSelection;
   char delimiter = '\n';

   std::cout << "Enter the desired SIM slot (1-Primary, 2-Secondary): ";
   std::getline(std::cin, slotSelection, delimiter);

   if (!slotSelection.empty()) {
      try {
         int slot = std::stoi(slotSelection);
         if (slot > MAX_SLOT_ID || slot < DEFAULT_SLOT_ID) {
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

void PhoneMenu::requestOperatorName(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
        auto status = phone->requestOperatorInfo(MyOperatorInfoCallback::requestOperatorInfoCb);
        if (status == Status::SUCCESS) {
            std::cout << "Request Operator name sent successfully\n";
        } else {
            std::cout << "ERROR - Failed to request operator name,"
                      << "Status:" << static_cast<int>(status) << "\n";
            Utils::printStatus(status);
        }
   } else {
        std::cout << "No phone found\n";
   }
}

void PhoneMenu::suppServicesMenu(std::vector<std::string> userInput) {
   SuppServicesMenu suppServicesMenu("Supp Services Menu", "SuppServices> ");
   if (suppServicesMenu.init()) {
      suppServicesMenu.mainLoop();
   }
   ConsoleApp::displayMenu();
}

void PhoneMenu::resetWwan(std::vector<std::string> userInput) {
   if(phoneManager_) {
        auto status = phoneManager_->resetWwan(MyResetWwanCallback::resetWwanResponse);
        if (status == Status::SUCCESS) {
            std::cout << "Reset WWAN sent successfully\n";
        } else {
            std::cout << "ERROR - Failed to reset WWAN,"
                      << "Status:" << static_cast<int>(status) << "\n";
            Utils::printStatus(status);
        }
   } else {
        std::cout << "No phoneManager found\n";
   }
}

void PhoneMenu::configureSignalStrength(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if (phone) {
      std::string sigConfigType;
      uint32_t sigType;
      int opt = -1;
      int num = 0;
      int delta;
      int lower_threshold;
      int upper_threshold;
      std::vector<telux::tel::SignalStrengthConfig> sigStrengthConfigList = {};

      std::cout
         << "\nAvailable Signal Strength RAT Types are: \n"
         " 0 - GSM_RSSI\n 1 - WCDMA_RSSI\n 2 - LTE_RSSI\n 3 - LTE_SNR\n 4 - LTE_RSRQ\n" <<
         " 5 - LTE_RSRP\n 6 - NR5G_SNR\n 7 - NR5G_RSRP\n 8 - NR5G_RSRQ \n\n";
      std::cout << "Enter the number of Signal type(s) to be configured : ";
      std::cin >> num;
      Utils::validateInput(num);
      if (num > 0 && num <= (static_cast<int>(telux::tel::RadioSignalStrengthType::NR5G_RSRQ)+1)) {
                            // count is non-zero positive number i.e. enum last element+1
         for (int i = 0; i < num ; i++) {
            std::cout << "Enter Signal RAT Type : ";
            std::cin >> sigType;
            Utils::validateInput(sigType);
            if (sigType < static_cast<int>(telux::tel::RadioSignalStrengthType::GSM_RSSI) ||
                sigType > static_cast<int>(telux::tel::RadioSignalStrengthType::NR5G_RSRQ)) {
                std::cout << "Invalid input " << std::endl;
                return;
            }
            std::cout << "Enter Signal Strength Configuration (1-Delta, 2-Threshold ): ";
            std::cin >> sigConfigType;
            if (!sigConfigType.empty()) {
               try {
                  opt = std::stoi(sigConfigType);
               } catch(const std::exception &e) {
                  std::cout << "ERROR: Invalid input \n" << opt << std::endl;
                  return;
               }
            } else {
               std::cout << "Signal Strength configuration should not be empty \n";
               return;
            }
            if (opt >= static_cast<int>(telux::tel::SignalStrengthConfigType::DELTA) &&
               opt <= static_cast<int>(telux::tel::SignalStrengthConfigType::THRESHOLD)) {
               if (opt == static_cast<int>(telux::tel::SignalStrengthConfigType::DELTA)) {
                  std::cout << "Enter delta value : ";
                  std::cin >> delta;
                  Utils::validateInput(delta);
                  if (delta <= 0) {
                     std::cout << "Invalid input \n" << std::endl;
                     return;
                  }
                  telux::tel::SignalStrengthConfig sigStrengthConfig = {};
                  sigStrengthConfig.configType =
                       static_cast<telux::tel::SignalStrengthConfigType>(opt);
                  sigStrengthConfig.ratSigType =
                       static_cast<telux::tel::RadioSignalStrengthType>(sigType);
                  sigStrengthConfig.delta = delta;
                  sigStrengthConfigList.emplace_back(sigStrengthConfig);
               } else if (opt ==
                  static_cast<int>(telux::tel::SignalStrengthConfigType::THRESHOLD)) {
                  std::cout << "Enter lower threshold value : ";
                  std::cin >> lower_threshold;
                  Utils::validateInput(lower_threshold);
                  std::cout << "Enter upper threshold value : ";
                  std::cin >> upper_threshold;
                  Utils::validateInput(upper_threshold);

                  telux::tel::SignalStrengthConfig sigStrengthConfig = {};
                  sigStrengthConfig.configType =
                       static_cast<telux::tel::SignalStrengthConfigType>(opt);
                  sigStrengthConfig.ratSigType =
                      static_cast<telux::tel::RadioSignalStrengthType>(sigType);
                  sigStrengthConfig.threshold.lowerRangeThreshold = lower_threshold;
                  sigStrengthConfig.threshold.upperRangeThreshold = upper_threshold;
                  sigStrengthConfigList.emplace_back(sigStrengthConfig);
               }
            } else {
               std::cout << "Invalid input \n " << std::endl;
               return;
            }
         }
      } else {
         std::cout << "Invalid input, check the total available signal strength RAT types."
             << std::endl;
         return;
      }
      telux::common::Status status = phone->configureSignalStrength(sigStrengthConfigList,
         MyConfigureSignalStrengthCallback::configureSignalStrengthResponse);
      std::cout << (status == telux::common::Status::SUCCESS
                    ? "Configure Signal Strength request is successful. \n"
                    : "Configure Signal Strength request failed, check the input provided.")
                << '\n';
   } else {
      std::cout << "No phone found\n";
   }
}

void PhoneMenu::configureSignalStrengthEx(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if (phone) {
       char delimiter = '\n';
       std::string numString = "";
       int num = 0;
       std::string radioTechString = "";
       int radioTech = -1;
       std::string configPreference = "";
       std::string deltaString = "";
       uint16_t delta = 0;
       std::string thresholdString = "";
       std::string sigMeasNumString = "";
       int sigMeasNum = 0;
       std::string sigMeasTypeString = "";
       int sigMeasType = -1;
       std::string hysDeltaString = "";
       uint16_t hysDelta = 0;
       std::string hysOptionString = "";
       int hysOption = -1;
       std::string hysTimerString = "";
       uint16_t hysTimer = 0;

       std::vector<telux::tel::SignalStrengthConfigEx> sigStrengthConfigList = {};

       std::cout << "\nAvailable Signal Strength RAT are: \n"
           " 0 - GSM\n 1 - WCDMA\n 2 - LTE\n 3 - NR5G\n\n";
       std::cout << "Enter the number of Signal Strength Configs RAT(s) to be configured : ";
       std::getline(std::cin, numString, delimiter);
       try {
           num = stoi(numString);
       } catch (const std::exception &e) {
           std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
               << std::endl;
           return;
       }
       Utils::validateInput(num);
       if (num > CONFIGURE_SIGNAL_STRENGTH_RAT_GSM && num <= CONFIGURE_SIGNAL_STRENGTH_RAT_NR5G+1)
       {
           for (int idx = 0; idx < num ; idx++) {
               telux::tel::SignalStrengthConfigEx sigStrengthConfig = {};
               telux::tel::SignalStrengthConfigMask configMask = {};
               std::cout << "Enter RAT : ";
               std::getline(std::cin, radioTechString, delimiter);
               try {
                   radioTech = stoi(radioTechString);
               } catch (const std::exception &e) {
                   std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
                       << radioTech << std::endl;
                   return;
               }
               Utils::validateInput(radioTech);
               if (radioTech < CONFIGURE_SIGNAL_STRENGTH_RAT_GSM ||
                   radioTech > CONFIGURE_SIGNAL_STRENGTH_RAT_NR5G) {
                   std::cout << "Invalid input " << std::endl;
                   return;
               }
               std::cout << "Available Signal Strength Configurations : \n"
                   " 1 - Delta\n 2 - Threshold\n 3 - Hysteresis DB\n\n ";
               std::cout << "Enter configuration preferences"
                   "(For example: enter 2,3 to prefer threshold and hysteresis DB): ";
               std::getline(std::cin, configPreference, delimiter);

               if (configPreference.empty()) {
                   std::cout << "Signal Strength configuration should not be empty \n";
                   return;
               }
               std::vector<int> configOptions = {};
               std::stringstream ss(configPreference);
               int input = INVALID;
               while (ss >> input) {
                   configOptions.push_back(input);
                   if(ss.peek() == ',' || ss.peek() == ' ')
                       ss.ignore();
               }

               for (auto &opt : configOptions) {
                   if (opt >= static_cast<int>(telux::tel::SignalStrengthConfigExType::DELTA) &&
                       opt <= static_cast<int>
                           (telux::tel::SignalStrengthConfigExType::HYSTERESIS_DB)) {
                       try {
                           configMask.set(opt);
                       } catch(const std::exception &e) {
                           std::cout << "ERROR: invalid input, please enter numerical value "
                               << opt << std::endl;
                           return;
                       }
                   } else {
                       std::cout << "ConfigOptions should not be out of range" << std::endl;
                       return;
                   }
               }
               sigStrengthConfig.configTypeMask = configMask;
               switch (radioTech) {
                   case CONFIGURE_SIGNAL_STRENGTH_RAT_GSM:
                   {
                       sigStrengthConfig.radioTech = telux::tel::RadioTechnology::RADIO_TECH_GSM;
                       telux::tel::SignalStrengthConfigData sigData = {};
                       std::cout
                           <<"\nAvailable Signal Strength Measurement Types are: \n"
                           " 0 - RSSI\n";
                       sigData.sigMeasType = telux::tel::SignalStrengthMeasurementType::RSSI;
                       if (configMask.test(telux::tel::SignalStrengthConfigExType::DELTA)) {
                           std::cout << "Enter delta : ";
                           std::getline(std::cin, deltaString, delimiter);
                           try {
                               delta = stoi(deltaString);
                           } catch (const std::exception &e) {
                               std::cout
                                   << "ERROR: invalid input, please enter a numerical value."
                                   " INPUT: " << delta << std::endl;
                               return;
                           }
                           Utils::validateInput(delta);
                           if (delta <= 0) {
                               std::cout << "Invalid input \n" << std::endl;
                               return;
                           }
                           sigData.delta = delta;
                       } else if(configMask.test
                           (telux::tel::SignalStrengthConfigExType::THRESHOLD)) {
                           std::vector<int32_t> thresholdList = {};
                           std::cout << "Enter threshold list by comma separated :";
                           std::getline(std::cin, thresholdString, delimiter);
                           std::stringstream ss(thresholdString);
                           int32_t value = INVALID;
                           while(ss >> value) {
                               thresholdList.push_back(value);
                               if (ss.peek() == ',' || ss.peek() == ' ')
                                   ss.ignore();
                           }

                           for (int thIdx = 0; thIdx < thresholdList.size(); thIdx++) {
                               sigData.thresholdList[thIdx] = thresholdList[thIdx];
                           }
                       }
                       if (configMask.test
                           (telux::tel::SignalStrengthConfigExType::HYSTERESIS_DB)) {
                           std::cout << "Enter hysteresis db: ";
                           std::getline(std::cin, hysDeltaString, delimiter);
                           try {
                               hysDelta = stoi(hysDeltaString);
                           } catch (const std::exception &e) {
                               std::cout
                                   << "ERROR: invalid input, please enter a numerical value."
                                   " INPUT: " << hysDelta << std::endl;
                               return;
                           }
                           Utils::validateInput(hysDelta);
                           if (hysDelta < 0) {
                               std::cout << "Invalid input \n" << std::endl;
                               return;
                           }
                           sigData.hysteresisDb = hysDelta;
                       }
                       // add to list
                       sigStrengthConfig.sigConfigData.emplace_back(sigData);
                       sigStrengthConfigList.emplace_back(sigStrengthConfig);
                       break;
                   }
                   case CONFIGURE_SIGNAL_STRENGTH_RAT_WCDMA:
                   {
                       sigStrengthConfig.radioTech = telux::tel::RadioTechnology::RADIO_TECH_UMTS;
                       std::cout<<"\nAvailable Signal Strength Measurement Types are: \n"
                           " 0 - RSSI\n 1 - ECIO\n 2 - RSCP\n";
                       std::cout
                           << "Enter the number of Signal Strength Measurement type(s)"
                           " to be configured : ";
                       std::getline(std::cin, sigMeasNumString, delimiter);
                       try {
                           sigMeasNum = stoi(sigMeasNumString);
                       } catch (const std::exception &e) {
                           std::cout << "ERROR: invalid input, please enter a numerical value."
                               " INPUT: " << sigMeasNum << std::endl;
                           return;
                       }
                       Utils::validateInput(sigMeasNum);
                       if (sigMeasNum > CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSSI &&
                           sigMeasNum <= CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSCP+1) {
                           for (int sigIdx = 0; sigIdx < sigMeasNum; sigIdx++) {
                               telux::tel::SignalStrengthConfigData sigData = {};
                               std::vector<int32_t> thresholdList = {};
                               std::cout << "Enter signal measurement type : ";
                               std::getline(std::cin, sigMeasTypeString, delimiter);
                               try {
                                   sigMeasType = stoi(sigMeasTypeString);
                               } catch (const std::exception &e) {
                                   std::cout
                                       << "ERROR: invalid input, please enter a numerical value."
                                       " INPUT: " << sigMeasType << std::endl;
                                   return;
                               }
                               Utils::validateInput(sigMeasType);
                               if (sigMeasType < CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSSI ||
                                   sigMeasType > CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSCP) {
                                   std::cout << "Invalid input " << std::endl;
                                   return;
                               }
                               switch(sigMeasType) {
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSSI:
                                       sigData.sigMeasType =
                                            telux::tel::SignalStrengthMeasurementType::RSSI;
                                       break;
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_ECIO:
                                       sigData.sigMeasType =
                                            telux::tel::SignalStrengthMeasurementType::ECIO;
                                       break;
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_WCDMA_RSCP:
                                       sigData.sigMeasType =
                                           telux::tel::SignalStrengthMeasurementType::RSCP;
                                       break;
                                   default:
                                       break;
                               }
                               if (configMask.test
                                  (telux::tel::SignalStrengthConfigExType::DELTA)) {
                                   std::cout << "Enter delta : ";
                                   std::getline(std::cin, deltaString, delimiter);
                                   try {
                                       delta = stoi(deltaString);
                                   } catch (const std::exception &e) {
                                       std::cout
                                           << "ERROR: invalid input, please enter a numerical"
                                           " value. INPUT: " << delta << std::endl;
                                       return;
                                   }
                                   Utils::validateInput(delta);
                                   if (delta <= 0) {
                                       std::cout << "Invalid input \n" << std::endl;
                                       return;
                                   }
                                   sigData.delta = delta;
                               } else if (configMask.test
                                   (telux::tel::SignalStrengthConfigExType::THRESHOLD)) {
                                   std::cout << "Enter threshold list by comma separated :";
                                   std::getline(std::cin, thresholdString, delimiter);
                                   std::stringstream ss(thresholdString);
                                   int32_t value = INVALID;
                                   while(ss >> value) {
                                       thresholdList.push_back(value);
                                       if (ss.peek() == ',' || ss.peek() == ' ')
                                           ss.ignore();
                                   }

                                   for (int thIdx = 0; thIdx < thresholdList.size(); thIdx++) {
                                       sigData.thresholdList[thIdx] = thresholdList[thIdx];
                                   }
                               }
                               if (configMask.test
                                   (telux::tel::SignalStrengthConfigExType::HYSTERESIS_DB)) {
                                   std::cout << "Enter hysteresis db: ";
                                   std::getline(std::cin, hysDeltaString, delimiter);
                                   try {
                                       hysDelta = stoi(hysDeltaString);
                                   } catch (const std::exception &e) {
                                       std::cout << "ERROR: invalid input, please enter a"
                                           " numerical value. INPUT: " << hysDelta << std::endl;
                                       return;
                                   }
                                   Utils::validateInput(hysDelta);
                                   if (hysDelta < 0) {
                                       std::cout << "Invalid input \n" << std::endl;
                                       return;
                                   }
                                   sigData.hysteresisDb = hysDelta;
                               }
                               sigStrengthConfig.sigConfigData.emplace_back(sigData);
                           }
                       } else {
                           std::cout << "Invalid input, check the total available signal"
                               " strength measurement types." << std::endl;
                           return;
                       }
                       // add to list
                       sigStrengthConfigList.emplace_back(sigStrengthConfig);
                       break;
                   }
                   case CONFIGURE_SIGNAL_STRENGTH_RAT_LTE:
                   {
                       sigStrengthConfig.radioTech = telux::tel::RadioTechnology::RADIO_TECH_LTE;
                       std::cout<<"\nAvailable Signal Strength Measurement Types are: \n"
                           " 0 - RSSI\n 1 - RSRP\n 2 - RSRQ\n 3 - SNR\n";
                       std::cout
                           << "Enter the number of Signal Strength Measurement type(s)"
                           " to be configured : ";
                       std::getline(std::cin, sigMeasNumString, delimiter);
                       try {
                           sigMeasNum = stoi(sigMeasNumString);
                       } catch (const std::exception &e) {
                           std::cout << "ERROR: invalid input, please enter a numerical value."
                               " INPUT: " << sigMeasNum << std::endl;
                           return;
                       }
                       Utils::validateInput(sigMeasNum);
                       if (sigMeasNum > CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSSI &&
                           sigMeasNum <= CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_SNR+1) {
                           for (int sigIdx = 0; sigIdx < sigMeasNum; sigIdx++) {
                               telux::tel::SignalStrengthConfigData sigData = {};
                               std::vector<int32_t> thresholdList = {};
                               std::cout << "Enter signal measurement type : ";
                               std::getline(std::cin, sigMeasTypeString, delimiter);
                               try {
                                   sigMeasType = stoi(sigMeasTypeString);
                               } catch (const std::exception &e) {
                                   std::cout << "ERROR: invalid input, please enter a numerical"
                                       " value. INPUT: " << sigMeasType << std::endl;
                                   return;
                               }
                               Utils::validateInput(sigMeasType);
                               if (sigMeasType < CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSSI ||
                                   sigMeasType > CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_SNR) {
                                   std::cout << "Invalid input " << std::endl;
                                   return;
                               }
                               switch(sigMeasType) {
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSSI:
                                       sigData.sigMeasType =
                                           telux::tel::SignalStrengthMeasurementType::RSSI;
                                       break;
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSRP:
                                       sigData.sigMeasType =
                                           telux::tel::SignalStrengthMeasurementType::RSRP;
                                       break;
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_RSRQ:
                                       sigData.sigMeasType =
                                          telux::tel::SignalStrengthMeasurementType::RSRQ;
                                       break;
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_LTE_SNR:
                                       sigData.sigMeasType =
                                           telux::tel::SignalStrengthMeasurementType::SNR;
                                       break;
                                   default:
                                       break;
                               }
                               if (configMask.test
                                   (telux::tel::SignalStrengthConfigExType::DELTA)) {
                                   std::cout << "Enter delta : ";
                                   std::getline(std::cin, deltaString, delimiter);
                                   try {
                                       delta = stoi(deltaString);
                                   } catch (const std::exception &e) {
                                       std::cout << "ERROR: invalid input, please enter a"
                                           << " numerical value. INPUT: " << delta <<std::endl;
                                       return;
                                   }
                                   Utils::validateInput(delta);
                                   if (delta <= 0) {
                                       std::cout << "Invalid input \n" << std::endl;
                                       return;
                                   }
                                   sigData.delta = delta;
                               } else if(configMask.test
                                   (telux::tel::SignalStrengthConfigExType::THRESHOLD)) {
                                   std::cout << "Enter threshold list by comma separated :";
                                   std::getline(std::cin, thresholdString, delimiter);
                                   std::stringstream ss(thresholdString);
                                   int32_t value = INVALID;
                                   while(ss >> value) {
                                       thresholdList.push_back(value);
                                       if (ss.peek() == ',' || ss.peek() == ' ')
                                           ss.ignore();
                                   }

                                   for (int thIdx = 0; thIdx < thresholdList.size(); thIdx++) {
                                       sigData.thresholdList[thIdx] = thresholdList[thIdx];
                                   }
                               }
                               if (configMask.test
                                   (telux::tel::SignalStrengthConfigExType::HYSTERESIS_DB)) {
                                   std::cout << "Enter hysteresis db: ";
                                   std::getline(std::cin, hysDeltaString, delimiter);
                                   try {
                                       hysDelta = stoi(hysDeltaString);
                                   } catch (const std::exception &e) {
                                       std::cout << "ERROR: invalid input, please enter a"
                                           " numerical value. INPUT: " << hysDelta << std::endl;
                                       return;
                                   }
                                   Utils::validateInput(hysDelta);
                                   if (hysDelta < 0) {
                                       std::cout << "Invalid input \n" << std::endl;
                                       return;
                                   }
                                   sigData.hysteresisDb = hysDelta;
                               }
                               sigStrengthConfig.sigConfigData.emplace_back(sigData);
                           }
                       } else {
                           std::cout << "Invalid input, check the total available signal"
                               " strength measurement types." << std::endl;
                           return;
                       }
                       // add to list
                       sigStrengthConfigList.emplace_back(sigStrengthConfig);
                       break;
                   }
                   case CONFIGURE_SIGNAL_STRENGTH_RAT_NR5G:
                   {
                       sigStrengthConfig.radioTech = telux::tel::RadioTechnology::RADIO_TECH_NR5G;
                       std::cout<<"\nAvailable Signal Strength Measurement Types are: \n"
                           " 0 - RSRP\n 1 - RSRQ\n 2 - SNR\n";
                       std::cout
                           << "Enter the number of Signal Strength Measurement type(s)"
                           " to be configured : ";
                       std::getline(std::cin, sigMeasNumString, delimiter);
                       try {
                           sigMeasNum = stoi(sigMeasNumString);
                       } catch (const std::exception &e) {
                           std::cout << "ERROR: invalid input, please enter a numerical value."
                               " INPUT: " << sigMeasNum << std::endl;
                           return;
                       }
                       Utils::validateInput(sigMeasNum);
                       if (sigMeasNum > CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_RSRP &&
                           sigMeasNum <= CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_SNR+1) {
                           for (int sigIdx = 0; sigIdx < sigMeasNum; sigIdx++) {
                               telux::tel::SignalStrengthConfigData sigData = {};
                               std::vector<int32_t> thresholdList = {};
                               std::cout << "Enter signal measurement type : ";
                               std::getline(std::cin, sigMeasTypeString, delimiter);
                               try {
                                   sigMeasType = stoi(sigMeasTypeString);
                               } catch (const std::exception &e) {
                                   std::cout << "ERROR: invalid input, please enter a numerical"
                                       " value. INPUT: " << sigMeasType << std::endl;
                                   return;
                               }
                               Utils::validateInput(sigMeasType);
                               if (sigMeasType < CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_RSRP ||
                                   sigMeasType > CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_SNR) {
                                   std::cout << "Invalid input " << std::endl;
                                   return;
                               }
                               switch(sigMeasType) {
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_RSRP:
                                       sigData.sigMeasType =
                                           telux::tel::SignalStrengthMeasurementType::RSRP;
                                       break;
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_RSRQ:
                                       sigData.sigMeasType =
                                           telux::tel::SignalStrengthMeasurementType::RSRQ;
                                       break;
                                   case CONFIGURE_SIGNAL_STRENGTH_SIG_MEAS_NR5G_SNR:
                                       sigData.sigMeasType =
                                           telux::tel::SignalStrengthMeasurementType::SNR;
                                       break;
                                   default:
                                       break;
                               }
                               if (configMask.test
                                   (telux::tel::SignalStrengthConfigExType::DELTA)) {
                                   std::cout << "Enter delta : ";
                                   std::getline(std::cin, deltaString, delimiter);
                                   try {
                                       delta = stoi(deltaString);
                                   } catch (const std::exception &e) {
                                       std::cout << "ERROR: invalid input, please enter a"
                                           " numerical value. INPUT: " << delta << std::endl;
                                       return;
                                   }
                                   Utils::validateInput(delta);
                                   if (delta <= 0) {
                                       std::cout << "Invalid input \n" << std::endl;
                                       return;
                                   }
                                   sigData.delta = delta;
                               } else if(configMask.test
                                   (telux::tel::SignalStrengthConfigExType::THRESHOLD)) {
                                   std::cout << "Enter threshold list by comma separated :";
                                   std::getline(std::cin, thresholdString, delimiter);
                                   std::stringstream ss(thresholdString);
                                   int32_t value = INVALID;
                                   while(ss >> value) {
                                       thresholdList.push_back(value);
                                       if (ss.peek() == ',' || ss.peek() == ' ')
                                           ss.ignore();
                                   }

                                   for (int thIdx = 0; thIdx < thresholdList.size(); thIdx++) {
                                       sigData.thresholdList[thIdx] = thresholdList[thIdx];
                                   }
                               }
                               if (configMask.test
                                   (telux::tel::SignalStrengthConfigExType::HYSTERESIS_DB)) {
                                   std::cout << "Enter hysteresis db: ";
                                   std::getline(std::cin, hysDeltaString, delimiter);
                                   try {
                                       hysDelta = stoi(hysDeltaString);
                                   } catch (const std::exception &e) {
                                       std::cout << "ERROR: invalid input, please enter a"
                                           " numerical value. INPUT: " << hysDelta << std::endl;
                                       return;
                                   }
                                   Utils::validateInput(hysDelta);
                                   if (hysDelta < 0) {
                                       std::cout << "Invalid input \n" << std::endl;
                                       return;
                                   }
                                   sigData.hysteresisDb = hysDelta;
                               }
                               sigStrengthConfig.sigConfigData.emplace_back(sigData);
                           }
                       } else {
                           std::cout << "Invalid input, check the total available signal"
                               " strength measurement types." << std::endl;
                           return;
                       }
                       // add to list
                       sigStrengthConfigList.emplace_back(sigStrengthConfig);
                       break;
                   }
                   default:
                       break;
               }
           }
       } else {
           std::cout << "Invalid input, check the total available RATs." << std::endl;
           return;
       }
       // update hysteresis timer
       std::cout << "Configuration for hysteresis timer (0-No, 1-Yes) : ";
       std::getline(std::cin, hysOptionString, delimiter);
       try {
           hysOption = stoi(hysOptionString);
       } catch (const std::exception &e) {
           std::cout << "ERROR: invalid input, please enter a numerical value."
               " INPUT: " << hysOption << std::endl;
           return;
       }
       Utils::validateInput(hysOption, {1, 0});
       if (hysOption == 1) {
           std::cout << "Enter hysteresis timer(in milliseconds,"
               "a value of 0 disables the hysteresis timer): ";
           std::getline(std::cin, hysTimerString, delimiter);
           try {
               hysTimer = stoi(hysTimerString);
           } catch (const std::exception &e) {
               std::cout << "ERROR: invalid input, please enter a numerical value."
                   " INPUT: " << hysTimer << std::endl;
               return;
           }
           Utils::validateInput(hysTimer);
       }

       telux::common::Status status = phone->configureSignalStrength(sigStrengthConfigList,
           hysTimer, MyConfigureSignalStrengthCallback::configureSignalStrengthResponse);
       std::cout << (status == telux::common::Status::SUCCESS
           ? "Configure Signal Strength request is successful. \n"
           : "Configure Signal Strength request failed, check the input provided.")
           << '\n';
   } else {
      std::cout << "No phone found\n";
   }
}
