/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <memory>
#include <cstdlib>
#include <sstream>
#include <csignal>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#include "DataQoSApp.hpp"

/**
 * @file: DataQoSApp.cpp
 *
 * @brief: Sample application to create traffic class and add QoS Entry.
 *
 * Example use cases as per different data paths in system:
 * 1. VLAN-based downlink traffic, tethered to apps software path
 * 2. VLAN-based uplink traffic, tethered to the apps software path
 * 3.  i. IPv4-based downlink traffic, tethered to the WAN hardware accelerated path
 *    ii. VLAN-based downlink traffic, tethered to the WAN hardware accelerated path
 * 4. IPv4-based uplink traffic, tethered to the WAN hardware accelerated path
 * 5. IPv4-based uplink traffic, from apps to the WAN path
 *
 *
 * For usage, use qos_sample_app -h
 * The app can be run in console mode using qos_sample_app -c
 * Other possible options are to:
 *       - Configure VLANs
 *       - Clear VLAN configurations
 *       - List use case
 *       - Run use case
 *       - Delete all QoS TC and filter configurations
 *       - Show all configurations
 */

std::shared_ptr<DataQoSApp> DataQoSApp::getInstance() {
   static std::shared_ptr<DataQoSApp> instance = std::make_shared<DataQoSApp>();
   return instance;
}

DataQoSApp::DataQoSApp()
    : ConsoleApp("Data QoS App Menu", "data-qos-app> ") {

   if(!dataQoSManager_) {
      std::promise<telux::common::ServiceStatus> qosProm;

      // QoS Manager instance
      dataQoSManager_  = telux::data::DataFactory::getInstance().getQoSManager([&qosProm]
         (telux::common::ServiceStatus status) {
            qosProm.set_value(status);
         });

      if (!dataQoSManager_) {
         std::cout << " Failed to get DataQoSManager object" << std::endl;
      }

      telux::common::ServiceStatus subSystemStatus = qosProm.get_future().get();
      if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << " *** QoS manager is Ready *** " << std::endl;
      } else {
         std::cout << " *** Unable to initialize QoS subsystem *** " << std::endl;
      }
   }
}

DataQoSApp::~DataQoSApp() {
}

void DataQoSApp::onDataCallInfoChanged(
   const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   std::cout << "\n onDataCallInfoChanged";
   logDataCallDetails(dataCall);
}

void DataQoSApp::logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   std::cout << " ** DataCall Details **\n";
   std::cout << " SlotID: " << dataCall->getSlotId() << std::endl;
   std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
   std::cout << " interfaceName: " << dataCall->getInterfaceName() << std::endl;
   std::cout << " DataCallStatus: " << (int)dataCall->getDataCallStatus() << std::endl;
   std::cout
      << " DataCallEndReason: Type = " << static_cast<int>(dataCall->getDataCallEndReason().type)
      << std::endl;
   std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
   if(dataCall->getDataCallStatus() == telux::data::DataCallStatus::NET_CONNECTED) {
      if((dataCall->getSlotId() == slotId_) && (dataCall->getProfileId() == profileId_)) {
         std::cout << " ** Active data call on default profile **\n";
         std::lock_guard<std::mutex> lock(dataCallMtx_);
         rmnetIp_ = ipAddrList.begin()->ifAddress;
         dataCallCv_.notify_all();
      }
      for(auto &it : ipAddrList) {
         std::cout << "\n ifAddress: " << it.ifAddress
                  << "\n primaryDnsAddress: " << it.primaryDnsAddress
                  << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
      }
      std::cout << "IpFamilyType: " << static_cast<int>(dataCall->getIpFamilyType()) << '\n';
      std::cout << "TechPreference: " << static_cast<int>(dataCall->getTechPreference()) << '\n';
   }
}

// initialize data connection mananger
bool DataQoSApp::initDataConnectionManager() {
   std::promise<telux::common::ServiceStatus> dcmProm;
   SlotId slotId = DEFAULT_SLOT_ID;
   if(!dataConnectionManager_) {
      // data connection mananger
      dataConnectionManager_ =
         telux::data::DataFactory::getInstance().getDataConnectionManager(slotId,
                                                [&dcmProm](telux::common::ServiceStatus status)
                                                { dcmProm.set_value(status); });

      if (!dataConnectionManager_) {
         std::cout << " Failed to get DataConnectionManager object" << std::endl;
         return false;
      }

      //wait for connection manager to get ready
      std::cout << " Initializing Data connection manager subsystem Please wait" << std::endl;
      telux::common::ServiceStatus subSystemStatus = dcmProm.get_future().get();

      if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << " *** Data Connection Manager is ready *** " << std::endl;
         std::shared_ptr<telux::data::IDataConnectionListener> dcmListener =  shared_from_this();
         std::cout << " *** Data Connection Manager is ready 22 *** " << std::endl;
         telux::common::Status status =
            dataConnectionManager_->registerListener(dcmListener);

         if (status != telux::common::Status::SUCCESS) {
            std::cout << " Unable to register data connection manager listener" << std::endl;
            return false;
         }
      } else {
         std::cout << " Data Connection Manager is failed" << std::endl;
         dataConnectionManager_ = nullptr;
         return false;
      }
   }
   return true;
}

// initialize VLAN manager
bool DataQoSApp::initVlanManager() {
   if(!vlanManager_) {
      std::promise<telux::common::ServiceStatus> vlanProm;
      // VLAN Manager instance
      vlanManager_  = telux::data::DataFactory::getInstance().getVlanManager(
         telux::data::OperationType::DATA_LOCAL,
         [&vlanProm](telux::common::ServiceStatus status) {
            vlanProm.set_value(status);
         });

      if (!vlanManager_) {
         std::cout <<  " Failed to get VLAN Manager object" << std::endl;;
         return false;
      }

      telux::common::ServiceStatus subSystemStatus = vlanProm.get_future().get();
      if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << " *** VLAN manager is Ready *** " << std::endl;
      } else {
         std::cout << " *** Unable to initialize VLAN subsystem *** " << std::endl;
         return false;
      }
   }
   return true;
}

