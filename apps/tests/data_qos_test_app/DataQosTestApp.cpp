/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <memory>
#include <cstdlib>
#include <csignal>

#include <telux/data/DataFactory.hpp>
#include "DataUtils.hpp"
#include "DataQosTestApp.hpp"

#include "../../common/utils/Utils.hpp"

using namespace telux::data;
using namespace telux::common;

/**
 * @file: DataQosTestApp.cpp
 *
 * @brief: Test application to demonstrate QOS TFT request and notifications
 */

#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_TCP_UDP 253

static bool listenerEnabled = false;
static std::mutex mutex;
static std::mutex listMutex;
static std::condition_variable cv;
static std::condition_variable requestCompleteCV;
static std::condition_variable listCV;

// Response callback for start or stop dataCall
void responseCallback(const std::shared_ptr<IDataCall> &dataCall, ErrorCode errorCode) {
   std::cout << "startCallResponse: errorCode: " << static_cast<int>(errorCode) << std::endl;
}

void DataQosTestApp::dataCallListResponseCb(
   const std::vector<std::shared_ptr<IDataCall>> &dataCallList, ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " ** Found "<<dataCallList.size()<<" DataCalls in the list **\n";
      for(auto dataCall:dataCallList) {
         std::cout << " ----------------------------------------------------------\n";
         std::cout << " ProfileID: " << dataCall->getProfileId()
            << "\n InterfaceName: " << dataCall->getInterfaceName()
            << "\n DataCallStatus: "
            << DataUtils::dataCallStatusToString(dataCall->getDataCallStatus())
            << "\n DataCallEndReason:\n   Type: "
            << DataUtils::callEndReasonTypeToString(dataCall->getDataCallEndReason().type)
            << ", Code: " << DataUtils::callEndReasonCode(dataCall->getDataCallEndReason())
            << std::endl;
            std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
            for(auto &it : ipAddrList) {
               std::cout << "\n ifAddress: " << it.ifAddress << "\n gwAddress: " << it.gwAddress
                  << "\n primaryDnsAddress: " << it.primaryDnsAddress
                  << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
            }
         std::cout << " IpFamilyType: "
            << DataUtils::ipFamilyTypeToString(dataCall->getIpFamilyType()) << '\n';
         std::cout << " TechPreference: "
            << DataUtils::techPreferenceToString(dataCall->getTechPreference()) << '\n';
         std::cout << " OperationType: "
            << DataUtils::operationTypeToString(dataCall->getOperationType()) << '\n';
         std::cout << " ----------------------------------------------------------\n\n";
      }
   } else {
      PRINT_CB << "requestDataCallList() failed,  errorCode: " << static_cast<int>(error)
         << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
   dataCallList_ = dataCallList;
   listCV.notify_all();
}

