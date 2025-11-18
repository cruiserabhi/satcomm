/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * This application demonstrates how to get current roaming status and listen
 * to the roaming status change notifications. The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IServingSystemManager instance from DataFactory.
 * 3. Wait for the serving system service to become available.
 * 4. Register a listener which will be invoked when roaming status changes.
 * 5. Define slot ID to use.
 * 6. Get current roaming status.
 * 7. Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./data_roaming_status_app <slot-id>
 *
 * Example - ./data_roaming_status_app 1
 *
 * <slot id> : 1 for default slot, 2 for second slot
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
#include <telux/data/ServingSystemManager.hpp>

class RoamingStatus : public telux::data::IServingSystemListener,
                      public std::enable_shared_from_this<RoamingStatus> {
 public:
    int init(SlotId slotId) {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataServingSystemMgr_ = dataFactory.getServingSystemManager(
                slotId, [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataServingSystemMgr_) {
            std::cout << "Can't get IServingSystemManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Serving system service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        status = dataServingSystemMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 7 */
        status = dataServingSystemMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int getRoamingStatus() {
        telux::common::Status status;

        auto respCb = std::bind(&RoamingStatus::onRoamingStatusAvailable,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 6 */
        status = dataServingSystemMgr_->requestRoamingStatus(respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request roaming status, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Roaming status requested" << std::endl;
        return 0;
    }

    /* Called as a response to requestRoamingStatus() request */
    void onRoamingStatusAvailable(telux::data::RoamingStatus roamingStatus,
            telux::common::ErrorCode error) {
        std::cout << "\nonRoamingStatusAvailable()" << std::endl;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to get roaming status, err" <<
                static_cast<int>(error) << std::endl;
            return;
        }

        logRoamingStatusDetails(roamingStatus);
    }

    /* Called whenever roaming status changes */
    void onRoamingStatusChanged(telux::data::RoamingStatus roamingStatus) override {
        std::cout << "onRoamingStatusChanged()" << std::endl;

        logRoamingStatusDetails(roamingStatus);
    }

 private:
    std::shared_ptr<telux::data::IServingSystemManager> dataServingSystemMgr_;

    void logRoamingStatusDetails(const telux::data::RoamingStatus& status) {
        std::cout << " ** Roaming Status Details **\n";
        bool isRoaming = status.isRoaming;
        if(isRoaming) {
            std::cout << "System is in Roaming State" << std::endl;
            std::cout << "Roaming Type: ";
            switch(status.type)  {
                case telux::data::RoamingType::INTERNATIONAL:
                    std::cout << "International" << std::endl;
                break;
                case telux::data::RoamingType::DOMESTIC:
                    std::cout << "Domestic" << std::endl;
                break;
                default:
                    std::cout << "Unknown" << std::endl;
            }
        } else {
            std::cout << "System is not in Roaming State" << std::endl;
        }
    }
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<RoamingStatus> app;

    SlotId slotId;

    if (argc != 2) {
        std::cout << "Usage: ./data_roaming_status_app <slot-id>" << std::endl;
        return -EINVAL;
    }

    /* Step - 5 */
    slotId = static_cast<SlotId>(std::atoi(argv[1]));

    try {
        app = std::make_shared<RoamingStatus>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate RoamingStatus" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    ret = app->getRoamingStatus();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses before exiting the application.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\nData roaming status app exiting" << std::endl;
    return 0;
}