// load default sim and profile
bool DataQoSApp::loadDefaultProfile() {
   if(initDataConnectionManager()) {
      std::promise<telux::common::ErrorCode> defaultProfileProm;
      telux::common::Status status = dataConnectionManager_->getDefaultProfile(
         telux::data::OperationType::DATA_LOCAL, [&defaultProfileProm, this]
            (int profileId, SlotId slotId, telux::common::ErrorCode error) {
            profileId_ = profileId;
            slotId_ = slotId;
            defaultProfileProm.set_value(error);
         });
      if ((status == telux::common::Status::SUCCESS) &&
            (defaultProfileProm.get_future().get() == telux::common::ErrorCode::SUCCESS)) {
         return true;
      }
   }
   return false;
}

// Start and wait for data call
bool DataQoSApp::startAndWaitForDataCall() {
   if(initDataConnectionManager()) {
      telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;
      if(loadDefaultProfile()) {
         telux::common::Status status =
            dataConnectionManager_->startDataCall(profileId_, ipFamilyType,
            [this](const std::shared_ptr<telux::data::IDataCall> &dataCall,
               telux::common::ErrorCode errorCode) {
                  std::cout << "startCallResponse: errorCode: "
                  << static_cast<int>(errorCode) << std::endl;
               logDataCallDetails(dataCall);
            }, telux::data::OperationType::DATA_LOCAL);
         if(status == telux::common::Status::SUCCESS) {
            std::unique_lock<std::mutex> lck(dataCallMtx_);
            dataCallCv_.wait(lck, [&]{return !rmnetIp_.empty();});
            return true;
         }
      }
   }
   if(rmnetIp_.empty())
      return false;
   else
      return true;
}

// Create and wait for VLAN
bool DataQoSApp::createAndWaitForVlan(int vlanId, bool isAccelerated, int pcp) {
   if(initVlanManager()) {
      std::promise<telux::common::ErrorCode> vlanProm;
      auto respCb = [&vlanProm](bool isAccelerated, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                  << "createVlan Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Acceleration "
                  << (isAccelerated ? "is allowed" : "is not allowed") << "\n";
         }
         vlanProm.set_value(error);
      };

      telux::data::VlanConfig config;
      config.iface = telux::data::InterfaceType::ETH;
      config.vlanId = vlanId;
      config.priority = pcp;
      config.isAccelerated = isAccelerated;

      telux::common::Status status = vlanManager_->createVlan(config, respCb);
      if ((status == telux::common::Status::SUCCESS) &&
            (vlanProm.get_future().get() == telux::common::ErrorCode::SUCCESS)) {
         return true;
      }
   }
   return false;
}

// Bind VLAN to backhaul
bool DataQoSApp::bindVlanToBackhaul(int vlanId) {
   if(loadDefaultProfile() && initVlanManager()) {
      std::promise<telux::common::ErrorCode> vlanProm;
      telux::data::net::VlanBindConfig vlanBindConfig = {};
      vlanBindConfig.bhInfo.backhaul = telux::data::BackhaulType::WWAN;
      vlanBindConfig.bhInfo.profileId = profileId_;
      vlanBindConfig.bhInfo.slotId = slotId_;
      vlanBindConfig.vlanId = vlanId;

      auto respCb = [&vlanProm](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                  << "bindToBackhaul Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         vlanProm.set_value(error);
      };

      telux::common::Status status = vlanManager_->bindToBackhaul(vlanBindConfig, respCb);
      if ((status == telux::common::Status::SUCCESS) &&
            (vlanProm.get_future().get() == telux::common::ErrorCode::SUCCESS)) {
         return true;
      }
   }
   return false;
}

// Stop data call
void DataQoSApp::stopDataCall() {
   if(dataConnectionManager_) {
      std::promise<telux::common::ErrorCode> conProm;
      dataConnectionManager_->stopDataCall(profileId_, telux::data::IpFamilyType::IPV4,
         [&conProm](const std::shared_ptr<telux::data::IDataCall> &dataCall,
            telux::common::ErrorCode errorCode) {
               std::cout << "stopCallResponse: errorCode: "
               << static_cast<int>(errorCode) << std::endl;
               conProm.set_value(errorCode);
         }, telux::data::OperationType::DATA_LOCAL);
      conProm.get_future().get();
   }
}

// Remove VLAN
void DataQoSApp::removeVlan(int vlanId) {
   if(initVlanManager()) {
      std::promise<telux::common::ErrorCode> vlanProm;
      telux::common::Status status = vlanManager_->removeVlan(vlanId,
      telux::data::InterfaceType::ETH, [&vlanProm](telux::common::ErrorCode errorCode) {
               std::cout << "removeVlan: errorCode: "
               << static_cast<int>(errorCode) << std::endl;
               vlanProm.set_value(errorCode);
         });
      vlanProm.get_future().get();
   }
}

// Create uplink traffic class
bool DataQoSApp::createUplinkTrafficClass(int trafficClass, telux::data::net::DataPath dataPath) {
   telux::data::net::TcConfigBuilder tcConfigBuilder;
   tcConfigBuilder.setTrafficClass(trafficClass).
      setDirection(telux::data::Direction::UPLINK).
      setDataPath(dataPath);

   telux::data::net::TcConfigErrorCode tcConfigErrorCode;
   telux::common::ErrorCode errorCode
      = dataQoSManager_->createTrafficClass(tcConfigBuilder.build(), tcConfigErrorCode);
   if (errorCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << " Create traffic class is successful." << std::endl;
      trafficClass_.push_back(trafficClass);
   } else {
      std::cout << " Create traffic class is failed. ErrorCode: " << static_cast<int>(errorCode)
                  << " " << static_cast<int>(tcConfigErrorCode) << std::endl;
      return false;
   }
   return true;
}

// Create downlink traffic class
bool DataQoSApp::createDownLinkTrafficClass(int trafficClass, telux::data::net::DataPath dataPath,
   int minBandwidth, int maxBandwidth) {
   telux::data::net::BandwidthConfig bandwidthConfig;
   bandwidthConfig.setDlBandwidthRange(minBandwidth, maxBandwidth);
   telux::data::net::TcConfigBuilder tcConfigBuilder;
   tcConfigBuilder.setTrafficClass(trafficClass).
      setDirection(telux::data::Direction::DOWNLINK).
      setDataPath(dataPath).
      setBandwidthConfig(bandwidthConfig);

   telux::data::net::TcConfigErrorCode tcConfigErrorCode;
   telux::common::ErrorCode errorCode
      = dataQoSManager_->createTrafficClass(tcConfigBuilder.build(), tcConfigErrorCode);
   if (errorCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << " Create traffic class is successful." << std::endl;
      trafficClass_.push_back(trafficClass);
   } else {
      std::cout << " Create traffic class is failed. ErrorCode: " << static_cast<int>(errorCode)
                  << " " << static_cast<int>(tcConfigErrorCode) << std::endl;
      return false;
   }
   return true;
}

