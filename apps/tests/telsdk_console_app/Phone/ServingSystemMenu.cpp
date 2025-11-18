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


#include <chrono>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include <telux/tel/PhoneFactory.hpp>

#include "MyServingSystemHandler.hpp"
#include "ServingSystemMenu.hpp"

#define INVALID -1
#define GSM_RAT 1
#define WCDMA_RAT 2
#define LTE_RAT 3
#define NR_SA_RAT 4
#define NR_NSA_RAT 5

ServingSystemMenu::ServingSystemMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

ServingSystemMenu::~ServingSystemMenu() {
   for (auto index = 0; index < servingSystemMgrs_.size(); index++) {
       servingSystemMgrs_[index]->deregisterListener(servingSystemListener_);
       servingSystemMgrs_[index] = nullptr;
   }
   if (servingSystemListener_){
      servingSystemListener_ = nullptr;
   }
}

bool ServingSystemMenu::init() {

   //  Get the PhoneFactory and ServingSystemManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom;
   auto phoneManager = phoneFactory.getPhoneManager([&](telux::common::ServiceStatus status) {
       prom.set_value(status);
   });
   servingSystemListener_ = std::make_shared<MyServingSystemListener>();
   if (!phoneManager) {
       std::cout << "ERROR - Failed to get Phone Manager \n";
       return false;
   }
   std::vector<int> phoneIds;
   telux::common::ServiceStatus phoneMgrStatus = phoneManager->getServiceStatus();
   if (phoneMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Phone Manager subsystem is not ready, Please wait \n";
   }
   phoneMgrStatus = prom.get_future().get();
   if (phoneMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Phone Manager subsystem is ready \n";
       telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
       if (status == telux::common::Status::SUCCESS) {
          for (auto index = 1; index <= phoneIds.size(); index++) {
             std::promise<telux::common::ServiceStatus> prom;
             // Get the serving system manager instances
             auto servingSystemMgr = phoneFactory.getServingSystemManager(
                index, [&](telux::common::ServiceStatus status) {
                   prom.set_value(status);
             });
             if (!servingSystemMgr) {
                std::cout << "ERROR - Failed to get Serving System manager instance \n";
                return false;
             }

             std::cout << "Waiting for Serving System Manager to be ready on slotId " << index
                 << "\n";
             telux::common::ServiceStatus servSysMgrStatus = prom.get_future().get();
             if (servSysMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                 std::cout << "Serving System subsystem is ready on slotId " << index << "\n";
                 servingSystemMgrs_.emplace_back(servingSystemMgr);
             } else {
                 std::cout << "ERROR - Unable to initialize Serving System subsystem on slotId "
                     << index << std::endl;
                 return false;
             }
          }
       }
       for (auto index = 0; index < servingSystemMgrs_.size(); index++) {
          auto status = servingSystemMgrs_[index]->registerListener(servingSystemListener_);
          if(status != telux::common::Status::SUCCESS) {
             std::cout << "Failed to registerListener for Serving system Manager" << "\n";
             return false;
          }
       }

       std::shared_ptr<ConsoleAppCommand> getRatModePreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "1", "Get_RAT_mode_preference", {},
             std::bind(&ServingSystemMenu::getRatModePreference, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> setRatModePreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "2", "Set_RAT_mode_preference", {},
             std::bind(&ServingSystemMenu::setRatModePreference, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getServiceDomainPreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "3", "Get_service_domain_preference", {},
             std::bind(&ServingSystemMenu::getServiceDomainPreference, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> setServiceDomainPreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "4", "Set_service_domain_preference", {},
             std::bind(&ServingSystemMenu::setServiceDomainPreference, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getSystemInfoCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "5", "Get_Serving_System_Information", {},
             std::bind(&ServingSystemMenu::getSystemInfo, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getDcStatusCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "6", "Get_NR_Dual_Connectivity_Status", {},
             std::bind(&ServingSystemMenu::getDualConnectivityStatus, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> reqNetworkTimeCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "7", "Request_Network_Info_Time", {},
             std::bind(&ServingSystemMenu::requestNetworkInfo, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> reqRFBandInfoCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "8", "Request_RF_Band_Info", {},
             std::bind(&ServingSystemMenu::requestRFBandInfo, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getRejectInfoCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "9", "Get_Network_Reject_Information", {},
             std::bind(&ServingSystemMenu::getNetworkRejectInfo, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getCallBarringInfoCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "10", "Get_Call_Barring_Information", {},
             std::bind(&ServingSystemMenu::getCallBarringInfo, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getSmsCapabilityCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "11", "Get_SMS_Capability", {},
             std::bind(&ServingSystemMenu::getSmsCapability, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getLteCsCapabilityCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "12", "Get_LTE_CS_Capability", {},
             std::bind(&ServingSystemMenu::getLteCsCapability, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> requestRFBandCapabilityCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "13", "Request_RF_Band_Capability", {},
             std::bind(&ServingSystemMenu::requestRFBandCapability, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> requestRFBandPrefCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "14", "Request_RF_Band_Preferences", {},
             std::bind(&ServingSystemMenu::requestRFBandPref, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> setRFBandPrefCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "15", "Set_RF_Band_Preferences", {},
             std::bind(&ServingSystemMenu::setRFBandPref, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> reqSib16NetworkTimeCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "16", "Request_LTE_SIB16_Network_Time_Info", {},
             std::bind(&ServingSystemMenu::requestLteSib16NetworkTimeInfo, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> reqNr5gRrcUtcTimeCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "17", "Request_NR5G_RRC_UTC_Time_Info", {},
             std::bind(&ServingSystemMenu::requestNr5gRrcUtcTimeInfo, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "18", "Select_sim_slot", {},
             std::bind(&ServingSystemMenu::selectSimSlot, this, std::placeholders::_1)));
       std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListNetworkSubMenu
          = {getRatModePreferenceCommand, setRatModePreferenceCommand,
             getServiceDomainPreferenceCommand, setServiceDomainPreferenceCommand,
             getSystemInfoCommand, getDcStatusCommand, reqNetworkTimeCommand, reqRFBandInfoCommand,
             getRejectInfoCommand, getCallBarringInfoCommand, getSmsCapabilityCommand,
             getLteCsCapabilityCommand, requestRFBandCapabilityCommand, requestRFBandPrefCommand,
             setRFBandPrefCommand, reqSib16NetworkTimeCommand, reqNr5gRrcUtcTimeCommand};

       if (servingSystemMgrs_.size() > 1) {
          commandsListNetworkSubMenu.emplace_back(selectSimSlotCommand);
       }

       addCommands(commandsListNetworkSubMenu);
       ConsoleApp::displayMenu();
   } else {
       std::cout << " PhoneManager is NULL, failed to initialize ServingSystemMenu" << std::endl;
       return false;
   }
   return true;
}

void ServingSystemMenu::getRatModePreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      auto ret = servingSystemMgr->requestRatPreference(
         MyRatPreferenceResponseCallback::ratPreferenceResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet RAT mode preference request sent successfully\n";
      } else {
         std::cout << "\nGet RAT mode preference request failed \n";
      }
   }
}

void ServingSystemMenu::setRatModePreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      char delimiter = '\n';
      std::string preference;
      telux::tel::RatPreference pref;
      std::vector<int> options;
      std::cout
         << "Available RAT mode preferences: \n"
            "(0 - CDMA_1X\n 1 - CDMA_EVDO\n 2 - GSM\n 3 - WCDMA\n 4 - LTE\n 5 - TDSCDMA\n" <<
            " 6 - NR5G_COMBINED\n 7 - NR5G_NSA\n 8 - NR5G_SA\n 9 - NB1_NTN) \n\n";
      std::cout
         << "Enter RAT mode preferences\n(For example: enter 2,4 to prefer GSM & LTE mode): ";
      std::getline(std::cin, preference, delimiter);

      std::stringstream ss(preference);
      int i = INVALID;
      while(ss >> i) {
         options.push_back(i);
         if(ss.peek() == ',' || ss.peek() == ' ')
            ss.ignore();
      }

      for(auto &opt : options) {
         if(opt >= 0 && opt <= 9) {
            try {
               pref.set(opt);
            } catch(const std::exception &e) {
               std::cout << "ERROR: invalid input, please enter numerical values " << opt
                         << std::endl;
            }
         } else {
            std::cout << "Preference should not be out of range" << std::endl;
         }
      }
      auto ret = servingSystemMgr->setRatPreference(
         pref, MyServingSystemResponsecallback::servingSystemResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nSet RAT mode preference request sent successfully\n";
      } else {
         std::cout << "\nSet RAT mode preference request failed \n";
      }
   }
}

void ServingSystemMenu::getServiceDomainPreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      auto ret = servingSystemMgr->requestServiceDomainPreference(
         MyServiceDomainPrefResponseCallback::serviceDomainPrefResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet service domain preference request sent successfully\n";
      } else {
         std::cout << "\nGet service domain preference request failed \n";
      }
   }
}

void ServingSystemMenu::setServiceDomainPreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      std::string serviceDomain;
      int opt = -1;
      std::cout << "Enter service domain preference: (0 - CS, 1 - PS, 2 - CS/PS): ";
      std::cin >> serviceDomain;
      if(!serviceDomain.empty()) {
         try {
            opt = std::stoi(serviceDomain);
         } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input " << opt << std::endl;
         }
      } else {
         std::cout << "Service domain should not be empty";
      }
      telux::tel::ServiceDomainPreference domainPref
         = static_cast<telux::tel::ServiceDomainPreference>(opt);
      auto ret = servingSystemMgr->setServiceDomainPreference(
         domainPref, MyServingSystemResponsecallback::servingSystemResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nSet service domain preference request sent successfully\n";
      } else {
         std::cout << "\nSet service domain preference request failed \n";
      }
   }
}

void ServingSystemMenu::getSystemInfo(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      telux::tel::ServingSystemInfo sysInfo;
      auto status = servingSystemMgr->getSystemInfo(sysInfo);
      if(status == telux::common::Status::SUCCESS) {
         std::cout << "\n getSystemInfo is successful"
            << "\n Serving RAT is " << MyServingSystemHelper::getRadioTechnology(sysInfo.rat)
            << "\n Service domain is"
            << MyServingSystemHelper::getServiceDomain(sysInfo.domain)
            << "\n Service state is " << MyServingSystemHelper::getServiceState(sysInfo.state)
            << std::endl;
      } else {
         std::cout << "\n getSystemInfo failed, status: " << static_cast<int>(status);
      }
   }
}

void ServingSystemMenu::selectSimSlot(std::vector<std::string> userInput) {
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

void ServingSystemMenu::getDualConnectivityStatus(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      telux::tel::DcStatus dcStatus = servingSystemMgr->getDcStatus();
      std::cout << "\nENDC Availability: \n"
               << MyServingSystemHelper::getEndcAvailability(dcStatus.endcAvailability);
      std::cout << "\nDCNR Restriction: \n"
               << MyServingSystemHelper::getDcnrRestriction(dcStatus.dcnrRestriction);
   }
}

void ServingSystemMenu::requestNetworkInfo(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      auto ret = servingSystemMgr->requestNetworkTime(
         NetworkTimeResponseCallback::networkTimeResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet network time request sent successfully\n";
      } else {
         std::cout << "\nGet network time request failed \n";
      }
   }
}


void ServingSystemMenu::requestRFBandInfo(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if (servingSystemMgr) {
      auto ret = servingSystemMgr->requestRFBandInfo(
         RFBandInfoResponseCallback::rfBandInfoResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet RF band info sent successfully\n";
      } else {
         std::cout << "\nGet RF band info failed \n";
      }
   }
}

void ServingSystemMenu::getNetworkRejectInfo(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      telux::tel::NetworkRejectInfo rejectInfo = {
         {telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN,
          telux::tel::ServiceDomain::UNKNOWN}, 0, "", ""};
      auto status = servingSystemMgr->getNetworkRejectInfo(rejectInfo);
      if(status == telux::common::Status::SUCCESS) {
         std::cout << "\n getNetworkRejectInfo is successful"
            << "\n RAT: "
            << MyServingSystemHelper::getRadioTechnology(rejectInfo.rejectSrvInfo.rat)
            << "\n Service Domain: "
            << MyServingSystemHelper::getServiceDomain(rejectInfo.rejectSrvInfo.domain)
            << "\n Reject cause: " << static_cast<int>(rejectInfo.rejectCause)
            << "\n MCC: " << rejectInfo.mcc << "\n MNC: " << rejectInfo.mnc
            << std::endl;
      } else {
         std::cout << "\n getNetworkRejectInfo failed, status: " << static_cast<int>(status);
      }
   }
}

void ServingSystemMenu::getCallBarringInfo(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      std::vector<telux::tel::CallBarringInfo> barringInfo;
      auto status = servingSystemMgr->getCallBarringInfo(barringInfo);
      if(status == telux::common::Status::SUCCESS) {
         std::cout << "\n getCallBarringInfo is successful"  << std::endl;
         for (auto info : barringInfo) {
            std::cout << " RAT: "
               << MyServingSystemHelper::getRadioTechnology(info.rat)
               << ", Service Domain: "
               << MyServingSystemHelper::getServiceDomain(info.domain)
               << ", Call type: "
               << MyServingSystemHelper::getCallBarringType(info.callType)
               << std::endl;
         }
      } else {
         std::cout << "\n getCallBarringInfo failed, status: " << static_cast<int>(status);
      }
   }
}

void ServingSystemMenu::getSmsCapability(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      telux::tel::SmsCapability smsCapability = {
          telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN,
          telux::tel::SmsDomain::UNKNOWN};
      auto status = servingSystemMgr->getSmsCapabilityOverNetwork(smsCapability);
      if(status == telux::common::Status::SUCCESS) {
         std::cout << "\n getSmsCapability is successful"
            << "\n RAT: "
            << MyServingSystemHelper::getRadioTechnology(smsCapability.rat)
            << ((smsCapability.rat != telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
                "\n SMS Domain: " : "")
            << ((smsCapability.rat != telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
                MyServingSystemHelper::getSmsDomain(smsCapability.domain) : "")
            << ((smsCapability.rat == telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
               "\n SMS Service status: " : "")
            << ((smsCapability.rat == telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN) ?
                MyServingSystemHelper::getNtnSmsStatus(smsCapability.smsStatus) : "")
            << std::endl;
      } else {
         std::cout << "\n getSmsCapability failed, status: " << static_cast<int>(status);
      }
   }
}

void ServingSystemMenu::getLteCsCapability(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      telux::tel::LteCsCapability lteCapability = telux::tel::LteCsCapability::UNKNOWN;
      auto status = servingSystemMgr->getLteCsCapability(lteCapability);
      if(status == telux::common::Status::SUCCESS) {
         std::cout << "\n getLteCsCapability is successful"
            << "\n LTE CS Capability: "
            << MyServingSystemHelper::getLteCsCapability(lteCapability)
            << std::endl;
      } else {
         std::cout << "\n getLteCsCapability failed, status: " << static_cast<int>(status);
      }
   }
}

void ServingSystemMenu::requestRFBandCapability(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if (servingSystemMgr) {
      auto ret = servingSystemMgr->requestRFBandCapability(
         RFBandCapabilityResponseCallback::rfBandCapabilityResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nRequest RF band capability sent successfully\n";
      } else {
         std::cout << "\nRequest RF band capability failed \n";
      }
   }
}

void ServingSystemMenu::requestRFBandPref(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if (servingSystemMgr) {
      auto ret = servingSystemMgr->requestRFBandPreferences(
         RFBandPrefResponseCallback::rfBandPrefResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nRequest RF band preferences sent successfully\n";
      } else {
         std::cout << "\nRequest RF band preferences failed \n";
      }
   }
}

void ServingSystemMenu::setRFBandPref(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if (servingSystemMgr) {
      std::string ratSelection = "";
      char delimiter = '\n';
      int ratType = 0;
      std::vector<telux::tel::GsmRFBand> gsmBands;
      std::vector<telux::tel::WcdmaRFBand> wcdmaBands;
      std::vector<telux::tel::LteRFBand> lteBands;
      std::vector<telux::tel::NrRFBand> saBands;
      std::vector<telux::tel::NrRFBand> nsaBands;

      std::cout <<"Available RATs for RF band preferences: \n"
          << "(1 - GSM\n 2 - WCDMA\n 3 - LTE\n 4 - NR5G_SA\n 5 - NR5G_NSA\n q - exit \n) \n";
      while(true) {
         std::cout << "\nSelect the RAT mode: ";
         std::getline(std::cin, ratSelection, delimiter);
         if (ratSelection.empty()) {
            std::cout << "RAT mode input is empty \n";
            return;
         }
         if (ratSelection == "q") {
            break;
         }
         try {
             ratType = std::stoi(ratSelection);
         } catch (const std::exception &e) {
             std::cout << "ERROR::Invalid input, please enter a numerical value \n";
             return;
         }

         std::string bandSelection = "";
         std::vector<int> options;
         std::stringstream ss(bandSelection);
         int opt = 0;
         int i = INVALID;
         switch (ratType){
             case GSM_RAT:
                 std::cout << "Enter GSM RF band preferences for RATs \n"
                     << "(1 - GSM_450\n 2 - GSM_480\n 3 - GSM_750\n 4 - GSM_850\n"
                     << " 5 - GSM_900_EXTENDED\n 6 - GSM_900_PRIMARY\n 7 - GSM_900_RAILWAYS\n"
                     << " 8 - GSM_1800\n 9 - GSM_1900\n"
                     << " For example: enter 1,3 to prefer GSM band 450 & band 750\n) \n";
                 std::getline(std::cin, bandSelection, delimiter);
                 if (bandSelection.empty()) {
                    std::cout << " RF bands selection is empty \n";
                    return;
                 }
                 ss << bandSelection;
                 while(ss >> i) {
                     options.push_back(i);
                     if(ss.peek() == ',' || ss.peek() == ' ')
                         ss.ignore();
                 }
                 for(auto &opt : options) {
                     if(opt >= 1 && opt <= 9) {
                         try {
                             gsmBands.push_back(static_cast<telux::tel::GsmRFBand>(opt));
                         } catch(const std::exception &e) {
                             std::cout << "ERROR: invalid input, please enter numerical values "
                                 << opt << std::endl;
                             return;
                         }
                     } else {
                         std::cout << "Preference of GSM should not be out of range \n";
                         return;
                     }
                 }
                 break;
             case WCDMA_RAT:
                 std::cout << "Enter WCDMA RF band preferences for RATs \n"
                     << "(1 - WCDMA_2100\n 2 - WCDMA_PCS_1900\n 3 - WCDMA_DCS_1800\n"
                     << " 4 - WCDMA_1700_US\n 5 - WCDMA_850\n 6 - WCDMA_800\n 7 - WCDMA_2600\n"
                     << " 8 - WCDMA_900\n 9 - WCDMA_1700_JAPAN\n 10 - WCDMA_1500_JAPAN\n"
                     << " 11 - WCDMA_850_JAPAN\n"
                     << " For example: enter 1,3 to prefer WCDMA band 2100 & band DCS_1800\n) \n";
                 std::getline(std::cin, bandSelection, delimiter);
                 if (bandSelection.empty()) {
                    std::cout << " RF bands selection is empty \n";
                    return;
                 }
                 ss << bandSelection;
                 while(ss >> i) {
                     options.push_back(i);
                     if(ss.peek() == ',' || ss.peek() == ' ')
                         ss.ignore();
                 }
                 for(auto &opt : options) {
                     if(opt >= 1 && opt <= 11) {
                         try {
                             wcdmaBands.push_back(static_cast<telux::tel::WcdmaRFBand>(opt));
                         } catch(const std::exception &e) {
                             std::cout << "ERROR: invalid input, please enter numerical values "
                                 << opt << std::endl;
                             return;
                         }
                     } else {
                         std::cout << "Preference of WCDMA should not be out of range \n";
                         return;
                     }
                 }
                 break;
             case LTE_RAT:
                 std::cout << "Enter LTE RF band preferences for RATs \n"
                     << "(For example: enter 1,3 to prefer LTE band 1 & band 3\n) \n";
                 std::getline(std::cin, bandSelection, delimiter);
                 if (bandSelection.empty()) {
                    std::cout << " RF bands selection is empty \n";
                    return;
                 }
                 ss << bandSelection;
                 while(ss >> i) {
                     options.push_back(i);
                     if(ss.peek() == ',' || ss.peek() == ' ')
                         ss.ignore();
                 }
                 for(auto &opt : options) {
                     if(opt >= 1 && opt <= 256) {
                         try {
                             lteBands.push_back(static_cast<telux::tel::LteRFBand>(opt));
                         } catch(const std::exception &e) {
                             std::cout << "ERROR: invalid input, please enter numerical values "
                                 << opt << std::endl;
                             return;
                         }
                     } else {
                         std::cout << "Preference of LTE should not be out of range \n";
                         return;
                     }
                 }
                 break;
             case NR_SA_RAT:
             case NR_NSA_RAT:
                 std::cout << "Enter NR RF band preferences for RATs \n"
                     << "(For example: enter 1,3 to prefer NR band 1 & band 3\n) \n";
                 std::getline(std::cin, bandSelection, delimiter);
                 if (bandSelection.empty()) {
                    std::cout << " RF bands selection is empty \n";
                    return;
                 }
                 ss << bandSelection;
                 while(ss >> i) {
                     options.push_back(i);
                     if(ss.peek() == ',' || ss.peek() == ' ')
                         ss.ignore();
                 }
                 for(auto &opt : options) {
                     if(opt >= 1 && opt <= 261) {
                         try {
                             if (ratType == 4) {
                                saBands.push_back(static_cast<telux::tel::NrRFBand>(opt));
                             } else {
                                nsaBands.push_back(static_cast<telux::tel::NrRFBand>(opt));
                             }
                         } catch(const std::exception &e) {
                             std::cout << "ERROR: invalid input, please enter numerical values "
                                 << opt << std::endl;
                             return;
                         }
                     } else {
                         std::cout << "Preference of NR should not be out of range \n";
                         return;
                     }
                 }
                 break;
             default:
                 std::cout << "Invalid configuration selection \n";
                 return;
         }
      }
      auto builder = std::make_shared<telux::tel::RFBandListBuilder>();
      telux::common::ErrorCode errCode = telux::common::ErrorCode::UNKNOWN;
      std::shared_ptr<telux::tel::IRFBandList> prefBands = builder->addGsmRFBands(gsmBands)
                                        .addWcdmaRFBands(wcdmaBands)
                                        .addLteRFBands(lteBands)
                                        .addNrRFBands(telux::tel::NrType::SA, saBands)
                                        .addNrRFBands(telux::tel::NrType::NSA, nsaBands)
                                        .build(errCode);
      if (errCode == telux::common::ErrorCode::SUCCESS) {
          auto ret = servingSystemMgr->setRFBandPreferences(
              prefBands, RFBandPrefResponseCallback::setRFBandPrefResponse);
          if(ret == telux::common::Status::SUCCESS) {
             std::cout << "\nSet RF band preferences sent successfully\n";
          } else {
             std::cout << "\nSet RF band preferences failed \n";
          }
      } else {
        std::cout << "\nBuild RF band preferences failed \n";
      }
   }
}

void ServingSystemMenu::requestLteSib16NetworkTimeInfo(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if (servingSystemMgr) {
      auto ret = servingSystemMgr->requestLteSib16NetworkTime(
         NetworkTimeResponseCallback::networkTimeResponse);
      if (ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet LTE SIB16 network time request sent successfully\n";
      } else {
         std::cout << "\nGet LTE SIB16 network time request failed \n";
      }
   } else {
      std::cout << "\nGet LTE SIB16 network time request failed \n";
   }
}

void ServingSystemMenu::requestNr5gRrcUtcTimeInfo(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if (servingSystemMgr) {
      auto ret = servingSystemMgr->requestNr5gRrcUtcTime(
         NetworkTimeResponseCallback::networkTimeResponse);
      if (ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet NR5G RRC UTC time request sent successfully\n";
      } else {
         std::cout << "\nGet NR5G RRC UTC time request failed \n";
      }
   } else {
      std::cout << "\nGet NR5G RRC UTC time request failed \n";
   }
}
