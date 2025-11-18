/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "DataLinkListener.hpp"
#include "../DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

DataLinkListener::DataLinkListener(){
    std::cout << "DataLinkListener constructed" << std::endl;
}

DataLinkListener::~DataLinkListener(){
    std::cout << "DataLinkListener destructed" << std::endl;
}

void DataLinkListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {

    std::string stat ="";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }

    PRINT_NOTIFICATION <<
        " ** Data Link onServiceStatusChange **\n" << stat << std::endl;
}

void DataLinkListener::onEthDataLinkStateChange(telux::data::LinkState linkState) {
    PRINT_NOTIFICATION <<
        " ** Data Link State Change to  **\n" << (linkState == telux::data::LinkState::UP ? "UP"
                : "DOWN") << std::endl;
}

std::string DataLinkListener::ethModeTypeToString(telux::data::EthModeType ethModeType) {
    std::string mode ="";
    switch(ethModeType) {
        case telux::data::EthModeType::ETHMODE_USXGMII_10G:
            mode = " USXGMII_10G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_5G:
            mode = " USXGMII_5G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_2_5G:
            mode = " USXGMII_2_5G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_1G:
            mode = " USXGMII_1G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_100M:
            mode = " USXGMII_100M";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_10M:
            mode = " USXGMII_10M";
            break;
        case telux::data::EthModeType::ETHMODE_SGMII_2_5G:
            mode = " SGMII_2_5G";
            break;
        case telux::data::EthModeType::ETHMODE_SGMII_1G:
            mode = " SGMII_1G";
            break;
        case telux::data::EthModeType::ETHMODE_SGMII_100M:
            mode = " SGMII_100M";
            break;
        default:
            mode = " Unknown ETH mode";
            break;
    }
    return mode;
}



std::string DataLinkListener::linkModeChangeStatusToString(telux::data::LinkModeChangeStatus status) {
    std::string stat ="";
    switch(status) {
        case telux::data::LinkModeChangeStatus::ACCEPTED:
            stat = " ACCEPTED";
            break;
        case telux::data::LinkModeChangeStatus::COMPLETED:
            stat = " COMPLETED";
            break;
        case telux::data::LinkModeChangeStatus::FAILED:
            stat = " FAILED";
            break;
        case telux::data::LinkModeChangeStatus::REJECTED:
            stat = " REJECTED";
            break;
        case telux::data::LinkModeChangeStatus::TIMEOUT:
            stat = " TIMEOUT";
            break;

        default:
            stat = " Unknown ETH status";
            break;
    }
    return stat;
}

void DataLinkListener::onEthModeChangeRequest(telux::data::EthModeType ethModeType) {
    PRINT_NOTIFICATION <<
        " ** Data Link onEthModeChangeRequest **\n" <<
        ethModeTypeToString(ethModeType) << std::endl;
}

void DataLinkListener::onEthModeChangeTransactionStatus(telux::data::EthModeType ethModeType,
    telux::data::LinkModeChangeStatus status) {
    std::string stat = linkModeChangeStatusToString(status);

    PRINT_NOTIFICATION << " ** Data Link onEthModeChangeTransactionStatus **\n"
                       << ethModeTypeToString(ethModeType)
                       << " ,status : " << static_cast<int>(status) << " "
                       << stat << std::endl;
}
