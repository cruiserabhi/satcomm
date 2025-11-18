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
 * This application demonstrates how to enable/disable L2TP un-managed tunnels
 * and add new tunnel. The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IL2tpManager instance from DataFactory.
 * 3. Wait for the L2TP service to become available.
 * 4. Enable L2TP for unmanaged Tunnel State.
 * 5. Set L2TP Configuration for one tunnel.
 *
 * Usage:
 * # ./l2tp_sample_app <configuration-file>
 *
 * Example: ./l2tp_sample_app /etc/DataL2tpApp.conf
 *
 * Please follow instructions in Readme file placed in same folder as this app to
 * setup L2TP mode and associated VLAN before running this application.
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/L2tpManager.hpp>

#include "ConfigParser.hpp"

/* Utility class to parse <configuration-file> and populate parameters */
class Utils {
 public:
    Utils(std::string configFile) {
        configParser_ = std::make_shared<ConfigParser>(configFile);
    };

    bool getL2TPEnable() {
        int val = std::atoi(configParser_->getValue(std::string("L2TP_ENABLE")).c_str());
        return ((val == 0) ? false : true);
    }

    bool getMSSEnable() {
        int val = std::atoi(configParser_->getValue(std::string("TCP_MSS_ENABLE")).c_str());
        return ((val == 0) ? false : true);
    }

    bool getMTUEnable() {
        int val = std::atoi(configParser_->getValue(std::string("MTU_SIZE_ENABLE")).c_str());
        return ((val == 0) ? false : true);
    }

    int getMtuSize() {
        return std::atoi(configParser_->getValue(std::string("MTU_SIZE_BYTES")).c_str());
    }

    void populateTunnelConfig(telux::data::net::L2tpTunnelConfig l2tpTunnelConfig) {
        l2tpTunnelConfig.locIface = configParser_->getValue(std::string("HW_IF_NAME"));
        l2tpTunnelConfig.prot = static_cast<telux::data::net::L2tpProtocol>(std::atoi(
            configParser_->getValue(std::string("ENCAP_PROTOCOL")).c_str()));
        l2tpTunnelConfig.locId = std::atoi(
            configParser_->getValue(std::string("LOCAL_TUNNEL_ID")).c_str());
        l2tpTunnelConfig.peerId = std::atoi(
            configParser_->getValue(std::string("PEER_TUNNEL_ID")).c_str());
        l2tpTunnelConfig.localUdpPort = std::atoi(
            configParser_->getValue(std::string("LOCAL_UDP_PORT")).c_str());
        l2tpTunnelConfig.peerUdpPort = std::atoi(
            configParser_->getValue(std::string("PEER_UDP_PORT")).c_str());
        l2tpTunnelConfig.ipType =  static_cast<telux::data::IpFamilyType>(std::atoi(
            configParser_->getValue(std::string("PEER_IP_FAMILY")).c_str()));
        l2tpTunnelConfig.peerIpv6Addr =  configParser_->getValue(std::string("PEER_IP_ADDRESS"));
    }

 private:
    std::shared_ptr<ConfigParser> configParser_;
};

class DataL2TP : public std::enable_shared_from_this<DataL2TP> {
 public:
    DataL2TP(std::shared_ptr<Utils> utils) {
        utils_ = utils;
    }

    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataL2TPMgr_  = dataFactory.getL2tpManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataL2TPMgr_) {
            std::cout << "Can't get IL2tpManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "L2TP service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int setL2TPConfiguration() {
        int mtuSize;
        bool enableL2tp;
        bool enableMss;
        bool enableMtu;
        telux::common::Status status;

        enableL2tp = utils_->getL2TPEnable();
        enableMss = utils_->getMSSEnable();
        enableMtu = utils_->getMTUEnable();
        mtuSize = utils_->getMtuSize();

        auto responseCb = std::bind(
            &DataL2TP::onConfigResponseAvailable, this, std::placeholders::_1);

        /* Step - 4 */
        status = dataL2TPMgr_->setConfig(
            enableL2tp, enableMss, enableMtu, responseCb, mtuSize);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't set config, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForResponse()) {
            std::cout << "Failed to set config, err " <<
                static_cast<int>(errorCode_) << std::endl;
            return -EIO;
        }

        std::cout << "Configuration set" << std::endl;
        return 0;
    }

    int configureAndAddTunnel() {
        telux::common::Status status;
        telux::data::net::L2tpTunnelConfig l2tpTunnelConfig{};
        telux::data::net::L2tpSessionConfig l2tpSessionConfig{};

        utils_->populateTunnelConfig(l2tpTunnelConfig);

        l2tpSessionConfig.locId = 1;
        l2tpSessionConfig.peerId = 1;
        l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig);

        auto responseCb = std::bind(
            &DataL2TP::onAddTunnelResponseAvailable, this, std::placeholders::_1);

        /* Step - 5 */
        status = dataL2TPMgr_->addTunnel(l2tpTunnelConfig, responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't add tunnel, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForResponse()) {
            std::cout << "Failed to add tunnel, err " <<
                static_cast<int>(errorCode_) << std::endl;
            return -EIO;
        }

        std::cout << "Tunnel added" << std::endl;
        return 0;
    }

    bool waitForResponse() {
        int const DEFAULT_TIMEOUT_SECONDS = 5;
        std::unique_lock<std::mutex> lock(updateMutex_);

        auto cvStatus = updateCV_.wait_for(lock,
            std::chrono::seconds(DEFAULT_TIMEOUT_SECONDS));

        if (cvStatus == std::cv_status::timeout) {
            std::cout << "Timedout" << std::endl;
            errorCode_ = telux::common::ErrorCode::TIMEOUT_ERROR;
            return false;
        }

        return true;
    }

    /* Receives response of the setConfig() request */
    void onConfigResponseAvailable(telux::common::ErrorCode error) {
        std::lock_guard<std::mutex> lock(updateMutex_);
        std::cout << "\nonConfigResponseAvailable()" << std::endl;
        errorCode_ = error;
        updateCV_.notify_one();
    }

    /* Receives response of the addTunnel() request */
    void onAddTunnelResponseAvailable(telux::common::ErrorCode error) {
        std::lock_guard<std::mutex> lock(updateMutex_);
        std::cout << "\nonAddTunnelResponseAvailable()" << std::endl;
        errorCode_ = error;
        updateCV_.notify_one();
    }

 private:
    std::mutex updateMutex_;
    std::condition_variable updateCV_;
    telux::common::ErrorCode errorCode_;
    std::shared_ptr<Utils> utils_;
    std::shared_ptr<telux::data::net::IL2tpManager> dataL2TPMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DataL2TP> app;

    std::shared_ptr<Utils> utils;

    if (argc != 2) {
        std::cout << "Usage: ./l2tp_sample_app <configuration-file>" << std::endl;
        return -EINVAL;
    }

    try {
        utils = std::make_shared<Utils>(argv[1]);
        app = std::make_shared<DataL2TP>(utils);
    } catch (const std::exception& e) {
        std::cout << "Can't allocate Utils/DataL2TP" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->setL2TPConfiguration();
    if (ret < 0) {
        return ret;
    }

    ret = app->configureAndAddTunnel();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nL2TP tunnel app exiting" << std::endl;
    return 0;
}
