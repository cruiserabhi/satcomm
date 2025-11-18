/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * This application demonstrates how to set data stall parameters. The steps are as follows:
 *
 * 1. Get DataControlManager from DataFactory.
 * 2. Set data stall parameters.
 *
 * Usage:
 * # ./data_stall_app <SlotId (1 / 2)>
       <Direction (1: UPLINK / 2: DOWNLINK)>
 *     <ApplicationType (0-UNSPECIFIED, 1-CONV_AUDIO, 2-CONV_VIDEO,3-STREAMING_AUDIO,
 *     4-STREAMING_VIDEO, 5-TYPE_GAMING, 6-WEB_BROWSING, 7-FILE_TRANSFER)
 *     <DataStallStatus (0-False, 1-True)>
 *
 * Example:
 * # ./data_stall_app 1 1 3 1
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
#include <telux/data/DataControlManager.hpp>


class DataStallApp : public telux::data::IDataControlListener,
                      public std::enable_shared_from_this<DataStallApp> {
 public:
    int initDataControlManager() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        auto &dataFactory = telux::data::DataFactory::getInstance();

        dataControlMgr_  = dataFactory.getDataControlManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataControlMgr_) {
            std::cout << "Can't get IDataControlManager" << std::endl;
            return -ENOMEM;
        }

        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        status = dataControlMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int setdataStallParams(const SlotId &slotId, const telux::data::DataStallParams &params) {
        telux::common::ErrorCode errCode;

        errCode = dataControlMgr_->setDataStallParams(slotId, params);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set  data stall params, err " <<
                static_cast<int>(errCode) << std::endl;
            return -EIO;
        }

        std::cout << " set data stall params succeed" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        status = dataControlMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

 private:
    std::shared_ptr<telux::data::IDataControlManager> dataControlMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DataStallApp> app;

    SlotId slotId;
    telux::data::DataStallParams params;

    if (argc != 5) {
        std::cout << "./data_stall_app <SlotId> <Direction> <ApplicationType> <DataStallStatus>"
            << std::endl;
        return -EINVAL;
    }

    if ((std::atoi(argv[1]) != SlotId::SLOT_ID_1) || (std::atoi(argv[1]) != SlotId::SLOT_ID_2)) {
        std::cout << " Invalid slotId, valid values: 1/2" << std::endl;
        return -EINVAL;
    }
    slotId = static_cast<SlotId>(std::atoi(argv[1]));

    if ((std::atoi(argv[2]) != static_cast<int>(telux::data::Direction::UPLINK)) ||
        (std::atoi(argv[2]) != static_cast<int>(telux::data::Direction::DOWNLINK))) {
        std::cout << " Invalid direction, valid values: 1/2" << std::endl;
        return -EINVAL;
    }
    params.trafficDir = static_cast<telux::data::Direction>(std::atoi(argv[2]));

    if ((std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::UNSPECIFIED))      ||
        (std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::CONV_AUDIO))       ||
        (std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::CONV_VIDEO))       ||
        (std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::STREAMING_AUDIO))  ||
        (std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::STREAMING_VIDEO))  ||
        (std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::TYPE_GAMING))      ||
        (std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::WEB_BROWSING))     ||
        (std::atoi(argv[3]) != static_cast<int>(telux::data::ApplicationType::FILE_TRANSFER))) {
        std::cout << " Invalid application, valid values: 0/1/2/3/4/5/6/7" << std::endl;
        return -EINVAL;
    }
    params.appType = static_cast<telux::data::ApplicationType>(std::atoi(argv[3]));

    if ((std::atoi(argv[4]) != 0) || (std::atoi(argv[4]) != 1)) {
        std::cout << " Invalid data stall status, valid values: 0/1" << std::endl;
        return -EINVAL;
    }
    params.dataStall = static_cast<bool>(std::atoi(argv[4]));

    try {
        app = std::make_shared<DataStallApp>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate DataStallApp" << std::endl;
        return -ENOMEM;
    }

        /** Step - 1 */
        ret = app->initDataControlManager();
        if (ret < 0) {
            return ret;
        }

        /** Step - 2 */
        ret = app->setdataStallParams(slotId, params);
        if (ret < 0) {
            return ret;
        }

        /** Step - 3 */
        ret = app->deinit();
        if (ret < 0) {
            return ret;
        }

    std::cout << "\nData-Stall app exiting" << std::endl;
    return 0;
}