// Add PCP and VLAN based QoS filter.
uint32_t DataQoSApp::addVlanPcpQoSFilter(int trafficClass, telux::data::Direction direction,
   telux::data::net::DataPath dataPath, int vlanId, int pcp) {
   telux::data::TrafficFilterBuilder tfBuilder;
   tfBuilder.setDirection(direction).
   setVlanList({vlanId}, telux::data::FieldType::DESTINATION).
   setDataPath(dataPath).setPCP(pcp);

   // Configure QoS filter
   telux::data::net::QoSFilterConfig qosFilterConfig = {0};
   // traffic class
   qosFilterConfig.trafficClass = trafficClass;
   // traffic filter
   qosFilterConfig.trafficFilter = tfBuilder.build();

   // Add QoS filter
   uint32_t policyHandle;

   telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
   telux::common::ErrorCode errorCode =
      dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
   if (errorCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                  << std::endl;
      qosFilterHandles_.push_back(policyHandle);
   } else {
      std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                  << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
      return 0;
   }
   return policyHandle;
}


// Add VLAN based QoS filter.
uint32_t DataQoSApp::addVlanQoSFilter(int trafficClass, telux::data::Direction direction,
   telux::data::net::DataPath dataPath, int vlanId) {
   telux::data::TrafficFilterBuilder tfBuilder;
   tfBuilder.setDirection(direction).setDataPath(dataPath);
   if(direction == telux::data::Direction::DOWNLINK) {
      tfBuilder.setVlanList({vlanId}, telux::data::FieldType::DESTINATION);
   } else {
      tfBuilder.setVlanList({vlanId}, telux::data::FieldType::SOURCE);
   }

   // Configure QoS filter
   telux::data::net::QoSFilterConfig qosFilterConfig = {0};
   // traffic class
   qosFilterConfig.trafficClass = trafficClass;
   // traffic filter
   qosFilterConfig.trafficFilter = tfBuilder.build();

   // Add QoS filter
   uint32_t policyHandle;
   telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
   telux::common::ErrorCode errorCode =
      dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
   if (errorCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                  << std::endl;
      qosFilterHandles_.push_back(policyHandle);
      return policyHandle;
   } else {
      std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                  << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
      return 0;
   }
}

// Add IPv4 based QoS filter
uint32_t DataQoSApp::addIPv4QoSFilter(int trafficClass, telux::data::Direction direction,
   telux::data::net::DataPath dataPath, int protocol, std::string srcIPv4, int destPort,
   int srcPort) {

   telux::data::TrafficFilterBuilder tfBuilder;
   tfBuilder.setDirection(direction).
      setIPv4Address(srcIPv4, telux::data::FieldType::SOURCE).
      setIPProtocol(protocol).
      setDataPath(dataPath);
   if(destPort != -1) {
      tfBuilder.setPort(destPort, telux::data::FieldType::DESTINATION);
   }
   if(srcPort != -1) {
      tfBuilder.setPort(srcPort, telux::data::FieldType::SOURCE);
   }

   // Configure QoS filter
   telux::data::net::QoSFilterConfig qosFilterConfig = {0};
   // traffic class
   qosFilterConfig.trafficClass = trafficClass;
   // traffic filter
   qosFilterConfig.trafficFilter = tfBuilder.build();

   // Add QoS filter
   uint32_t policyHandle;
   telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
   telux::common::ErrorCode errorCode =
      dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
   if (errorCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                  << std::endl;
      qosFilterHandles_.push_back(policyHandle);
      return policyHandle;
   } else {
      std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                  << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
      return 0;
   }
}

// 1. VLAN-based downlink traffic, tethered to apps software path
bool DataQoSApp::createTCAndAddQoSFilterForDLTetheredToAppsSWPath() {
   std::cout <<
   "\n\n1. VLAN-based downlink traffic, tethered to apps software path:\n"
   "       Steps:\n"
   "       - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 20\n"
   "             HW Acceleration = False\n"
   "             PCP = 7\n"
   "       - Create traffic class\n"
   "             TC ID = 0\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             BW Config {min = 5Mbps, max = 10Mbps}\n"
   "             Direction = DOWNLINK\n"
   "       - Add VLAN based QoS filter\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             Direction = DOWNLINK\n"
   "             PCP = 7\n"
   "             VLAN IDs = [20]\n";
   std::cout << "\n\nPress ENTER to execute use case 1 \n\n";
   std::cin.ignore();
   // Create VLAN
   // Note: All vlan configuration as prerequisite added in
   //       "./qos_sample_app -v"  i.e. configureVLANs()

   // Create traffic class for downlink
   if(!createDownLinkTrafficClass(0, telux::data::net::DataPath::TETHERED_TO_APPS_SW, 5, 10)) {
      return false;
   }

   // Create traffic class not needed as using same traffic class created above.
   // Add VLAN based QoS filter
   if(int handle = addVlanPcpQoSFilter(0, telux::data::Direction::DOWNLINK,
      telux::data::net::DataPath::TETHERED_TO_APPS_SW, 20, 7)) {
      // Get QoS filter
      std::shared_ptr<telux::data::net::IQoSFilter> qosFilterInfo;
      telux::common::ErrorCode errorCode = dataQoSManager_->getQosFilter(handle, qosFilterInfo);
      if (errorCode == telux::common::ErrorCode::SUCCESS) {
         std::cout << " Request QoS filter is successful." << std::endl;
         std::cout << qosFilterInfo->toString() << std::endl;
      } else {
         std::cout << " Get QoS filter has failed. ErrorCode: " << static_cast<int>(errorCode)
         << std::endl;
         return false;
      }
   } else {
      return false;
   }
   return true;

}

