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
 * This application demonstrates how to make a data call and configure firewall.
 * The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IFirewallManager instance from DataFactory.
 * 3. Wait for the firewall service to become available.
 * 4. Define the parameters for the firewall configuration.
 * 5. Configure the firewall with parameters from step 4.
 *
 * Usage:
 * # ./fwl_enable_sample_app <op-type> <slot-id> <profile-id> <enable-firewall> <allow-packets>
 *
 * Example: ./fwl_enable_sample_app 1 1 5 1 1
 *
 * This start a data call with profile ID as 5, enables firewall, allow packets,
 * on the remote host using default slot.
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/FirewallManager.hpp>

class FirewallConfigurator : public std::enable_shared_from_this<FirewallConfigurator> {
 public:
    int init(telux::data::OperationType opType) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataFwMgr_ = dataFactory.getFirewallManager(opType,
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

    int updateFirewallConfiguration(telux::data::net::FirewallConfig& fwConfig) {
        telux::common::Status status;

        auto respCb = std::bind(
            &FirewallConfigurator::fwConfigUpdateResponse, this, std::placeholders::_1);

        /* Step - 5 */
        status = dataFwMgr_->setFirewallConfig(fwConfig, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't update configuration, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Requested firewall set" << std::endl;
        return 0;
    }

    /* Receives response of the setFirewallConfig() request */
    void fwConfigUpdateResponse(telux::common::ErrorCode error) {
        std::cout << "\nfwConfigUpdateResponse(), err " <<
            static_cast<int>(error) << std::endl;
    }

 private:
    std::shared_ptr<telux::data::net::IFirewallManager> dataFwMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<FirewallConfigurator> app;

    bool enable;
    bool allowPackets;
    telux::data::OperationType opType;
    telux::data::BackhaulInfo bhInfo = {};

    if (argc != 6) {
        std::cout << "Usage: ./fwl_enable_sample_app <op-type> <slot-id> " <<
        "<profile-id> <enable-firewall> <allow-packets>" << std::endl;
        return -EINVAL;
    }

    /* Step - 4 */
    opType = static_cast<telux::data::OperationType>(std::atoi(argv[1]));
    bhInfo.slotId = static_cast<SlotId>(std::atoi(argv[2]));
    bhInfo.backhaul = telux::data::BackhaulType::WWAN;
    bhInfo.profileId = std::atoi(argv[3]);;
    enable = std::atoi(argv[4]);
    allowPackets = std::atoi(argv[5]);

    try {
        app = std::make_shared<FirewallConfigurator>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate FirewallConfigurator" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(opType);
    if (ret < 0) {
        return ret;
    }

    telux::data::net::FirewallConfig fwConfig = {};
    fwConfig.bhInfo = bhInfo;
    fwConfig.enable = enable;
    fwConfig.allowPackets = allowPackets;
    ret = app->updateFirewallConfiguration(fwConfig);
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "\nFirewall configurator app exiting" << std::endl;
    return 0;
}