void DataQosTestApp::printFilterDetails(std::shared_ptr<telux::data::IIpFilter> filter) {

   IPv4Info ipv4Info_ = filter->getIPv4Info();
   if(!ipv4Info_.srcAddr.empty()) {
      std::cout << "\tIPv4 Src Address : " << ipv4Info_.srcAddr << std::endl;
   }
   if(!ipv4Info_.srcSubnetMask.empty()) {
      std::cout << "\tIPv4 Src Subnet Mask : " << ipv4Info_.srcSubnetMask << std::endl;
   }
   if(!ipv4Info_.destAddr.empty()) {
      std::cout << "\tIPv4 Dest Address : " << ipv4Info_.destAddr << std::endl;
   }
   if(!ipv4Info_.destSubnetMask.empty()) {
      std::cout << "\tIPv4 Dest Subnet Mask : " << ipv4Info_.destSubnetMask << std::endl;
   }
   if(ipv4Info_.value > 0) {
      std::cout << "\tIPv4 Type of service value : " << (int)ipv4Info_.value << std::endl;
   }
   if(ipv4Info_.mask > 0) {
      std::cout << "\tIPv4 Type of service mask : " << (int)ipv4Info_.mask << std::endl;
   }

   IPv6Info ipv6Info_ = filter->getIPv6Info();
   if(!ipv6Info_.srcAddr.empty()) {
      std::cout << "\tIPv6 Src Address : " << ipv6Info_.srcAddr << std::endl;
   }
   if(!ipv6Info_.destAddr.empty()) {
      std::cout << "\tIPv6 Dest Address : " << ipv6Info_.destAddr << std::endl;
   }
   if(ipv6Info_.val > 0) {
      std::cout << "\tIPv6 Traffic class value : " << (int)ipv6Info_.val << std::endl;
   }
   if(ipv6Info_.mask > 0) {
      std::cout << "\tIPv6 Traffic class mask : " << (int)ipv6Info_.mask << std::endl;
   }
   if(ipv6Info_.flowLabel > 0) {
      std::cout << "\tIPv6 Flow label : " << (int)ipv6Info_.flowLabel << std::endl;
   }

   IpProtocol proto = filter->getIpProtocol();
   switch (proto) {
      case PROTO_TCP: {
         auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(filter);
         if (tcpFilter) {
            TcpInfo portInfo_ = tcpFilter->getTcpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tTCP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tTCP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tTCP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tTCP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      case PROTO_UDP: {
         auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(filter);
         if (udpFilter) {
            UdpInfo portInfo_ = udpFilter->getUdpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tUDP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tUDP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tUDP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tUDP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      case PROTO_TCP_UDP: {
         auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(filter);
         if (tcpFilter) {
            TcpInfo portInfo_ = tcpFilter->getTcpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tTCP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tTCP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tTCP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tTCP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
         auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(filter);
         if (udpFilter) {
            UdpInfo portInfo_ = udpFilter->getUdpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tUDP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tUDP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tUDP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tUDP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      default: {
         std::cout << " Invalid XPort Protocol" <<std::endl;
      }
   }
}

void DataQosTestApp::logQosDetails(std::shared_ptr<TrafficFlowTemplate> &tft) {
   std::cout << " QoS Identifier : " << tft->qosId << std::endl;
   std::cout << " Profile Id : " << profileId_ << std::endl;

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_TX_GRANTED) &&
         (tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
            tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX))) {
      std::cout << " TX QOS FLow Granted: " << std::endl;

      if (tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS)) {
         std::cout << "\tIP FLow Traffic class: "
            << DataUtils::trafficClassToString(tft->txGrantedFlow.tfClass) << std::endl;

      }
      if (tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)) {
         std::cout << "\tMaximum required data rate (bits per second): "
            << tft->txGrantedFlow.dataRate.maxRate << std::endl;
         std::cout << "\tMinimum required data rate (bits per second): "
            << tft->txGrantedFlow.dataRate.minRate << std::endl;
      }
   }

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_RX_GRANTED) &&
         (tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
            tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX))) {
      std::cout << " RX QOS FLow Granted: " << std::endl;

      if (tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS)) {
         std::cout << "\tIP FLow Traffic class: "
            << DataUtils::trafficClassToString(tft->rxGrantedFlow.tfClass) << std::endl;

      }
      if (tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)) {
         std::cout << "\tMaximum required data rate (bits per second): "
            << tft->rxGrantedFlow.dataRate.maxRate << std::endl;
         std::cout << "\tMinimum required data rate (bits per second): "
            << tft->rxGrantedFlow.dataRate.minRate << std::endl;
      }
   }

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_TX_FILTERS)) {
      for (uint32_t i = 0 ; i < tft->txFiltersLength ; i++) {
         for (auto filter:tft->txFilters[i].filter) {
            IpProtocol proto = filter->getIpProtocol();
            std::string protocol = "TCP";
            if (PROTO_UDP == proto) {
               protocol = "UDP";
            }
            std::cout <<  " " << protocol << " TX Filter: " << (i+1) << std::endl;
            std::cout << "\tFilter ID: " << tft->txFilters[i].filterId << std::endl;
            std::cout << "\tFilter Precedence: " << tft->txFilters[i].filterPrecedence << std::endl;
            if (filter) {
               std::cout << "\tIP Family: "
                  << DataUtils::ipFamilyTypeToString(filter->getIpFamily()) << std::endl;
               printFilterDetails(filter);
            }
         }
      }
   }

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_RX_FILTERS)) {
      for (uint32_t i = 0 ; i < tft->rxFiltersLength ; i++) {
         for (auto filter:tft->rxFilters[i].filter) {
            IpProtocol proto = filter->getIpProtocol();
            std::string protocol = "TCP";
            if (PROTO_UDP == proto) {
               protocol = "UDP";
            }
            std::cout << " " << protocol << " RX Filter: " << (i+1) << std::endl;
            std::cout << "\tFilter ID: " << tft->rxFilters[i].filterId << std::endl;
            std::cout << "\tFilter Precedence: " << tft->rxFilters[i].filterPrecedence << std::endl;
            if (filter) {
               std::cout << "\tIP Family: "
                  << DataUtils::ipFamilyTypeToString(filter->getIpFamily()) << std::endl;
               printFilterDetails(filter);
            }
         }
      }
   }
}

