/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * This application demonstrates how to set Ethernet data link state. The steps are as follows:
 *
 * 1. Get DataLinkManager from DataFactory.
 * 2. Set Ethernet data link state.
 *
 * Usage:
 * # ./eth_init_app <LinkState (1: UP / 2: DOWN)>
 *
 * Example:
 * # ./eth_init_app 1
 */

#include <errno.h>

#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataLinkManager.hpp>


class SetEthDataLinkStateApp : public telux::data::IDataLinkListener,
                      public std::enable_shared_from_this<SetEthDataLinkStateApp> {
 public:
    int initDataLinkManager() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        auto &dataFactory = telux::data::DataFactory::getInstance();

        dataLinkMgr_  = dataFactory.getDataLinkManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataLinkMgr_) {
            std::cout << "Can't get IDataLinkManager" << std::endl;
            return -ENOMEM;
        }

        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        status = dataLinkMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int setEthDataLinkState(telux::data::LinkState ethLinkState) {
        telux::common::ErrorCode errCode;

        errCode = dataLinkMgr_->setEthDataLinkState(ethLinkState);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set  eth link state, err " <<
                static_cast<int>(errCode) << std::endl;
            return -EIO;
        }

        std::cout << " set eth link state succeed" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        status = dataLinkMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

 private:
    std::shared_ptr<telux::data::IDataLinkManager> dataLinkMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<SetEthDataLinkStateApp> app;

    telux::data::LinkState linkState;

    if (argc != 2) {
        std::cout << "./eth_init_app 1" << std::endl;
        return -EINVAL;
    }

    int ethState = std::atoi(argv[1]);

    if ( (ethState != 1) || (ethState != 2) ) {
        std::cout << " Invalid input, valid values: 1/2" << std::endl;
        return -EINVAL;
    }

    if (ethState == 1) {
        linkState = telux::data::LinkState::UP;
    } else if (ethState == 2) {
        linkState = telux::data::LinkState::DOWN;
    } else {
        std::cout << " Invalid input, valid values: 1/2" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<SetEthDataLinkStateApp>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate SetEthDataLinkStateApp" << std::endl;
        return -ENOMEM;
    }

        /** Step - 1 */
        ret = app->initDataLinkManager();
        if (ret < 0) {
            return ret;
        }

        /** Step - 2 */
        ret = app->setEthDataLinkState(linkState);
        if (ret < 0) {
            return ret;
        }

        /** Step - 3 */
        ret = app->deinit();
        if (ret < 0) {
            return ret;
        }

    std::cout << "\nEth-init app exiting" << std::endl;
    return 0;
}
