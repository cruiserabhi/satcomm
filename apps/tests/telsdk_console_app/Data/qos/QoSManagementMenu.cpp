/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include "../DataUtils.hpp"
#include "QoSManagementMenu.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

QoSManagementMenu::QoSManagementMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

QoSManagementMenu::~QoSManagementMenu() {
}

bool QoSManagementMenu::init() {
    bool initStatus = initQoSManager();

    // If both local and remote QoS Managers fail, exit
    if (not initStatus) {
        return false;
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> createTrafficClass
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "create_traffic_class", {},
                std::bind(&QoSManagementMenu::createTrafficClass, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getAllTrafficClasses
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "get_all_traffic_classes",
                {},
                std::bind(&QoSManagementMenu::getAllTrafficClasses, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deleteTrafficClass
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "delete_traffic_class", {},
                std::bind(&QoSManagementMenu::deleteTrafficClass, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addQoSFilter
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "create_QoS_filter", {},
                std::bind(&QoSManagementMenu::addQoSFilter, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getQosFilter
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "get_QoS_filter", {},
                std::bind(&QoSManagementMenu::getQosFilter, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getQosFilters
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "get_QoS_filters", {},
                std::bind(&QoSManagementMenu::getQosFilters, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deleteQosFilter
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "delete_QoS_filter", {},
                std::bind(&QoSManagementMenu::deleteQosFilter, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deleteAllQosConfigs
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "delete_all_QoS_config",
                {},
                std::bind(&QoSManagementMenu::deleteAllQosConfigs, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
            = {createTrafficClass, getAllTrafficClasses, deleteTrafficClass, addQoSFilter,
                getQosFilter, getQosFilters, deleteQosFilter, deleteAllQosConfigs};

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

bool QoSManagementMenu::initQoSManager() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    bool retVal = false;

    auto initCb = std::bind(&QoSManagementMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto qosMgr = dataFactory.getQoSManager(initCb);
    if (qosMgr) {
        qosMgr->registerListener(shared_from_this());
        std::unique_lock<std::mutex> lck(mtx_);
        telux::common::ServiceStatus subSystemStatus = qosMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing "
                      << " QoS Manager subsystem, Please wait \n";
            cv_.wait(lck, [this] { return this->subSystemStatusUpdated_; });
            subSystemStatus = qosMgr->getServiceStatus();
        }

        // At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\n"
                      << " QoS Manager is ready" << std::endl;
            retVal = true;
            qosManager_ = qosMgr;
        } else {
            std::cout << "\n"
                      << " QoS Manager is not ready" << std::endl;
        }
    }
    return retVal;
}

void QoSManagementMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void QoSManagementMenu::addQoSFilter(std::vector<std::string> &inputCommand) {
    std::cout << "add QoS filter" << std::endl;

    telux::data::net::QoSFilterConfig qosFilterConfig = {0};
    // traffic class
    int input;
    std::cout << "Enter traffic class: ";
    std::cin >> input;
    Utils::validateInput(input);
    qosFilterConfig.trafficClass = static_cast<uint8_t>(input);

    // traffic filter
    qosFilterConfig.trafficFilter = getTrafficFilter();

    telux::data::net::QoSFilterHandle filterHandle;
    telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
    telux::common::ErrorCode errorCode
        = qosManager_->addQoSFilter(qosFilterConfig, filterHandle, qosFilterErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS)
        std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << filterHandle
                  << std::endl;
    else {
        std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << " "
                  << qosFilterErrorCodeToString(qosFilterErrorCode) << std::endl;
    }
}

void QoSManagementMenu::getQosFilter(std::vector<std::string> &inputCommand) {
    std::cout << "request QoS filter" << std::endl;
    uint32_t handle;
    std::cout << "Enter QoS filter handle: ";
    std::cin >> handle;
    Utils::validateInput(handle);

    std::shared_ptr<telux::data::net::IQoSFilter> qosFilterInfo;
    telux::common::ErrorCode errorCode = qosManager_->getQosFilter(handle, qosFilterInfo);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Request QoS filter is successful." << std::endl;
        std::cout << qosFilterInfo->toString() << std::endl;
    } else {
        std::cout << " Get QoS filter has failed. ErrorCode: " << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}

void QoSManagementMenu::getQosFilters(std::vector<std::string> &inputCommand) {
    std::cout << "request QoS filters" << std::endl;
    std::vector<std::shared_ptr<telux::data::net::IQoSFilter>> qosFilterInfo;
    telux::common::ErrorCode errorCode = qosManager_->getQosFilters(qosFilterInfo);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Request QoS filters is successful. Count " << qosFilterInfo.size()
                  << std::endl;
        for (size_t i = 0; i < qosFilterInfo.size(); ++i) {
            std::cout << qosFilterInfo[i]->toString() << std::endl;
        }
    } else {
        std::cout << " Get QoS filters has failed. ErrorCode: " << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}

void QoSManagementMenu::deleteQosFilter(std::vector<std::string> &inputCommand) {
    std::cout << "delete QoS filter" << std::endl;

    uint32_t handle;
    std::cout << "Enter QoS filter handle: ";
    std::cin >> handle;
    Utils::validateInput(handle);

    telux::common::ErrorCode errorCode = qosManager_->deleteQosFilter(handle);
    if (errorCode == telux::common::ErrorCode::SUCCESS)
        std::cout << " Delete QoS filter is successful." << std::endl;
    else
        std::cout << " The deletion of the QoS filter has failed. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
}

void QoSManagementMenu::deleteAllQosConfigs(std::vector<std::string> &inputCommand) {
    std::cout << "delete all QoS filter" << std::endl;

    telux::common::ErrorCode errorCode = qosManager_->deleteAllQosConfigs();
    if (errorCode == telux::common::ErrorCode::SUCCESS)
        std::cout << " The deletion of all QoS configs is successful" << std::endl;
    else
        std::cout << " The deletion of all QoS configs has failed. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
}

void QoSManagementMenu::createTrafficClass(std::vector<std::string> &inputCommand) {
    std::cout << "Create traffic class" << std::endl;
    int input;
    telux::data::net::TcConfigBuilder tcConfigBuilder;

    // traffic class
    telux::data::TrafficClass tc;
    std::cout << "Enter the traffic class : ";
    std::cin >> input;
    Utils::validateInput(input);
    tc = static_cast<uint8_t>(input);
    tcConfigBuilder.setTrafficClass(tc);

    // direction
    telux::data::Direction direction = telux::data::Direction::UPLINK;

    std::cout << "Enter traffic direction (1 - UPLINK, 2 - DOWNLINK): ";
    std::cin >> input;
    Utils::validateInput(input, {1, 2});
    if (input == 1) {
        direction = telux::data::Direction::UPLINK;
    } else if (input == 2) {
        direction = telux::data::Direction::DOWNLINK;
    }
    tcConfigBuilder.setDirection(direction);

    // data path
    telux::data::net::DataPath dataPath;
    std::cout <<
        "\nConfigure data path: "
        "\n0 - TETHERED_TO_WAN_HW: Traffic classes with data path TETHERED_TO_WAN_HW can be"
        " associated with traffic filters with data path TETHERED_TO_WAN_HW and APPS_TO_WAN\n"
        "\n1 - TETHERED_TO_APPS_SW: Traffic classes with data path TETHERED_TO_APPS_SW can be"
        " associated with traffic filters with data path TETHERED_TO_APPS_SW and APPS_TO_WAN\n"
        "\n2 - APPS_TO_WAN: Traffic classes with data path APPS_TO_WAN can be associated with"
        " traffic filters with data path APPS_TO_WAN"
        "\n    Traffic classes created with APPS_TO_WAN can only be associated with UPLINK data"
        " path\n";
    std::cin >> input;
    Utils::validateInput(input, {0, 1, 2});
    dataPath = static_cast<telux::data::net::DataPath>(input);
    tcConfigBuilder.setDataPath(dataPath);

    // bandwidth
    if (direction == telux::data::Direction::DOWNLINK) {
        std::cout << "Enter bandwidth config (0 - Skip, 1 - Bandwidth range) :";
        std::cin >> input;
        Utils::validateInput(input, {0, 1});
        if (input) {
            telux::data::net::BandwidthConfig bandwidthConfig;
            uint32_t minBandwidth, maxBandwidth;
            std::cout << "Enter minimum bandwidth (Mbps): ";
            std::cin >> minBandwidth;
            Utils::validateInput(minBandwidth);
            std::cout << "Enter maximum bandwidth (Mbps): ";
            std::cin >> maxBandwidth;
            Utils::validateInput(maxBandwidth);
            bandwidthConfig.setDlBandwidthRange(minBandwidth, maxBandwidth);
            tcConfigBuilder.setBandwidthConfig(bandwidthConfig);
        }
    }

    telux::data::net::TcConfigErrorCode tcConfigErrorCode;
    telux::common::ErrorCode errorCode
        = qosManager_->createTrafficClass(tcConfigBuilder.build(), tcConfigErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS)
        std::cout << " Create traffic class is successful." << std::endl;
    else {
        std::cout << " Create traffic class is failed. ErrorCode: " << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << " "
                  << tcConfigErrorCodeToString(tcConfigErrorCode) << std::endl;
    }
}

void QoSManagementMenu::getAllTrafficClasses(std::vector<std::string> &inputCommand) {
    std::cout << "Get all traffic classes" << std::endl;
    std::vector<std::shared_ptr<telux::data::net::ITcConfig>> tcConfigs;
    telux::common::ErrorCode errorCode = qosManager_->getAllTrafficClasses(tcConfigs);
    if (errorCode == telux::common::ErrorCode::SUCCESS)
        std::cout << " Request get all traffic classes is successful." << std::endl;
    else {
        std::cout << " The request of get all traffic classes has failed. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
        return;
    }

    for (size_t i = 0; i < tcConfigs.size(); ++i) {
        std::cout << tcConfigs[i]->toString() << std::endl;
    }
}

void QoSManagementMenu::deleteTrafficClass(std::vector<std::string> &inputCommand) {
    std::cout << "delete traffic class" << std::endl;
    telux::data::net::TcConfigBuilder tcConfigBuilder;
    int input;
    // traffic class
    telux::data::TrafficClass tc;
    std::cout << "Enter the traffic class : ";
    std::cin >> input;
    Utils::validateInput(input);
    tc = static_cast<uint8_t>(input);
    tcConfigBuilder.setTrafficClass(tc);

    // direction
    telux::data::Direction direction = telux::data::Direction::UPLINK;
    std::cout << "Enter traffic direction (1 - UPLINK, 2 - DOWNLINK): ";
    std::cin >> input;
    Utils::validateInput(input, {1, 2});
    if (input == 1) {
        direction = telux::data::Direction::UPLINK;
    } else if (input == 2) {
        direction = telux::data::Direction::DOWNLINK;
    }
    tcConfigBuilder.setDirection(direction);

    telux::common::ErrorCode errorCode = qosManager_->deleteTrafficClass(tcConfigBuilder.build());
    if (errorCode == telux::common::ErrorCode::SUCCESS)
        std::cout << " Delete traffic class is successful." << std::endl;
    else
        std::cout << " The deletion of traffic class has failed. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
}

std::string QoSManagementMenu::qosFilterErrorCodeToString(
    telux::data::net::QoSFilterErrorCode qosFilterErr) {
    switch (qosFilterErr) {
        case telux::data::net::QoSFilterErrorCode::INVALID_MULTIPLE_SOURCE_INFO:
            return "INVALID_MULTIPLE_SOURCE_INFO";
        case telux::data::net::QoSFilterErrorCode::INVALID_MULTIPLE_DESTINATION_INFO:
            return "INVALID_MULTIPLE_DESTINATION_INFO";
        case telux::data::net::QoSFilterErrorCode::MISSING_DIRECTION:
            return "MISSING_DIRECTION";
        default:
            return "";
    }
}

std::string QoSManagementMenu::tcConfigErrorCodeToString(
    telux::data::net::TcConfigErrorCode tcErr) {
    switch (tcErr) {
        case telux::data::net::TcConfigErrorCode::MISSING_TRAFFIC_CLASS:
            return "MISSING_TRAFFIC_CLASS";
        case telux::data::net::TcConfigErrorCode::MISSING_DATA_PATH:
            return "MISSING_DATA_PATH";
        case telux::data::net::TcConfigErrorCode::MISSING_DIRECTION:
            return "MISSING_DIRECTION";
        default:
            return "";
    }
}

std::shared_ptr<telux::data::ITrafficFilter> QoSManagementMenu::getTrafficFilter() {
    std::cout << "Prepare traffic filter " << std::endl;
    int input;
    telux::data::TrafficFilterBuilder tfBuilder;

    // direction
    telux::data::Direction direction = telux::data::Direction::UPLINK;
    std::cout << "Enter traffic direction (1 - UPLINK, 2 - DOWNLINK): ";
    std::cin >> input;
    Utils::validateInput(input, {1, 2});
    if (input == 1) {
        direction = telux::data::Direction::UPLINK;
    } else if (input == 2) {
        direction = telux::data::Direction::DOWNLINK;
    }
    tfBuilder.setDirection(direction);

    // data path
    telux::data::net::DataPath dataPath;
    std::cout <<
        "\nConfigure data path: "
        "\n0 - Data flow between clients tethered to the NAD over Eth and the WAN interface using"
        " HW acceleration (Eth <=> IPA <=> Modem <=> WAN)"
        "\n1 - Data flows between clients tethered to the NAD over Eth and software running on the"
        " apps processor using a software path (Eth <=> Apps Processor)"
        "\n2 - Data flow between the apps processor and WAN (Apps Processor <=> WAN)\n";
    std::cin >> input;
    Utils::validateInput(input, {0, 1, 2});
    dataPath = static_cast<telux::data::net::DataPath>(input);
    tfBuilder.setDataPath(dataPath);

    // pcp
    std::cout << "Do you want to enter PCP info: [0 - Skip, 1 - Yes]: ";
    std::cin >> input;
    Utils::validateInput(input, {0, 1});
    if (input) {
        int pcp;
        std::cout << "Enter PCP (VLAN priority) number : ";
        std::cin >> pcp;
        Utils::validateInput(pcp);
        tfBuilder.setPCP(static_cast<int8_t>(pcp));
    }

    // source info
    std::cout << "Do you want to enter source info (0 - Skip, 1 - Source IP, 2 - "
                 "Source VLAN list"
                 "): ";
    std::cin >> input;
    Utils::validateInput(input, {0, 1, 2});

    switch (input) {
        case 1:
            getIPAddressParamsFromUser(tfBuilder, telux::data::FieldType::SOURCE);
            break;
        case 2:
            getVlanInfo(tfBuilder, telux::data::FieldType::SOURCE);
            break;
        default:
            break;
    }

    // source port
    std::cout << "Do you want to enter source port ";
    getPortsFromUser(tfBuilder, telux::data::FieldType::SOURCE);

    // destination info
    std::cout << "Do you want to enter destination info (0 - Skip, 1 - destination IP, "
                 "2 - destination VLAN list): ";
    std::cin >> input;
    Utils::validateInput(input, {0, 1, 2});

    switch (input) {
        case 1:
            getIPAddressParamsFromUser(tfBuilder, telux::data::FieldType::DESTINATION);
            break;
        case 2:
            getVlanInfo(tfBuilder, telux::data::FieldType::DESTINATION);
            break;

        default:
            break;
    }

    // destination port
    std::cout << "Do you want to enter destination port ";
    getPortsFromUser(tfBuilder, telux::data::FieldType::DESTINATION);

    // protocol
    std::cout << "Do you want to enter protocol info: [0 - Skip, 1 - Yes]: ";
    std::cin >> input;
    Utils::validateInput(input, {0, 1});
    if (input) {
        char delimiter = '\n';
        std::string protoStr;
        std::cout << "Enter Protocol (TCP, UDP): ";
        std::getline(std::cin, protoStr, delimiter);
        telux::data::IpProtocol proto = DataUtils::getProtcol(protoStr);
        tfBuilder.setIPProtocol(proto);
    }
    return tfBuilder.build();
}

void QoSManagementMenu::getVlanInfo(
    telux::data::TrafficFilterBuilder &tfBuilder, telux::data::FieldType fieldType) {
    char delimiter = '\n';
    std::string vlansStr = "";
    std::cout << "Enter VLAN list (For example: enter 10,20,30 ): ";
    std::getline(std::cin, vlansStr, delimiter);
    std::vector<int> vlans;
    std::stringstream ss(vlansStr);
    int i = -1;
    while (ss >> i) {
        if (i > 0) {
            vlans.push_back(static_cast<int>(i));
            if (ss.peek() == ',' || ss.peek() == ' ')
                ss.ignore();
        } else {
            std::cout << "ERROR: skipping invalid input";
        }
    }
    tfBuilder.setVlanList(vlans, fieldType);
}

void QoSManagementMenu::getIPAddressParamsFromUser(
    telux::data::TrafficFilterBuilder &tfBuilder, telux::data::FieldType fieldType) {
    int option;
    std::cout << " Select IP version [4 - IPv4, 6 - IPv6]: ";
    std::cin >> option;
    Utils::validateInput(option, {4, 6});
    if (option == 4) {
        std::string ipv4Addr = "", ipv4SubnetMask = "";
        char delimiter = '\n';
        std::cout << "Enter IPv4 address: ";
        std::getline(std::cin, ipv4Addr, delimiter);
        tfBuilder.setIPv4Address(ipv4Addr, fieldType);
    } else if (option == 6) {
        char delimiter = '\n';
        std::string ipv6Addr = "";
        std::cout << "Enter IPv6 address: ";
        std::getline(std::cin, ipv6Addr, delimiter);
        tfBuilder.setIPv6Address(ipv6Addr, fieldType);
    }
}

void QoSManagementMenu::getPortsFromUser(
    telux::data::TrafficFilterBuilder &tfBuilder, telux::data::FieldType fieldType) {
    int option = 0;
    std::cout << " [0 - Skip, 1 - Port, 2 - Port Range ]: ";
    std::cin >> option;
    Utils::validateInput(option, {0, 1, 2});

    if (option == 1) {
        std::cout << "Enter port: ";
        uint16_t port;
        std::cin >> port;
        Utils::validateInput(port);
        tfBuilder.setPort(port, fieldType);
    } else if (option == 2) {
        uint16_t startPort = 0, portRange = 0;
        std::cout << "Enter start port: ";
        std::cin >> startPort;
        Utils::validateInput(startPort);
        std::cout << "Enter port range: ";
        std::cin >> portRange;
        Utils::validateInput(portRange);
        tfBuilder.setPortRange(startPort, portRange, fieldType);
    }
}