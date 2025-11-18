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
 * This application demonstrates how to enable/disable Socks proxy service.
 * The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a ISocksManager instance from DataFactory.
 * 3. Wait for the Socks service to become available.
 * 4. Define whether to enable/disable Socks proxy service on local/remote system.
 * 5. Finally, enable/disable the Socks proxy service.
 *
 * Usage:
 * # ./socks_sample_app <operation-type> <enable>
 *
 * Example - ./socks_sample_app 1 1
 *
 * Please follow instructions in Readme file located in same folder as this app
 * to setup Socks Proxy before running this app.
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/SocksManager.hpp>

class SocksEnabler : public std::enable_shared_from_this<SocksEnabler> {
 public:
    int init(telux::data::OperationType opType) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataSocksMgr_ = dataFactory.getSocksManager(
                opType, [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataSocksMgr_) {
            std::cout << "Can't get ISocksManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Socks service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int enableSocks(bool enable) {
        telux::common::Status status;

        auto respCb = std::bind(
            &SocksEnabler::onSocksStatusAvailable, this, std::placeholders::_1);

        /* Step - 5 */
        enable_ = enable;
        status = dataSocksMgr_->enableSocks(enable, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't enable/disable Socks, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Requested Socks enablement" << std::endl;
        return 0;
    }

    /* Called as a response to enableSocks() request */
    void onSocksStatusAvailable(telux::common::ErrorCode error) {
        std::cout << "onSocksStatusAvailable()" << std::endl;
        bool state;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to enable/disable Socks, err" <<
                static_cast<int>(error) << std::endl;
            return;
        }

        state = enable_ ? "enabled" : "disabled";
        std::cout << "Socks " << state << " successfully" << std::endl;
    }

 private:
    bool enable_;
    std::shared_ptr<telux::data::net::ISocksManager> dataSocksMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<SocksEnabler> app;

    bool enable;
    telux::data::OperationType opType;

    if (argc != 3) {
        std::cout << "Usage: ./socks_sample_app <operation-type> <enable>" << std::endl;
        return -EINVAL;
    }

    /* Step - 4 */
    opType = static_cast<telux::data::OperationType>(std::atoi(argv[1]));
    enable = std::atoi(argv[2]);

    try {
        app = std::make_shared<SocksEnabler>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate SocksEnabler" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(opType);
    if (ret < 0) {
        return ret;
    }

    ret = app->enableSocks(enable);
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses before exiting the application.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\nSocks app exiting" << std::endl;
    return 0;
}