// 2. VLAN-based uplink traffic, tethered to the apps software path:
bool DataQoSApp::createTCAndAddQoSFilterForULTetheredToAppsSWPath() {
   std::cout <<
   "\n\n2. VLAN-based uplink traffic, tethered to the apps software path:\n"
   "       Steps:\n"
   "       - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 19\n"
   "             HW Acceleration = False\n"
   "             PCP = 7\n"
   "       - Create traffic class\n"
   "             TC ID = 0\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             Direction = UPLINK\n"
   "       - Add VLAN based QoS filter\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             Direction = UPLINK\n"
   "             PCP = 7\n"
   "             VLAN IDs = [19]\n";
   std::cout << "\n\nPress ENTER to execute use case 2 \n\n";
   std::cin.ignore();

   // Create VLAN
   // Note: All vlan configuration as prerequisite added in
   //       "./qos_sample_app -v"  i.e. configureVLANs()

   // Create traffic class for uplink traffic
   if(!createUplinkTrafficClass(0, telux::data::net::DataPath::TETHERED_TO_APPS_SW)) {
      return false;
   }

   // Create traffic class not needed as using same traffic class created above.
   // Add VLAN based QoS filter
   if(int handle = addVlanPcpQoSFilter(0, telux::data::Direction::UPLINK,
      telux::data::net::DataPath::TETHERED_TO_APPS_SW, 19, 7)) {
      // Get QoS filter
      std::shared_ptr<telux::data::net::IQoSFilter> qosFilterInfo;
      telux::common::ErrorCode errorCode = dataQoSManager_->getQosFilter(handle, qosFilterInfo);
      if (errorCode == telux::common::ErrorCode::SUCCESS) {
         std::cout << " Request QoS filter is successful." << std::endl;
         std::cout << qosFilterInfo->toString() << std::endl;
      } else {
         std::cout << " Get QoS filter has failed. ErrorCode: " << static_cast<int>(errorCode)
         << std::endl;
         return false;
      }
   } else {
      return false;
   }
   return true;
}

// 3.  i. IPv4-based downlink traffic, tethered to the WAN hardware accelerated path
//    ii. VLAN-based downlink traffic, tethered to the WAN hardware accelerated path
bool DataQoSApp::createTCAndAddQoSFilterForDLTetheredToWanHWAccPath() {
   std::cout <<
   "\n\n3.  Tethered to the WAN downlink hardware accelerated path:\n"
   "        Steps:\n"
   "        - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 18\n"
   "             HW Acceleration = True\n"
   "             PCP not set (Internally PCP = 0)\n"
   "        - Bind VLAN-18 to default WWAN Backhaul\n"
   "        - Bring-up data call\n"
   "        - Create traffic class\n"
   "             TC ID = 1\n"
   "             BW Config {min = 5Mbps, max = 10Mbps}\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = DOWNLINK\n\n"
   "\n      i)  IPv4-based downlink traffic, tethered to the WAN hardware accelerated path:\n"
   "        - Add IP based QoS filter\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = DOWNLINK\n"
   "             Source IP = Remote server (e.g. environment variable\n"
   "                                             TETHERED_TO_WAN_HW_DL_SOURCE_IP)\n"
   "             Destination port = 30044\n"
   "             Protocol = TCP (6 as per IANA)\n"
   "             Source port = 8080\n";
   std::cout << "\n\nPress ENTER to execute use case 3. i \n\n";
   std::cin.ignore();
   // Bring-up data call
   if(!startAndWaitForDataCall()){
      return false;
   }

   // Create traffic class for downlink
   if(!createDownLinkTrafficClass(1, telux::data::net::DataPath::TETHERED_TO_WAN_HW, 5, 10)) {
      return false;
   }

   // Create traffic filter (IP based)
   // Note: For DOWNLINK IP-based filter, any reduced combination from the 5-tuple can be provided.
   int handle;
   char * ip = std::getenv("TETHERED_TO_WAN_HW_DL_SOURCE_IP");
   std::string sourceIp = (ip == NULL ? "142.250.132.100" : std::string(ip));
   if(handle = addIPv4QoSFilter(1, telux::data::Direction::DOWNLINK,
      telux::data::net::DataPath::TETHERED_TO_WAN_HW, 6, sourceIp, 30044, 8080)) {
      // Get QoS filter
      std::shared_ptr<telux::data::net::IQoSFilter> qosFilterInfo;
      telux::common::ErrorCode errorCode = dataQoSManager_->getQosFilter(handle, qosFilterInfo);
      if (errorCode == telux::common::ErrorCode::SUCCESS) {
         std::cout << " Request QoS filter is successful." << std::endl;
         std::cout << qosFilterInfo->toString() << std::endl;
      } else {
         std::cout << " Get QoS filter has failed. ErrorCode: " << static_cast<int>(errorCode)
         << std::endl;
         return false;
      }
   } else {
      return false;
   }

   std::cout <<
   "\n    ii) VLAN-based downlink traffic, tethered to the WAN hardware accelerated path:\n"
   "      Steps:\n"
   "       - Add VLAN based QoS filter\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = DOWNLINK\n"
   "             VLAN IDs = [18]\n";
   std::cout << "\n\nPress ENTER to execute use case 3. ii \n\n";
   std::cin.ignore();
   // Create VLAN
   // Note: All vlan configuration as prerequisite added in
   //       "./qos_sample_app -v"  i.e. configureVLANs()

   // Create traffic class not needed as using same traffic class created above.
   // Add VLAN based QoS filter
   if(handle = addVlanQoSFilter(1, telux::data::Direction::DOWNLINK,
      telux::data::net::DataPath::TETHERED_TO_WAN_HW, 18)) {
      // Get QoS filter
      std::shared_ptr<telux::data::net::IQoSFilter> qosFilterInfo;
      telux::common::ErrorCode errorCode = dataQoSManager_->getQosFilter(handle, qosFilterInfo);
      if (errorCode == telux::common::ErrorCode::SUCCESS) {
         std::cout << " Request QoS filter is successful." << std::endl;
         std::cout << qosFilterInfo->toString() << std::endl;
         return true;
      } else {
         std::cout << " Get QoS filter has failed. ErrorCode: " << static_cast<int>(errorCode)
         << std::endl;
         return false;
      }
   } else {
      return false;
   }
}

