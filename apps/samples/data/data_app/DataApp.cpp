/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This application demonstrates how to make a data call. The steps are as follows:
 *
 *  1. Get a DataFactory instance.
 *  2. Get a IDataConnectionManager instance from DataFactory.
 *  3. Wait for the data service to become available.
 *  4. Register a listener which will receive updates whenever status of the call is changed.
 *  5. Define parameters for the call and place the data call.
 *  6. Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./data_app 1 1 0
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <list>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>

class DataConnection : public telux::data::IDataConnectionListener,
                       public std::enable_shared_from_this<DataConnection> {
 public:
    int init(SlotId slotId) {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataConMgr_ = dataFactory.getDataConnectionManager(slotId,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataConMgr_) {
            std::cout << "Can't get IDataConnectionManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        status = dataConMgr_->registerListener(shared_from_this());
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

        /* Step - 6 */
        status = dataConMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int makeDataCall(int profileId, telux::data::OperationType opType) {
        telux::common::Status status;

        auto responseCb = std::bind(&DataConnection::responseCallback,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 5 */
        status = dataConMgr_->startDataCall(profileId, telux::data::IpFamilyType::IPV4V6,
            responseCb, opType);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't start data call, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "\nData call initiated" << std::endl;
        return 0;
    }

    /* Receives response of the startDataCall() request */
    void responseCallback(
        const std::shared_ptr<telux::data::IDataCall> &dataCall,
        telux::common::ErrorCode error) {
        std::cout << "\nresponseCallback(), err " << static_cast<int>(error) << std::endl;
    }

    /* Receives data call information whenever there is a change */
    void onDataCallInfoChanged(
        const std::shared_ptr<telux::data::IDataCall> &dataCall) override {

        std::cout << "\nonDataCallInfoChanged()" << std::endl;
        std::list<telux::data::IpAddrInfo> ipAddrList;

        std::cout << "Data call details:" << std::endl;
        std::cout << " Slot ID: " << dataCall->getSlotId() << std::endl;
        std::cout << " Profile ID: " << dataCall->getProfileId() << std::endl;
        std::cout << " Interface name: " << dataCall->getInterfaceName() << std::endl;

        std::cout << " Data call status: " <<
            static_cast<int>(dataCall->getDataCallStatus()) << std::endl;
        std::cout << " Data call end reason, type : " <<
            static_cast<int>(dataCall->getDataCallEndReason().type) << std::endl;

        ipAddrList = dataCall->getIpAddressInfo();
        for(auto &it : ipAddrList) {
            std::cout << "\n ifAddress: " << it.ifAddress
                << "\n primaryDnsAddress: " << it.primaryDnsAddress
                << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << std::endl;
        }

        std::cout << " IP family type: " <<
            static_cast<int>(dataCall->getIpFamilyType()) << std::endl;
        std::cout << " Tech preference: " <<
            static_cast<int>(dataCall->getTechPreference()) << std::endl;
    }

 private:
    std::shared_ptr<telux::data::IDataConnectionManager> dataConMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DataConnection> app;

    if (argc != 4) {
        std::cout << "Usage: ./data_app <slot_id> <profile-id> <opType>" << std::endl;
        return -EINVAL;
    }

    SlotId slotId = static_cast<SlotId>(std::atoi(argv[1]));
    int profileId = std::atoi(argv[2]);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>
        (std::atoi(argv[3]));

    try {
        app = std::make_shared<DataConnection>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate DataConnection" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    ret = app->makeDataCall(profileId, opType);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Wait for receiving all asynchronous responses before exiting the application.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nData connection app exiting" << std::endl;
    return 0;
}
