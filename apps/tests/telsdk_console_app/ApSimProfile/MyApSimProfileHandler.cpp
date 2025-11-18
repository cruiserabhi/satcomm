/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "iostream"
#include "MyApSimProfileHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyApSimProfileCallback::onResponseCallback(telux::common::ErrorCode error) {
    std::cout << std::endl;
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request failed with errorCode: " << static_cast<int>(error)
                 << " Description : " << Utils::getErrorCodeAsString(error)<< std::endl;
    } else {
        PRINT_CB << "Request processed successfully \n";
    }
}