// 4. IPv4-based uplink traffic, tethered to the WAN hardware accelerated path
bool DataQoSApp::createTCAndAddQoSFilterForULTetheredToWanHWAccPath() {
   std::cout <<
   "\n\n4. IPv4-based uplink traffic, tethered to the WAN hardware accelerated path:\n"
   "       Steps:\n"
   "       - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 18\n"
   "             HW Acceleration = True\n"
   "             PCP not set (Internally PCP = 0)\n"
   "       - Bind VLAN-18 to default WWAN Backhaul\n"
   "       - Bring-up data call\n"
   "       - Create traffic class\n"
   "             TC ID = 1\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = UPLINK\n"
   "       - Add IP based QoS filter\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = UPLINK\n"
   "             Source IP = <Public IP> from IDataCall object\n"
   "             Destination port = 8081\n"
   "             Protocol = TCP (6 as per IANA)\n";

   std::cout << "\n\nPress ENTER to execute use case 4 \n\n";
   std::cin.ignore();
   // Bring-up data call if not already started
   if(!startAndWaitForDataCall()){
      return false;
   }

   // Create traffic class for uplink traffic
   if(!createUplinkTrafficClass(1, telux::data::net::DataPath::TETHERED_TO_WAN_HW)) {
      return false;
   }

   // Create traffic filter (IP based)
   // Note: For UPLINK IP-based filter involving modem, source IP, protocol, and one field
   // from destination ip or destination port is mandatory.
   if(int handle = addIPv4QoSFilter(1, telux::data::Direction::UPLINK,
      telux::data::net::DataPath::TETHERED_TO_WAN_HW, 6, rmnetIp_, 8081)) {
      // Get QoS filter
      std::shared_ptr<telux::data::net::IQoSFilter> qosFilterInfo;
      telux::common::ErrorCode errorCode = dataQoSManager_->getQosFilter(handle, qosFilterInfo);
      if (errorCode == telux::common::ErrorCode::SUCCESS) {
         std::cout << " Request QoS filter is successful." << std::endl;
         std::cout << qosFilterInfo->toString() << std::endl;
         return true;
      } else {
         std::cout << " Get QoS filter has failed. ErrorCode: " << static_cast<int>(errorCode)
         << std::endl;
         return false;
      }
   } else {
      return false;
   }
}

// Create a traffic class and add a QoS filter for uplink traffic, from apps to the WAN path:
bool DataQoSApp::createTCAndAddQoSFilterForULAppsToWanPath() {
   std::cout <<
   "\n\n5. IPv4-based uplink traffic, from apps to the WAN path\n"
   "       Steps:\n"
   "       - Bring-up data call\n"
   "       - Create traffic class\n"
   "             TC ID = 2\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = UPLINK\n"
   "       - Add IP based QoS filter\n"
   "             Data path = APPS_TO_WAN\n"
   "             Direction = UPLINK\n"
   "             Source IP = <Public IP> from IDataCall object\n"
   "             Destination port = 8080\n"
   "             Protocol = UDP (17 as per IANA)\n";
   std::cout << "\n\nPress ENTER to execute use case 5 \n\n";
   std::cin.ignore();
   // Bring-up data call if not already started
   if(!startAndWaitForDataCall()){
      return false;
   }

   // Create traffic class for uplink
   if(!createUplinkTrafficClass(2, telux::data::net::DataPath::TETHERED_TO_WAN_HW)) {
      return false;
   }

   // Create traffic filter (IP based)
   if(int handle = addIPv4QoSFilter(2, telux::data::Direction::UPLINK,
      telux::data::net::DataPath::APPS_TO_WAN, 17, rmnetIp_, 8080)) {
      // Get QoS filter
      std::shared_ptr<telux::data::net::IQoSFilter> qosFilterInfo;
      telux::common::ErrorCode errorCode = dataQoSManager_->getQosFilter(handle, qosFilterInfo);
      if (errorCode == telux::common::ErrorCode::SUCCESS) {
         std::cout << " Request QoS filter is successful." << std::endl;
         std::cout << qosFilterInfo->toString() << std::endl;
      } else {
         std::cout << " Get QoS filter has failed. ErrorCode: " << static_cast<int>(errorCode)
         << std::endl;
         return false;
      }
   } else {
      return false;
   }

   return true;
}

void DataQoSApp::printUseCases() {

   std::cout <<
   "\n\n1. VLAN-based downlink traffic, tethered to apps software path:\n"
   "       Steps:\n"
   "       - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 20\n"
   "             HW Acceleration = False\n"
   "             PCP = 7\n"
   "       - Create traffic class\n"
   "             TC ID = 0\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             BW Config {min = 5Mbps, max = 10Mbps}\n"
   "             Direction = DOWNLINK\n"
   "       - Add VLAN based QoS filter\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             Direction = DOWNLINK\n"
   "             PCP = 7\n"
   "             VLAN IDs = [20]\n";

   std::cout <<
   "\n\n2. VLAN-based uplink traffic, tethered to the apps software path:\n"
   "       Steps:\n"
   "       - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 19\n"
   "             HW Acceleration = False\n"
   "             PCP = 7\n"
   "       - Create traffic class\n"
   "             TC ID = 0\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             Direction = UPLINK\n"
   "       - Add VLAN based QoS filter\n"
   "             Data path = TETHERED_TO_APPS_SW\n"
   "             Direction = UPLINK\n"
   "             PCP = 7\n"
   "             VLAN IDs = [19]\n";

   std::cout <<
   "\n\n3.  Tethered to the WAN downlink hardware accelerated path:\n"
   "        Steps:\n"
   "        - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 18\n"
   "             HW Acceleration = True\n"
   "             PCP not set (Internally PCP = 0)\n"
   "        - Bind VLAN-18 to default WWAN Backhaul\n"
   "        - Bring-up data call\n"
   "        - Create traffic class\n"
   "             TC ID = 1\n"
   "             BW Config {min = 5Mbps, max = 10Mbps}\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = DOWNLINK\n\n"
   "\n    i)  IPv4-based downlink traffic, tethered to the WAN hardware accelerated path: \n"
   "        - Add IP based QoS filter\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = DOWNLINK\n"
   "             Source IP = Remote server\n"
   "             Destination port = 30044\n"
   "             Protocol = TCP (6 as per IANA)\n"
   "             Source port = 8080\n"
   "\n    ii) VLAN-based downlink traffic, tethered to the WAN hardware accelerated path:\n"
   "        Steps:\n"
   "       - Add VLAN based QoS filter\n"
   "             TC ID = 1\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = DOWNLINK\n"
   "             VLAN IDs = [18]\n";

   std::cout <<
   "\n\n4. IPv4-based uplink traffic, tethered to the WAN hardware accelerated path:\n"
   "    Steps:\n"
   "       - Pre-requisite: VLAN created with below attributes\n"
   "             ID = 18\n"
   "             HW Acceleration = True\n"
   "             PCP not set (Internally PCP = 0)\n"
   "       - Bind VLAN-18 to default WWAN Backhaul\n"
   "       - Bring-up data call\n"
   "       - Create traffic class\n"
   "             TC ID = 1\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = UPLINK\n"
   "       - Add IP based QoS filter\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = UPLINK\n"
   "             Source IP = <Public IP> from IDataCall object\n"
   "             Destination port = 8081\n"
   "             Protocol = TCP (6 as per IANA)\n";

   std::cout <<
   "\n\n5. IPv4-based uplink traffic, from apps to the WAN path\n"
   "    Steps:\n"
   "       - Bring-up data call\n"
   "       - Create traffic class\n"
   "             TC ID = 2\n"
   "             Data path = TETHERED_TO_WAN_HW\n"
   "             Direction = UPLINK\n"
   "       - Add IP based QoS filter\n"
   "             Data path = APPS_TO_WAN\n"
   "             Direction = UPLINK\n"
   "             Source IP = <Public IP> from IDataCall object\n"
   "             Destination port = 8080\n"
   "             Protocol = UDP (17 as per IANA)\n";
}

