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
 * This application demonstrates how to get current serving network status and listen
 * to the network status status change notifications. The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IServingSystemManager instance from DataFactory.
 * 3. Wait for the serving system service to become available.
 * 4. Register a listener which will be invoked when service status changes.
 * 5. Define slot ID to use.
 * 6. Get the current serving network status.
 * 7. Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./data_service_status_app <slot-id>
 *
 * Example - ./data_service_status_app 1
 *
 * <slot id> : 1 for default slot, 2 for second slot
 */

#include <errno.h>
#include <iomanip>
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

class ServingNetworkStatus : public telux::data::IServingSystemListener,
                             public std::enable_shared_from_this<ServingNetworkStatus> {
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
        status = dataServingSystemMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Serving system service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
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

    int getServingNetworkStatus() {
        telux::common::Status status;

        auto respCb = std::bind(&ServingNetworkStatus::onNetworkStatusAvailable,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 6 */
        status = dataServingSystemMgr_->requestServiceStatus(respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request roaming status, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Service status requested" << std::endl;
        return 0;
    }

    /* Called as a response to requestServiceStatus() request */
    void onNetworkStatusAvailable(telux::data::ServiceStatus serviceStatus,
            telux::common::ErrorCode error) {
        std::cout << "\nonNetworkStatusAvailable()" << std::endl;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to get servie status, err" <<
                static_cast<int>(error) << std::endl;
            return;
        }

        printDetails(serviceStatus);
    }

    /* Called whenever service status changes */
    void onServiceStateChanged(telux::data::ServiceStatus serviceStatus) override {
        std::cout << "onServiceStateChanged()" << std::endl;
        printDetails(serviceStatus);
    }

    void onLteAttachFailure(const telux::data::LteAttachFailureInfo info) override {
        std::cout << " rejectReason.type " << static_cast<int>(info.rejectReason.type) <<
            ", rejectReason.code " << static_cast<int>(info.rejectReason.IpCode) << std::endl;
        std::cout << " PLMN:";
        for (unsigned int i = 0; i < info.plmnId.size(); ++i) {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << info.plmnId[i];
        }
        std::cout << std::endl;

        if (info.primaryPlmnId.size()) {
            std::cout << " Primary PLMN:";
            for (unsigned int i = 0; i < info.primaryPlmnId.size(); ++i) {
                std::cout << std::setfill('0') << std::setw(2) << std::hex << info.primaryPlmnId[i];
            }
            std::cout << std::endl;
        }
    }

 private:
    void printDetails(telux::data::ServiceStatus &serviceStatus) {
        if (serviceStatus.serviceState == telux::data::DataServiceState::OUT_OF_SERVICE) {
            std::cout << "Currently out of service" << std::endl;
            return;
        }

        std::cout << "Current network: ";
        switch (serviceStatus.networkRat) {
            case telux::data::NetworkRat::CDMA_1X:
                std::cout << "CDMA 1X" << std::endl;
                break;
            case telux::data::NetworkRat::CDMA_EVDO:
                std::cout << "CDMA EVDO" << std::endl;
                break;
            case telux::data::NetworkRat::GSM:
                std::cout << "GSM" << std::endl;
                break;
            case telux::data::NetworkRat::WCDMA:
                std::cout << "WCDMA" << std::endl;
                break;
            case telux::data::NetworkRat::LTE:
                std::cout << "LTE" << std::endl;
                break;
            case telux::data::NetworkRat::TDSCDMA:
                std::cout << "TDSCDMA" << std::endl;
                break;
            case telux::data::NetworkRat::NR5G:
                std::cout << "NR5G" << std::endl;
                break;
            default:
                std::cout << "UNKNOWN" << std::endl;
                break;
        }
    }

    std::shared_ptr<telux::data::IServingSystemManager> dataServingSystemMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ServingNetworkStatus> app;

    SlotId slotId;

    if (argc != 2) {
        std::cout << "Usage: ./data_service_status_app <slot-id>" << std::endl;
        return -EINVAL;
    }

    /* Step - 5 */
    slotId = static_cast<SlotId>(std::atoi(argv[1]));

    try {
        app = std::make_shared<ServingNetworkStatus>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate ServingNetworkStatus" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    ret = app->getServingNetworkStatus();
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

    std::cout << "\nData service status app exiting" << std::endl;
    return 0;
}
