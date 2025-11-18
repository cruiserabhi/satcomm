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
 * This application demonstrates how to get current dedicated radio bearer (DRB)
 * status and listens to the status change notifications. The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IServingSystemManager instance from DataFactory.
 * 3. Wait for the serving system service to become available.
 * 4. Register a listener which will be invoked if a change in DRB status is detected.
 * 5. Define slot ID to use.
 * 6. Get current DRB status.
 * 7. When the use case is over, deregister the listener.
 *
 * Usage:
 * # ./data_drb_status_app <slot-id>
 *
 * Example - ./data_drb_status_app 1
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

class DRBStatus : public telux::data::IServingSystemListener,
                  public std::enable_shared_from_this<DRBStatus> {
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

    int drbGetStatus() {
        telux::data::DrbStatus drbStatus;

        /* Step - 6 */
        drbStatus = dataServingSystemMgr_->getDrbStatus();

        switch(drbStatus) {
            case telux::data::DrbStatus::ACTIVE:
                std::cout << "DRB status - active" << std::endl;
                break;
            case telux::data::DrbStatus::DORMANT:
                std::cout << "DRB status - dormant" << std::endl;
                break;
            case telux::data::DrbStatus::UNKNOWN:
                std::cout << "DRB status - unknown" << std::endl;
                break;
            default:
                std::cout << "DRB status - invalid" << std::endl;
                break;
        }

        return 0;
    }

    /* Called if the DRB status is changed */
    void onDrbStatusChanged(telux::data::DrbStatus drbStatus) override {
        std::cout << "onDrbStatusChanged()" << std::endl;
        switch(drbStatus) {
            case telux::data::DrbStatus::ACTIVE:
                std::cout << "DRB status - active" << std::endl;
                break;
            case telux::data::DrbStatus::DORMANT:
                std::cout << "DRB status - dormant" << std::endl;
                break;
            case telux::data::DrbStatus::UNKNOWN:
                std::cout << "DRB status - unknown" << std::endl;
                break;
            default:
                std::cout << "DRB status - invalid" << std::endl;
                break;
        }
    }

 private:
    std::shared_ptr<telux::data::IServingSystemManager> dataServingSystemMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DRBStatus> app;

    SlotId slotId;

    if (argc != 2) {
        std::cout << "Usage: ./data_drb_status_app <slot-id>" << std::endl;
        return -EINVAL;
    }

    /* Step - 5 */
    slotId = static_cast<SlotId>(std::atoi(argv[1]));

    try {
        app = std::make_shared<DRBStatus>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate DRBStatus" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    ret = app->drbGetStatus();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving asynchronous response.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nDRB status app exiting" << std::endl;
    return 0;
}
