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
 * This application demonstrates how to create a static NAT. The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a INatManager instance from DataFactory.
 * 3. Wait for the NAT service to become available.
 * 4. Define parameters for the adding NAT.
 * 5. Create NAT with given parameters.
 *
 * Usage:
 * # ./snat_sample_app <operation-type> <backhaul-type> <profile-id> <ip-address> \
 *      <protocol> <local-ip-port> <global-ip-port> 
 *
 * Example - ./snat_sample_app 1 3 5 192.168.225.22 6 500 500
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/NatManager.hpp>

class NATCreator : public std::enable_shared_from_this<NATCreator> {
 public:
    int init(telux::data::OperationType opType) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataSnatMgr_ = dataFactory.getNatManager(
                opType, [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataSnatMgr_) {
            std::cout << "Can't get INatManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "NAT service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int addNATEntry(int backhaulType, int profileId, std::string ipAddress, int localIpPort,
            int globalIpPort, int proto) {

        telux::common::Status status;
        telux::data::net::NatConfig natConfig{};

        natConfig.addr = ipAddress;
        natConfig.port = static_cast<uint16_t>(localIpPort);
        natConfig.globalPort = static_cast<uint16_t>(globalIpPort);
        natConfig.proto = static_cast<uint8_t>(proto);

        auto respCb = std::bind(
            &NATCreator::onAddNATStatusAvailable, this, std::placeholders::_1);

        telux::data::BackhaulInfo bhInfo{};
        bhInfo.backhaul = static_cast<telux::data::BackhaulType>(backhaulType);;
        bhInfo.slotId = DEFAULT_SLOT_ID;
        bhInfo.profileId = profileId;

        /* Step - 5 */
        status = dataSnatMgr_->addStaticNatEntry(bhInfo, natConfig, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request add nat, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Requested NAT addition" << std::endl;
        return 0;
    }

    /* Called as a response to addStaticNatEntry() request */
    void onAddNATStatusAvailable(telux::common::ErrorCode error) {
        std::cout << "onAddNATStatusAvailable()" << std::endl;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to add nat, err " <<
                static_cast<int>(error) << std::endl;
            return;
        }

        std::cout << "NAT added successfully" << std::endl;
    }

 private:
    std::shared_ptr<telux::data::net::INatManager> dataSnatMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<NATCreator> app;

    int backhaulType;
    int profileId;
    int localIpPort  = 0;
    int globalIpPort = 0;
    int proto;
    std::string ipAddress;
    telux::data::OperationType opType;


    if ((argc != 6 && argc != 8) ||
        (argc == 6 && (std::atoi(argv[5]) == 6 || std::atoi(argv[5]) == 17))) {
        /* 6-TCP, 17-UDP */
        std::cout << "Usage: ./snat_sample_app <operation-type> <backhaul-type> " <<
        "<profile-id>  <ip-address> <protocol> <local-ip-port> <global-ip-port> \n" <<
        "Note: local-ip-port and global-ip-port are ignored for protocol type " <<
        "ICMP, IGMP and ESP, so it can be skipped for these protocols"<< std::endl;
        return -EINVAL;
    }

    /* Step - 4 */
    opType = static_cast<telux::data::OperationType>(std::atoi(argv[1]));
    /* 0-ETH , 2-WLAN, 3-WWAN */
    backhaulType = std::atoi(argv[2]);

    profileId = std::atoi(argv[3]);
    ipAddress = static_cast<std::string>(argv[4]);

    /* 1-ICMP, 2-IGMP, 6-TCP, 17-UDP, 50-ESP */
    proto = std::atoi(argv[5]);

    if (argc == 8) {
        localIpPort = std::atoi(argv[6]);
        globalIpPort = std::atoi(argv[7]);
    }

    try {
        app = std::make_shared<NATCreator>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate NATCreator" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(opType);
    if (ret < 0) {
        return ret;
    }

    ret = app->addNATEntry(backhaulType, profileId, ipAddress, localIpPort, globalIpPort, proto);
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses before exiting the application.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\nNAT create app exiting" << std::endl;
    return 0;
}
