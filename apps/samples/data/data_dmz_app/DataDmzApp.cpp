/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to enable demilitarized zone (DMZ).
 * The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IFirewallManager instance from DataFactory.
 * 3. Wait for the firewall service to become available.
 * 4. Define parameters, modem's profile ID, slot ID, IPv4 address and local/remote system.
 * 5. Enable DMZ with parameters from step 4.
 * 6. Receive asynchronous response of the DMZ enable request.
 * 7. When the use case is over, disable the DMZ.
 * 8. Receive asynchronous response of the DMZ disable request.
 *
 * Usage:
 * # ./dmz_sample_app <operation-type> <slot-id> <profile-id> <ip-address>
 *
 * Example - ./dmz_sample_app 0 1 5 192.168.225.22
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/FirewallManager.hpp>

class DMZEnabler : public std::enable_shared_from_this<DMZEnabler> {
 public:
    int init(telux::data::OperationType operationType) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataFwMgr_ = dataFactory.getFirewallManager(operationType,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataFwMgr_) {
            std::cout << "Can't get IFirewallManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Firewall service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int enableDMZ(telux::data::net::DmzConfig config) {
        telux::common::Status status;

        auto responseCb = std::bind(
            &DMZEnabler::enableDMZResponseCb, this, std::placeholders::_1);

        /* Step - 5 */
        status = dataFwMgr_->enableDmz(config, responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't enable DMZ, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "\nRequested DMZ enablement" << std::endl;
        return 0;
    }

    int disableDMZ(telux::data::BackhaulInfo bhInfo, telux::data::IpFamilyType ipType) {
        telux::common::Status status;

        auto responseCb = std::bind(
            &DMZEnabler::disableDMZResponseCb, this, std::placeholders::_1);

        /* Step - 7 */
        status = dataFwMgr_->disableDmz(bhInfo, ipType, responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't disable DMZ, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "\nRequested DMZ disablement" << std::endl;
        return 0;
    }

    /* Step - 6 */
    /* Receives response of the enableDmz() request */
    void enableDMZResponseCb(telux::common::ErrorCode error) {
        std::cout << "\nenableDMZResponseCb()" << std::endl;
        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to enable DMZ, err " <<
                static_cast<int>(error) << std::endl;
        }
        std::cout << "DMZ enabled" << std::endl;
    }

    /* Step - 8 */
    /* Receives response of the disableDmz() request */
    void disableDMZResponseCb(telux::common::ErrorCode error) {
        std::cout << "\ndisableDMZResponseCb()" << std::endl;
        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to disable DMZ, err " <<
                static_cast<int>(error) << std::endl;
        }
        std::cout << "DMZ disabled" << std::endl;
    }

 private:
    std::shared_ptr<telux::data::net::IFirewallManager> dataFwMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DMZEnabler> app;

    std::string ipAddress;
    telux::data::OperationType operationType;
    telux::data::BackhaulInfo bhInfo = {};

    if (argc != 5) {
        std::cout <<
            "Usage: ./dmz_sample_app <operation-type> <slot-id> <profile-id> <ip-address>"
            << std::endl;
        return -EINVAL;
    }

    /* Step - 4 */
    operationType = static_cast<telux::data::OperationType>(std::atoi(argv[1]));
    bhInfo.slotId = static_cast<SlotId>(std::atoi(argv[2]));
    bhInfo.backhaul = telux::data::BackhaulType::WWAN;
    bhInfo.profileId = std::atoi(argv[3]);
    ipAddress = static_cast<std::string>(argv[4]);

    try {
        app = std::make_shared<DMZEnabler>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate DMZEnabler" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(operationType);
    if (ret < 0) {
        return ret;
    }

    telux::data::net::DmzConfig config;
    config.bhInfo = bhInfo;
    config.ipAddr = ipAddress;
    ret = app->enableDMZ(config);
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving asynchronous response.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    ret = app->disableDMZ(bhInfo, telux::data::IpFamilyType::IPV4);
    if (ret < 0) {
        return ret;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\nDMZ enable/disable app exiting" << std::endl;
    return 0;
}