// Implementation of IDataConnectionListener
void DataQosTestApp::onTrafficFlowTemplateChange(const std::shared_ptr<IDataCall> &dataCall,
   const std::vector<std::shared_ptr<TftChangeInfo>> &tfts) {

   // retrive profileid and ipfamily
   profileId_ = dataCall->getProfileId();
   std::cout << "\n onTrafficFlowTemplateChange" << std::endl;

   for (auto tft:tfts) {
      std::cout << " ----------------------------------------------------------\n";
      std::cout << " ** TFT Details **\n";
      std::cout << " Flow State: "
         << DataUtils::flowStateEventToString(tft->stateChange) << std::endl;
      logQosDetails(tft->tft);
      std::cout << " ----------------------------------------------------------\n\n";
   }
}

void DataQosTestApp::onTFTResponse(const std::vector<std::shared_ptr<TrafficFlowTemplate>> &tfts,
   telux::common::ErrorCode error) {
   std::cout << "\n onTFTResponse" << std::endl;

   if (error == telux::common::ErrorCode::SUCCESS) {
      for (auto tft:tfts) {
         std::cout << " ----------------------------------------------------------\n";
         std::cout << " ** TFT Details **\n";
         std::cout << " Flow State: "
            << DataUtils::flowStateEventToString(QosFlowStateChangeEvent::ACTIVATED) << std::endl;
         logQosDetails(tft);
         std::cout << " ----------------------------------------------------------\n\n";
      }
   } else {
      std::cout << "ErrorCode: " << static_cast<int>(error)
         << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
   requestCompleteCV.notify_all();
}

void DataQosTestApp::consoleinit() {
   std::shared_ptr<ConsoleAppCommand> getTftCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "1", "Get_tft", {},
         std::bind(&DataQosTestApp::getTft, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListQosMenu = {getTftCommand};
   ConsoleApp::addCommands(commandsListQosMenu);
   ConsoleApp::displayMenu();
}

void DataQosTestApp::getTft(std::vector<std::string> inputCommand) {
   std::cout << "\nGet tft " << std::endl;
   telux::common::Status retStat;

   if (dataConnMgr_) {
      // requesting local data calls and present list to user.
      telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
      retStat = dataConnMgr_->requestDataCallList(opType,
      std::bind(&DataQosTestApp::dataCallListResponseCb, this,
                     std::placeholders::_1, std::placeholders::_2));
      Utils::printStatus(retStat);
   }

   // wait for data call list to be displayed
   {
      std::unique_lock<std::mutex> lock(listMutex);
      listCV.wait(lock);
   }

   if (dataCallList_.size() > 0 ) {
      int profileId;
      std::cout << "Enter Profile Id : ";
      std::cin >> profileId;
      Utils::validateInput(profileId);
      profileId_ = profileId;

      int ipFamilyType;
      std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
      std::cin >> ipFamilyType;
      Utils::validateInput(ipFamilyType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
          static_cast<int>(telux::data::IpFamilyType::IPV6),
          static_cast<int>(telux::data::IpFamilyType::IPV4V6)});
      telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);

      bool dataCallFound = false;
      for(auto dataCall:dataCallList_) {
         if (dataCall->getProfileId() == profileId) {
            retStat = dataCall->requestTrafficFlowTemplate(ipFamType,
                  std::bind(&DataQosTestApp::onTFTResponse, this,
                        std::placeholders::_1, std::placeholders::_2));
            Utils::printStatus(retStat);
            dataCallFound = true;
            std::cout << std::endl;
            break;
         }
      }
      if (!dataCallFound) {
         std::cout << "Cannot find data call with profile id " << profileId <<
         " to request TFT" << std::endl;
      }
   } else {
      std::cout << "No data call up in system to request TFT\n";
   }
}

void DataQosTestApp::registerForUpdates() {
   // Registering a listener for Data connection notification
   telux::common::Status status = dataConnMgr_->registerListener(shared_from_this());
   if(status != telux::common::Status::SUCCESS) {
      std::cout << APP_NAME << " ERROR - Failed to register for data connection notification"
         << std::endl;
   } else {
      std::cout << APP_NAME << " Registered Listener for data connection notification" << std::endl;
   }
}