void DataQoSApp::runUseCase(int useCase) {

   switch (useCase) {
      case 1:
         // 1. VLAN-based downlink traffic, tethered to apps software path
         if(createTCAndAddQoSFilterForDLTetheredToAppsSWPath()) {
            std::cout << "\n\nSuccessful 1. VLAN-based downlink traffic, QoS filter \n\n";
         } else {
            std::cout << "\n\nerror in 1. VLAN-based downlink traffic, QoS filter \n\n";
         }
      break;

      case 2:
         // 2. VLAN-based uplink traffic, tethered to the apps software path
         if(createTCAndAddQoSFilterForULTetheredToAppsSWPath()) {
            std::cout << "\n\nSuccessful 2. VLAN-based uplink traffic, QoS filter \n\n";
         } else {
            std::cout << "\n\nerror in 2. VLAN-based uplink traffic, QoS filter \n\n";
         }
      break;

      case 3:
         // 3.  i.  IPv4-based downlink traffic, tethered to the WAN hardware accelerated path
         //     ii. VLAN-based downlink traffic, tethered to the WAN hardware accelerated path
         if(createTCAndAddQoSFilterForDLTetheredToWanHWAccPath()) {
            std::cout << "\nSuccessful 3. i, ii IPv4, VLAN based downlink traffic, QoS filter \n\n";
         } else {
            std::cout << "\nerror in 3.  i, ii IPv4, VLAN based downlink traffic, QoS filter \n\n";
         }
      break;
      case 4:
         // 4. IPv4-based uplink traffic, tethered to the WAN hardware accelerated path
         if(createTCAndAddQoSFilterForULTetheredToWanHWAccPath()) {
            std::cout << "\nSuccessful 4. IPv4-based uplink traffic, QoS filter \n\n";
         } else {
            std::cout << "\nerror in 4. IPv4-based uplink traffic, QoS filter \n\n";
         }
      break;
      case 5:
         // 5. IPv4-based uplink traffic, from apps to the WAN path
         if(createTCAndAddQoSFilterForULAppsToWanPath()) {
            std::cout << "\nSuccessful in 5. IPv4-based uplink traffic, QoS filter \n\n";
         } else {
            std::cout << "\n\nerror in 5. IPv4-based uplink traffic, QoS filter \n\n";
         }
      break;
      default:
         if(createTCAndAddQoSFilterForDLTetheredToAppsSWPath()) {
            std::cout << "\n\nSuccessful 1. VLAN-based downlink traffic, QoS filter \n\n";
         } else {
            std::cout << "\n\nerror in 1. VLAN-based downlink traffic, QoS filter \n\n";
         }
         if(createTCAndAddQoSFilterForULTetheredToAppsSWPath()) {
            std::cout << "\n\nSuccessful 2. VLAN-based uplink traffic, QoS filter \n\n";
         } else {
            std::cout << "\n\nerror in 2. VLAN-based uplink traffic, QoS filter \n\n";
         }
         if(createTCAndAddQoSFilterForDLTetheredToWanHWAccPath()) {
            std::cout << "\nSuccessful 3. i, ii IPv4, VLAN based downlink traffic, QoS filter \n\n";
         } else {
            std::cout << "\nerror in 3. i, ii IPv4, VLAN based downlink traffic, QoS filter \n\n";
         }
         if(createTCAndAddQoSFilterForULTetheredToWanHWAccPath()) {
            std::cout << "\nSuccessful 4. IPv4-based uplink traffic, QoS filter \n\n";
         } else {
            std::cout << "\nerror in 4. IPv4-based uplink traffic, QoS filter \n\n";
         }
         if(createTCAndAddQoSFilterForULAppsToWanPath()) {
            std::cout << "\nSuccessful in 5. IPv4-based uplink traffic, QoS filter \n\n";
         } else {
            std::cout << "\n\nerror in 5. IPv4-based uplink traffic, QoS filter \n\n";
         }
   }
}

