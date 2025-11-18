/*
 *  Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This application demonstrates how to bring up data calls on requested profile
 * and slot, resolve DNS using dig and communicate with remote host after binding
 * to an interface. The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IDataConnectionManager instance from DataFactory.
 * 3. Wait for the data connection service to become available.
 * 4. Register a listener which is invoked when there is change in data connection.
 * 5. Start a data call.
 * 6. Obtain the remote IP address.
 * 7. Connect to the remote host.
 * 8. Finally, when the use case is over deregister the listener.
 *
 * Usage:
 * # ./data_on_demand_pdn_app <slot-id> <profile-id> <operation-type> <domain> <port-number>
 *
 * Example: ./data_on_demand_pdn_app 1 2 0 www.example.com 80
 */

#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>

class OnDemandPDN : public telux::data::IDataConnectionListener,
                    public std::enable_shared_from_this<OnDemandPDN> {
 public:
    int init(SlotId slotId) {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataConMgr_  = dataFactory.getDataConnectionManager(slotId,
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

        /* Step - 8 */
        status = dataConMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }


    int triggerDataCall(int profileId, telux::data::OperationType opType) {
        telux::common::Status status;

        auto responseCb = std::bind(&OnDemandPDN::onDataCallResponseAvailable,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 5 */
        status = dataConMgr_->startDataCall(
            profileId, telux::data::IpFamilyType::IPV4, responseCb, opType);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't make call, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForResponse()) {
            std::cout << "Failed to start data call, err " <<
                static_cast<int>(errorCode_) << std::endl;
            return -EIO;
        }

        std::cout << "Data call initiated" << std::endl;
        return 0;
    }

    /* Step - 6 */
    int resolveDNS(std::string domain, std::string &remoteIPAddress) {
        const int SIZE_IP_ADDR_BUF = 40;
        FILE *stream;
        std::string command;
        std::string remoteIp;
        sockaddr_in sockAddress{};
        std::string dnsAddress;
        char address[SIZE_IP_ADDR_BUF] = {0};

        dnsAddress = dataCall_->getIpv4Info().addr.primaryDnsAddress;

        /* Use the provided DNS address to request dig for name resolution */
        command = "/usr/bin/dig @" + dnsAddress + " " + domain + " +short";

        stream = popen(command.c_str(), "r");
        if (!stream) {
            std::cout << "Can't create process, err " << errno << std::endl;
            return -errno;
        }

        /* Get all the answers from the DNS server */
        while (fgets(address, SIZE_IP_ADDR_BUF, stream) != NULL) {
            address[strlen(address) - 1] = '\0';

            /* If the received answer is verified as a valid IP address, return the address */
            if (inet_pton(AF_INET, address, &sockAddress.sin_addr) == 1) {
                remoteIPAddress = address;
                std::cout << remoteIPAddress;
                remoteIPAddress.erase(std::remove(remoteIPAddress.begin(),
                    remoteIPAddress.end(), '\n'), remoteIPAddress.end());
                break;
            }
        }

        std::cout << "\nResolved " << domain <<
            " using DNS server at " << dnsAddress << std::endl;
        return 0;
    }

    /* Step - 7 */
    int connectToHost(std::string remoteIPAddress, std::string portNumber) {
        int ret;
        ifreq ifr;
        int sockfd = 0;
        std::string outBoundIf;
        sockaddr_in serverIpAddress{};

        serverIpAddress.sin_family = AF_INET;
        serverIpAddress.sin_port = htons(stoi(portNumber));

        /* Convert IP address from text to binary */
        ret = inet_pton(AF_INET, remoteIPAddress.c_str(), &serverIpAddress.sin_addr);
        if (ret <= 0) {
            std::cout << "Can't connect" << std::endl;
            return (ret == 0 ? -EFAULT : -errno);
        }

        /* Create a socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cout << "Can't create socket" << std::endl;
            return -errno;
        }

        /* Bind the socket to the interface */
        outBoundIf = dataCall_->getInterfaceName();
        g_strlcpy(ifr.ifr_name, outBoundIf.c_str(), outBoundIf.length());

        /* Configure socket */
        ret = setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));
        if (ret < 0) {
            std::cout << "Can't create socket" << std::endl;
            return -errno;
        }

        /* Connect to the remote host */
        ret = connect(sockfd, (sockaddr *)&serverIpAddress, sizeof(serverIpAddress));
        if (ret < 0) {
            std::cout << "Can't connect" << std::endl;
            return -errno;
        }

        std::cout << "Connected to host" << std::endl;
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

    /* Receives response of the startDataCall() request */
    void onDataCallResponseAvailable(
        const std::shared_ptr<telux::data::IDataCall> &dataCall,
        telux::common::ErrorCode error) {

        std::lock_guard<std::mutex> lock(updateMutex_);
        std::cout << "\nonDataCallResponseAvailable(), err " <<
            static_cast<int>(error) << std::endl;
        errorCode_ = error;
        dataCall_ = dataCall;
        updateCV_.notify_one();
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
    std::mutex updateMutex_;
    std::condition_variable updateCV_;
    telux::common::ErrorCode errorCode_;
    std::shared_ptr<telux::data::IDataCall> dataCall_;
    std::shared_ptr<telux::data::IDataConnectionManager> dataConMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<OnDemandPDN> app;

    SlotId slotId;
    int profileId;
    telux::data::OperationType opType;
    std::string domain;
    std::string portNumber;
    std::string remoteIPAddress("");

    if (argc != 6) {
        std::cout << "./data_on_demand_pdn_app <slot-id> " <<
            "<profile-id> <operation-type> <domain> <port-number>" << std::endl;
        return -EINVAL;
    }

    slotId = static_cast<SlotId>(std::atoi(argv[1]));
    profileId = std::atoi(argv[2]);
    opType = static_cast<telux::data::OperationType>(std::atoi(argv[3]));
    domain = argv[4];
    portNumber = argv[5];

    try {
        app = std::make_shared<OnDemandPDN>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate OnDemandPDN" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    ret = app->triggerDataCall(profileId, opType);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->resolveDNS(domain, remoteIPAddress);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->connectToHost(remoteIPAddress, portNumber);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nOn-demand PDN app exiting" << std::endl;
    return 0;
}
