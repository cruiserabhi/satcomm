/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to perform the network scan and
 * get a list of available networks. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a INetworkSelectionManager instance from the PhoneFactory.
 * 3. Wait for the network selection service to become available.
 * 4. Trigger a network scan.
 * 5. Wait for the scan to complete.
 * 6. Receive result of the network scan.
 *
 * Usage:
 * # ./network_scan_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/NetworkSelectionManager.hpp>

class Utils {
 public:
    void printOperatorName(telux::tel::OperatorInfo operatorInfo) {
        std::cout << "Operator name: " << operatorInfo.getName() <<
            "\nMcc: " << operatorInfo.getMcc() <<
            "\nMnc: " << operatorInfo.getMnc() << std::endl;
    }

    void printInUseStatus(telux::tel::InUseStatus status) {
        switch(status) {
            case telux::tel::InUseStatus::UNKNOWN:
                std::cout << "In-use status: UNKNOWN, ";
                break;
            case telux::tel::InUseStatus::CURRENT_SERVING:
                std::cout << "In-use status: CURRENT_SERVING, ";
                break;
            case telux::tel::InUseStatus::AVAILABLE:
                std::cout << "In-use status: AVAILABLE, ";
                break;
            default:
                break;
       }
    }

    void printRoamingStatus(telux::tel::RoamingStatus status) {
        switch(status) {
            case telux::tel::RoamingStatus::UNKNOWN:
                std::cout << "Roaming status: UNKNOWN, ";
                break;
            case telux::tel::RoamingStatus::HOME:
                std::cout << "Roaming status: HOME, ";
                break;
            case telux::tel::RoamingStatus::ROAM:
                std::cout << "Roaming status: ROAM, ";
                break;
            default:
                break;
       }
    }

    void printForbiddenStatus(telux::tel::ForbiddenStatus status) {
        switch(status) {
            case telux::tel::ForbiddenStatus::UNKNOWN:
                std::cout << "Forbidden status: UNKNOWN, ";
                break;
            case telux::tel::ForbiddenStatus::FORBIDDEN:
                std::cout << "Forbidden status: FORBIDDEN, ";
                break;
            case telux::tel::ForbiddenStatus::NOT_FORBIDDEN:
                std::cout << "Forbidden status: NOT_FORBIDDEN, ";
                break;
            default:
                break;
       }
    }

    void printPreferredStatus(telux::tel::PreferredStatus status) {
        switch(status) {
            case telux::tel::PreferredStatus::UNKNOWN:
                std::cout << "Preferred status: UNKNOWN" << std::endl;
                break;
            case telux::tel::PreferredStatus::PREFERRED:
                std::cout << "Preferred status: PREFERRED" << std::endl;
                break;
            case telux::tel::PreferredStatus::NOT_PREFERRED:
                std::cout << "Preferred status: NOT_PREFERRED" << std::endl;
                break;
            default:
                break;
       }
    }
};

class NetworkScanner : public std::enable_shared_from_this<NetworkScanner> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        networkMgr_ = phoneFactory.getNetworkSelectionManager(DEFAULT_SLOT_ID,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!networkMgr_) {
            std::cout << "Can't get INetworkSelectionManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Network selection service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int scanNetwork() {
        telux::common::Status status;

        auto responseCb = std::bind(&NetworkScanner::networkScanResultReceiver,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 4 */
        status = networkMgr_->performNetworkScan(responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't scan, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Scan initiated" << std::endl;
        return 0;
    }

    /* Step - 6 */
    void networkScanResultReceiver(
        std::vector<telux::tel::OperatorInfo> operatorsInfo,
        telux::common::ErrorCode error) {

        std::cout << "networkScanResultReceiver()" << std::endl;
        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to scan, err " << static_cast<int>(error) << std::endl;
            return;
        }

        for (auto opInfo : operatorsInfo) {
            utils_->printOperatorName(opInfo);
            utils_->printInUseStatus(opInfo.getStatus().inUse);
            utils_->printRoamingStatus(opInfo.getStatus().roaming);
            utils_->printForbiddenStatus(opInfo.getStatus().forbidden);
            utils_->printPreferredStatus(opInfo.getStatus().preferred);
        }
    }

 private:
    std::shared_ptr<Utils> utils_;
    std::shared_ptr<telux::tel::INetworkSelectionManager> networkMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<NetworkScanner> app;

    try {
        app = std::make_shared<NetworkScanner>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate NetworkScanner" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->scanNetwork();
    if (ret < 0) {
        return ret;
    }

    /* Step - 5 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::minutes(3));

    std::cout << "\nNetwork scanner app exiting" << std::endl;
    return 0;
}
