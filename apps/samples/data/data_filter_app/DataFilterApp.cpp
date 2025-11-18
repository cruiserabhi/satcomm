/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * This application demonstrates how to get make a data call and install filter.
 * The steps are as follows:
 *
 *  1. Get a DataFactory instance.
 *  2. Get a IDataConnectionManager instance from DataFactory.
 *  3. Wait for the data connection service to become available.
 *  4. Register listener that will receive updates whenever a new data call
 *    is established.
 *  5. Get a IDataFilterManager instance from DataFactory.
 *  6. Wait for the filter service to become available.
 *  7. Register listener that will receive powersave filtering mode notifications.
 *  8. Get IIpFilter instance based on the IP protocol.
 *  9. Bring up a data call connection based on specified profile identifier,
 *     IP family type, and operation type (local/remote).
 * 10. Disable the auto exit feature enable data filter mode.
 * 11. Set the UDP header info.
 * 12. Add a filter rule for all the active data calls.
 * 13. Finally, deregister all listeners when the use case is over.
 *
 * Usage:
 * # ./data_filter_app <profile-id> <ip-address> <port>
 *
 * Example - ./data_filter_app 1 158.2.3.4 8000
 *
 * This start a data call with profile ID as 1, installs filter for Incoming
 * packet matching IP 158.2.3.4 and port 8000.
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
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataFilterManager.hpp>

class DataFilter : public telux::data::IDataConnectionListener,
                   public telux::data::IDataFilterListener,
                   public std::enable_shared_from_this<DataFilter> {
 public:
    int init() {
        const int PROTO_UDP = 17;
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> pConnection{};
        std::promise<telux::common::ServiceStatus> pFilter{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataConMgr_ = dataFactory.getDataConnectionManager(
                DEFAULT_SLOT_ID,
                [&pConnection](telux::common::ServiceStatus status) {
            pConnection.set_value(status);
        });

        if (!dataConMgr_) {
            std::cout << "Can't get IDataConnectionManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = pConnection.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data connection service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        status = dataConMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener for connection, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        /* Step - 5 */
        dataFilterMgr_ = dataFactory.getDataFilterManager(
                DEFAULT_SLOT_ID,
                [&pFilter](telux::common::ServiceStatus status) {
            pFilter.set_value(status);
        });

        if (!dataFilterMgr_) {
            std::cout << "Can't get IDataFilterManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 6 */
        serviceStatus = pFilter.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data filter service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 7 */
        status = dataFilterMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener for filter, err " <<
                static_cast<int>(status) << std::endl;
            dataConMgr_->deregisterListener(shared_from_this());
            return -EIO;
        }

        /* Step - 8 */
        dataFilter_ = dataFactory.getNewIpFilter(PROTO_UDP);

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 13 */
        status = dataConMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister connection listener, err " <<
                static_cast<int>(status) << std::endl;
            dataFilterMgr_->deregisterListener(shared_from_this());
            return -EIO;
        }

        status = dataFilterMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister filter listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int triggerDataCall(int profileId) {
        telux::common::Status status;

        auto responseCb = std::bind(&DataFilter::onDataCallResponseAvailable,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 9 */
        status = dataConMgr_->startDataCall(
            profileId, telux::data::IpFamilyType::IPV4, responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't make call, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "\nData call start request placed" << std::endl;
        return 0;
    }

    int applyRestriction(std::string ipAddress, int port) {
        telux::common::Status status;
        telux::data::PortInfo srcPort{};
        telux::data::UdpInfo udpInfo{};
        telux::data::IPv4Info ipv4Info{};
        telux::data::DataRestrictMode enableMode{};
        std::shared_ptr<telux::data::IUdpFilter> udpFilter;

        /* Step - 10 */
        enableMode.filterMode = telux::data::DataRestrictModeType::ENABLE;
        enableMode.filterAutoExit = telux::data::DataRestrictModeType::DISABLE;
        auto restrictionResponseCb = std::bind(
            &DataFilter::restrictionResponseReceiver, this, std::placeholders::_1);

        status = dataFilterMgr_->setDataRestrictMode(
            enableMode,restrictionResponseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't set restrict mode, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        /* Step - 11 */
        ipv4Info.srcAddr = ipAddress;
        dataFilter_->setIPv4Info(ipv4Info);

        srcPort.port = port;
        srcPort.range = 0;
        udpInfo.src = srcPort;
        udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(dataFilter_);
        udpFilter->setUdpInfo(udpInfo);

        /* Step - 12 */
        status = dataFilterMgr_->addDataRestrictFilter(
            dataFilter_, restrictionResponseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't add restriction filter, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    /* Receives response of the startDataCall() request */
    void onDataCallResponseAvailable(
        const std::shared_ptr<telux::data::IDataCall> &dataCall,
        telux::common::ErrorCode ec) {
        std::cout << "\nonDataCallResponseAvailable(), err " << static_cast<int>(ec) << std::endl;
    }

    /* Receives response of the setDataRestrictMode() and addDataRestrictFilter() request */
    void restrictionResponseReceiver(telux::common::ErrorCode ec) {
        std::cout << "\nrestrictionResponseReceiver(), err " << static_cast<int>(ec) << std::endl;
    }

    void onDataCallInfoChanged(
        const std::shared_ptr<telux::data::IDataCall> &dataCall) override {
            std::cout << "onDataCallInfoChanged()" << std::endl;
        std::list<telux::data::IpAddrInfo> ipAddrList;

        std::cout << "Data call details" << std::endl;
        std::cout << "Slot ID " << dataCall->getSlotId() << std::endl;
        std::cout << "Profile ID " << dataCall->getProfileId() << std::endl;
        std::cout << "Interface name " << dataCall->getInterfaceName() << std::endl;
        std::cout << "Call status " <<
            static_cast<int>(dataCall->getDataCallStatus()) << std::endl;
        std::cout << "Call end reason " <<
            static_cast<int>(dataCall->getDataCallEndReason().type) << std::endl;

        ipAddrList = dataCall->getIpAddressInfo();
        for (auto &addr : ipAddrList) {
            std::cout << "\n ifAddress: " << addr.ifAddress <<
            "\n primaryDnsAddress: " << addr.primaryDnsAddress <<
            "\n secondaryDnsAddress: " << addr.secondaryDnsAddress << std::endl;
        }

        std::cout << "IP family type " <<
            static_cast<int>(dataCall->getIpFamilyType()) << std::endl;
        std::cout << "Tech preference " <<
            static_cast<int>(dataCall->getTechPreference()) << std::endl;
    }

 private:
    std::shared_ptr<telux::data::IIpFilter> dataFilter_;
    std::shared_ptr<telux::data::IDataConnectionManager> dataConMgr_;
    std::shared_ptr<telux::data::IDataFilterManager> dataFilterMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<DataFilter> app;

    int port;
    int profileId;
    std::string ipAddress;

    if (argc != 4) {
        std::cout << "Usage: ./data_filter_app <profile-id> <ip-address> <port>" << std::endl;
        return -EINVAL;
    }

    profileId = std::atoi(argv[1]);
    ipAddress = std::string(argv[2]);
    port = std::atoi(argv[3]);

    try {
        app = std::make_shared<DataFilter>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate DataFilter" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->triggerDataCall(profileId);
    if (ret < 0) {
        return ret;
    }

    ret = app->applyRestriction(ipAddress, port);
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses before exiting the application.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(5));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nData filter app exiting" << std::endl;
    return 0;
}