void DataQosTestApp::deregisterForUpdates() {
   // De-registering a listener for data connection notification
   telux::common::Status status = dataConnMgr_->deregisterListener(shared_from_this());
   if(status != telux::common::Status::SUCCESS) {
      std::cout << APP_NAME << " ERROR - Failed to de-register for data connection notification"
         << std::endl;
   } else {
      std::cout << APP_NAME << " De-registered listener" << std::endl;
   }
}

std::shared_ptr<DataQosTestApp> init() {
   std::shared_ptr<DataQosTestApp> dataQosApp = std::make_shared<DataQosTestApp>();
   if (!dataQosApp) {
      std::cout << "Failed to instantiate DataQosTestApp" << std::endl;
      return nullptr;
   }

   // Get the DataFactory
   auto &dataFactory = telux::data::DataFactory::getInstance();
   dataQosApp->dataConnMgr_ = dataFactory.getDataConnectionManager();

   // Check if data subsystem is ready
   bool subSystemStatus = dataQosApp->dataConnMgr_->isSubsystemReady();

   // If data subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "DATA subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = dataQosApp->dataConnMgr_->onSubsystemReady();
      // If we want to wait unconditionally for data subsystem to be ready
      subSystemStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize data subsystems
   if(subSystemStatus) {
      std::cout << " *** DATA Sub System is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize data subsystem *** " << std::endl;
      return nullptr;
   }

   return dataQosApp;
}

static void signalHandler( int signum ) {
   std::unique_lock<std::mutex> lock(mutex);
   std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
   cv.notify_all();
}

static void printHelp() {
    std::cout << "-----------------------------------------------" << std::endl;
    std::cout << "./data_qos_test_app <-l> <-c> <-h>" << std::endl;
    std::cout << "   -l : listen to QOS TFT flow notifications" << std::endl;
    std::cout << "   -c : open interactive console" << std::endl;
    std::cout << "   -r : request for tft" << std::endl;
    std::cout << "   -h : print the help menu" << std::endl;
}

DataQosTestApp::DataQosTestApp()
    : ConsoleApp("Data Qos TFT Menu", "qos-test> ")
    , dataConnMgr_(nullptr)
    , profileId_(PROFILE_ID_MAX) {
}

DataQosTestApp::~DataQosTestApp() {
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {

   if(argc <= 1) {
      printHelp();
      return -1;
   }

   std::cout
       << "\n#################################################\n"
       << "Warning! This test application will be deprecated and no longer get "
          "updates.\nIts functionality will be moved into telsdk_console_app "
          "under Data - Data_Connection_Management_Menu.\n\n"
       << " Limitations of current app include \n"
       << "  * Can be used only from PVM and cannot be used from SVM\n"
       << "  * If dual sim is enabled does not support opertaion on second slot\n"
       << "#################################################\n" << std::endl;
   std::vector<std::string> supplementaryGrps{"system", "logd"};
   int rc = Utils::setSupplementaryGroups(supplementaryGrps);
   if (rc == -1) {
      std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
   }

   for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "-l") {
         listenerEnabled =true;
      } else if (std::string(argv[i]) == "-c") {
         std::shared_ptr<DataQosTestApp> myDataQosTestApp = init();
         myDataQosTestApp->registerForUpdates();
         listenerEnabled =true;
         myDataQosTestApp->consoleinit();
         myDataQosTestApp->mainLoop();
         myDataQosTestApp->deregisterForUpdates();
         return 0;
      } else if (std::string(argv[i]) == "-r") {
         std::shared_ptr<DataQosTestApp> myDataQosTestApp = init();
         std::unique_lock<std::mutex> lock(mutex);
         myDataQosTestApp->registerForUpdates();
         listenerEnabled =true;
         myDataQosTestApp->getTft(std::vector<std::string>());
         requestCompleteCV.wait(lock);
         myDataQosTestApp->deregisterForUpdates();
         return 0;
      } else {
         printHelp();
         return -1;
      }
   }

   std::shared_ptr<DataQosTestApp> myDataQosTestApp = init();
   if(listenerEnabled) {
      myDataQosTestApp->registerForUpdates();
   }
   signal(SIGINT, signalHandler);
   std::unique_lock<std::mutex> lock(mutex);

   std::cout << APP_NAME << " Press CTRL+C to exit" << std::endl;
   cv.wait(lock);
   if(listenerEnabled) {
      myDataQosTestApp->deregisterForUpdates();
   }

   std::cout << "Exiting application..." << std::endl;
   return 0;
}