void DataQoSApp::showAllConfigs() {
   if(initVlanManager()) {
      if (!loadDefaultProfile()) {
         std::cout << " Failed to load default profile";
         return;
      }

      // Query VLAN info
      std::cout << "Query VLAN info\n";
      std::promise<telux::common::ErrorCode> vlanProm;
      auto respCb = [this, &vlanProm](const std::vector<telux::data::VlanConfig> &configs,
         telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                     << "queryVlanInfo Response"
                     << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                     << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         if (configs.size() == 0) {
               std::cout << "No VLAN Entries Configured" << "\n";
         } else {
               for (auto c : configs) {
                  std::cout << "iface: "
                           << vlanInterfaceToString(c.iface, telux::data::OperationType::DATA_LOCAL)
                           << ", vlanId: " << c.vlanId
                           << ", Priority: " << static_cast<int>(c.priority)
                           << ", accelerated: " << (int)c.isAccelerated << "\n";
               }
         }
         vlanProm.set_value(error);
      };
      if(vlanManager_->queryVlanInfo(respCb) != telux::common::Status::SUCCESS) {
         std::cout << "queryVlanInfo failed" << "\n";
      }
      vlanProm.get_future().get();

      // Query VLAN To Backhaul Bindings
      std::cout << "Query VLAN To Backhaul Bindings " << std::endl;
      std::promise<telux::common::ErrorCode> vlanBindProm;

      auto respBindCb =
         [this, &vlanBindProm](const std::vector<telux::data::net::VlanBindConfig> bindings,
            telux::common::ErrorCode error) {
            std::cout << std::endl << std::endl;
            std::cout << "CALLBACK: "
                        << "queryVlanToBackhaulBindings Response"
                        << (error == telux::common::ErrorCode::SUCCESS
                              ? " is successful"
                              : " failed")
                        << ". ErrorCode: " << static_cast<int>(error)
                        << std::endl;
            for (auto c : bindings) {
               std::cout << "Backhaul: "
                           << backhaulToString(c.bhInfo.backhaul);
               if (c.bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
                  std::cout << ", profile id: " << c.bhInfo.profileId;
               }
               std::cout << ", vlanId: " << c.vlanId << "\n";
            }
            vlanBindProm.set_value(error);
         };
      if(vlanManager_->queryVlanToBackhaulBindings(
         telux::data::BackhaulType::WWAN, respBindCb, slotId_) != telux::common::Status::SUCCESS) {
         std::cout << "queryVlanInfo failed" << "\n";
      }
      vlanBindProm.get_future().get();
   }

   // Get all traffic classes
   std::cout << "Get all traffic classes" << std::endl;
   std::vector<std::shared_ptr<telux::data::net::ITcConfig>> tcConfigs;
   telux::common::ErrorCode errorCode = dataQoSManager_->getAllTrafficClasses(tcConfigs);
   if (errorCode == telux::common::ErrorCode::SUCCESS)
      std::cout << " Request get all traffic classes is successful." << std::endl;
   else {
      std::cout << " The request of get all traffic classes has failed. ErrorCode: "
               << static_cast<int>(errorCode) << std::endl;
      return;
   }

   for (size_t i = 0; i < tcConfigs.size(); ++i) {
      std::cout << tcConfigs[i]->toString() << std::endl;
   }

   // Request QoS filters
   std::cout << "request QoS filters" << std::endl;
   std::vector<std::shared_ptr<telux::data::net::IQoSFilter>> qosFilterInfo;
   errorCode = dataQoSManager_->getQosFilters(qosFilterInfo);
   if (errorCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << " Request QoS filters is successful. Count " << qosFilterInfo.size()
               << std::endl;
      for (size_t i = 0; i < qosFilterInfo.size(); ++i) {
         std::cout << qosFilterInfo[i]->toString() << std::endl;
      }
   } else {
      std::cout << " Get QoS filters has failed. ErrorCode: " << static_cast<int>(errorCode)
      << std::endl;
   }

}

std::string DataQoSApp::backhaulToString(telux::data::BackhaulType backhaul) {
   std::string retString = "UNKNOWN";
   switch(backhaul) {
      case telux::data::BackhaulType::ETH:
         retString = "ETH";
         break;
      case telux::data::BackhaulType::USB:
         retString = "USB";
         break;
      case telux::data::BackhaulType::WLAN:
         retString = "WLAN";
         break;
      case telux::data::BackhaulType::WWAN:
         retString = "WWAN";
         break;
      case telux::data::BackhaulType::BLE:
         retString = "BLE";
         break;
      default:
         break;
   }
   return retString;
}

std::string DataQoSApp::vlanInterfaceToString(
   telux::data::InterfaceType interface, telux::data::OperationType oprType) {
   std::string ifName = "UNKNOWN";
   switch(interface) {
      case telux::data::InterfaceType::WLAN:
         ifName = "WLAN";
         break;
      case telux::data::InterfaceType::ETH:
         ifName = "ETH";
         break;
      case telux::data::InterfaceType::ECM:
         ifName = "ECM";
         break;
      case telux::data::InterfaceType::RNDIS:
         ifName = "RNDIS";
         break;
      case telux::data::InterfaceType::MHI:
         ifName = "MHI";
         break;
      case telux::data::InterfaceType::VMTAP0:
#ifdef TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED
         if(oprType == telux::data::OperationType::DATA_LOCAL) {
            ifName = "VMTAP0";
         } else {
            ifName = "VMTAP-TELEVM";
         }
#else
         ifName = "VMTAP-TELEVM";
#endif
         break;
      case telux::data::InterfaceType::VMTAP1:
#ifdef TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED
         if(oprType == telux::data::OperationType::DATA_LOCAL) {
            ifName = "VMTAP1";
         } else {
            ifName = "VMTAP-FOTAVM";
         }
#else
         ifName = "VMTAP-FOTAVM";
#endif
         break;
      default:
         break;
   }
   return ifName;
}

void DataQoSApp::printHelp() {
    std::cout << "             Data QoS App\n"
    << "---------------------------------------------------------------\n"
    << "-c           Console app mode\n\n"
    << "-v           Configure VLANs \n\n"
    << "-x           Clear VLAN configurations\n\n"
    << "-l           List use case\n\n"
    << "-u <ID>      Run use case\n\n"
    << "-d           Delete all QoS TC and filter configurations\n\n"
    << "-s           Show all configurations\n\n"
    << "-h           Help\n" << std::endl;
}

