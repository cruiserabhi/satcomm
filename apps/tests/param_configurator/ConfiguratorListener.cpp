/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ConfiguratorListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void ConfigListener::onConfigUpdate(std::string key, std::string value) {
    PRINT_NOTIFICATION << "\n*********** CONFIGURATION UPDATE *********************\n";
    std::cout << "Updated Key: " << key << " New Value: " << value << "\n";
}

void ConfigListener::onServiceStatusChange(ServiceStatus status) {
    PRINT_NOTIFICATION << "\n*********** SERVICE STATUS UPDATE *********************\n";
    switch(status) {
        case ServiceStatus::SERVICE_UNAVAILABLE : std::cout << "Service Unavailable \n";
                                                  break;
        case ServiceStatus::SERVICE_AVAILABLE   : std::cout << "Service Available \n";
                                                  break;
        case ServiceStatus::SERVICE_FAILED      : std::cout << "Service Failed \n";
                                                  break;
    }
}