void DataQoSApp::consoleInit() {
   std::shared_ptr<ConsoleAppCommand> configureVLANs
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Configure_VLANs", {}, std::bind(&DataQoSApp::configureVLANs, this)));
   std::shared_ptr<ConsoleAppCommand> clearVlanConfigs
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Clear_VLAN_Configuration", {}, std::bind(&DataQoSApp::clearVlanConfigs, this)));
   std::shared_ptr<ConsoleAppCommand> createTCAndAddQoSFilterForDLTetheredToAppsSWPath
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Create_TC_And_Add_QoS_Filter_For_DL_Tethered_To_Apps_SW_Path", {},
         std::bind(&DataQoSApp::createTCAndAddQoSFilterForDLTetheredToAppsSWPath, this)));
   std::shared_ptr<ConsoleAppCommand> createTCAndAddQoSFilterForULTetheredToAppsSWPath
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Create_TC_And_Add_QoS_Filter_For_UL_Tethered_To_Apps_SW_Path", {},
         std::bind(&DataQoSApp::createTCAndAddQoSFilterForULTetheredToAppsSWPath, this)));
   std::shared_ptr<ConsoleAppCommand> createTCAndAddQoSFilterForDLTetheredToWanHWAccPath
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "5", "Create_TC_And_Add_QoS_Filter_For_DL_Tethered_To_Wan_HW_Acc_Path", {},
         std::bind(&DataQoSApp::createTCAndAddQoSFilterForDLTetheredToWanHWAccPath, this)));
    std::shared_ptr<ConsoleAppCommand> createTCAndAddQoSFilterForULTetheredToWanHWAccPath
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "Create_TC_And_Add_QoS_Filter_For_UL_Tethered_To_Wan_HW_Acc_Path", {},
         std::bind(&DataQoSApp::createTCAndAddQoSFilterForULTetheredToWanHWAccPath, this)));
    std::shared_ptr<ConsoleAppCommand> createTCAndAddQoSFilterForULAppsToWanPath
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "7", "Create_TC_And_Add_QoS_Filter_For_UL_Apps_To_Wan_Path", {},
         std::bind(&DataQoSApp::createTCAndAddQoSFilterForULAppsToWanPath, this)));

    std::shared_ptr<ConsoleAppCommand> cleanupCmd
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "8", "Clean_all_traffic_class_and_qos_filters", {},
         std::bind(&DataQoSApp::cleanup, this, SIGINT)));

    std::shared_ptr<ConsoleAppCommand> showAllConfigsCmd
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "9", "show_all_configs", {},
         std::bind(&DataQoSApp::showAllConfigs, this)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListPowerMenu
      = { configureVLANs, clearVlanConfigs,
         createTCAndAddQoSFilterForDLTetheredToAppsSWPath,
         createTCAndAddQoSFilterForULTetheredToAppsSWPath,
         createTCAndAddQoSFilterForDLTetheredToWanHWAccPath,
         createTCAndAddQoSFilterForULTetheredToWanHWAccPath,
         createTCAndAddQoSFilterForULAppsToWanPath, cleanupCmd, showAllConfigsCmd};
   ConsoleApp::addCommands(commandsListPowerMenu);
   ConsoleApp::displayMenu();
}

bool DataQoSApp::configureVLANs() {
   // for use case 1
   if(!createAndWaitForVlan(20, false, 7)) {
      return false;
   }

   // use case 2
   if(!createAndWaitForVlan(19, true)) {
      return false;
   }

   // use case 4
   if(!createAndWaitForVlan(18, true)) {
      return false;
   }

   // Bind VLAN-18 to default WWAN Backhaul
   // Note: Binding a VLAN to the backhaul for the first time lead to a device restart.
   if(!bindVlanToBackhaul(18)) {
      return false;
   }
}

void DataQoSApp::clearVlanConfigs() {
   removeVlan(20);
   removeVlan(19);
   removeVlan(18);
}

// clear system
void DataQoSApp::cleanup(int signum) {
   telux::common::ErrorCode errorCode;
   // delete QoS filter
   if(qosFilterHandles_.size() > 0) {
      // delete individual QoS filter using handle
      std::cout << "delete QoS filter" << std::endl;
      errorCode = dataQoSManager_->deleteQosFilter(qosFilterHandles_[0]);
      if (errorCode == telux::common::ErrorCode::SUCCESS)
         std::cout << " Delete QoS filter is successful." << std::endl;
      else
         std::cout << " The deletion of the QoS filter has failed. ErrorCode: "
                  << static_cast<int>(errorCode) << std::endl;
   }

   // delete all QoS filters and traffic class
   errorCode = dataQoSManager_->deleteAllQosConfigs();
   if (errorCode == telux::common::ErrorCode::SUCCESS)
      std::cout << " The deletion of all QoS configs is successful" << std::endl;
   else
      std::cout << " The deletion of all QoS configs has failed. ErrorCode: "
               << static_cast<int>(errorCode) << std::endl;

   stopDataCall();

   std::signal(signum, SIG_DFL);
   if (std::raise(signum) != 0) {
      std::cout << "raise(): error \n";
   }
}

bool DataQoSApp::parseArguments(int argc, char **argv) {
   int c;
   static struct option long_options[] = {
      {"Console app mode",                            no_argument, 0, 'c'},
      {"Configure VLANs",                             no_argument, 0, 'v'},
      {"Clear VLAN configurations",                   no_argument, 0, 'x'},
      {"List use case",                               no_argument, 0, 'l'},
      {"Run use case",                                required_argument, 0, 'u'},
      {"Delete all QoS TC and filter configurations", no_argument, 0, 'd'},
      {"Show all configurations",                     no_argument, 0, 's'},
      {"help",                                        no_argument, 0, 'h'},
      {0, 0, 0, 0}
   };

   int option_index = 0;
   c = getopt_long(argc, argv, "u:cvxldsh", long_options, &option_index);
   if (c == -1) {
      // if no option is entered help is printed.
      c = 'h';
   }

   switch (c) {
      case 'c':
         consoleInit();
         mainLoop();
         break;
      case 'v':
         configureVLANs();
         break;
      case 'x':
         clearVlanConfigs();
         break;
      case 'l':
         printUseCases();
         break;
      case 'u':
         runUseCase(atoi(optarg));
         break;
      case 'd':
         cleanup(SIGINT);
         break;
      case 's':
         showAllConfigs();
         break;
      case 'h':
         printHelp();

   }

   return true;
}

int main(int argc, char *argv[]) {
   bool status = false;
   std::shared_ptr<DataQoSApp> dataQoSApp = DataQoSApp::getInstance();

   status = dataQoSApp->parseArguments(argc, argv);
   if (!status) {
      std::cout << "Unable to parse" << std::endl;
      return EXIT_FAILURE;
   }

   // exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();
   return EXIT_SUCCESS;
}
