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
 * This application demonstrates how to set a firewall rule.
 * The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IFirewallManager instance from DataFactory.
 * 3. Wait for the firewall service to become available.
 * 4. Get Firewall entry based on IP protocol and set respective filter (i.e. TCP or UDP).
 * 5. Get IProtocol filter type.
 * 6. Set the IPv4 or IPv6 header info based on the IP family type.
 * 7. Set TCP or UDP header info based on the IP protocol.
 * 8. Finally, add the firewall rule.
 *
 * Usage:
 * # ./fwl_entry_sample_app <configuration-file>
 *
 * Example: ./fwl_entry_sample_app /etc/DataFwlEntryApp.conf
 *
 * This application assumes firewall has been enabled by running data_fwl_enable_app.
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
#include <telux/data/net/FirewallManager.hpp>

#include "ConfigParser.hpp"

/* Utility class to parse <configuration-file> and populate parameters */
class Utils {
 public:
    Utils(std::string configFile) {
        configParser_ = std::make_shared<ConfigParser>(configFile);
    };

    telux::data::OperationType getOperationType() {
        return static_cast<telux::data::OperationType>(
            std::atoi(configParser_->getValue(std::string("OPERATION_TYPE")).c_str()));
    }

    telux::data::Direction getDirection() {
        return static_cast<telux::data::Direction>(
            std::atoi(configParser_->getValue(std::string("DIRECTION")).c_str()));
    }

    int getProfileId() {
        return std::atoi(configParser_->getValue(std::string("PROFILE_ID")).c_str());
    }

    SlotId getSlotId() {
        return static_cast<SlotId>(
            std::atoi(configParser_->getValue(std::string("SLOT_ID")).c_str()));
    }

    telux::data::IpFamilyType getIPFamilyType() {
        return static_cast<telux::data::IpFamilyType>(
            std::atoi(configParser_->getValue(std::string("IP_FAMILY")).c_str()));
    }

    int getProtocol() {
        std::string prtocol;
        prtocol = configParser_->getValue(std::string("PROTOCOL"));
        if (!prtocol.compare("TCP")) {
            return 6;
        } else if (!prtocol.compare("UDP")) {
            return 17;
        } else {
            return -EINVAL;
        }
    }

    void populateIPv4Info(telux::data::IPv4Info &v4Info, int proto) {
        v4Info.nextProtoId = proto;
        v4Info.srcAddr = configParser_->getValue(std::string("SOURCE_ADDR"));
        v4Info.destAddr= configParser_->getValue(std::string("DEST_ADDR"));
        v4Info.srcSubnetMask = configParser_->getValue(std::string("IPV4_SRC_SUBNET_MASK"));
        v4Info.destSubnetMask = configParser_->getValue(std::string("IPV4_DEST_SUBNET_MASK"));
        v4Info.value = static_cast<uint8_t>(
            std::atoi(configParser_->getValue(std::string("IPV4_SERVICE_TYPE")).c_str()));
        v4Info.mask = static_cast<uint8_t>(
            std::atoi(configParser_->getValue(std::string("IPV4_SERVICE_TYPE_MASK")).c_str()));
    }

    void populateIPv6Info(telux::data::IPv6Info &v6Info, int proto) {
        v6Info.nextProtoId = proto;
        v6Info.srcAddr = configParser_->getValue(std::string("SOURCE_ADDR"));
        v6Info.destAddr= configParser_->getValue(std::string("DEST_ADDR"));
        v6Info.val = static_cast<uint8_t>(
            std::atoi(configParser_->getValue(std::string("IPV6_TRAFFIC_CLASS")).c_str()));
        v6Info.mask = static_cast<uint8_t>(
            std::atoi(configParser_->getValue(std::string("IPV6_TRAFFIC_CLASS_MASK")).c_str()));
        v6Info.flowLabel = static_cast<uint32_t>(
            std::atoi(configParser_->getValue(std::string("IPV6_FLOW_LABEL")).c_str()));
    }

    void populateProtocolInfo(uint16_t srcPort,
        uint16_t srcRange, uint16_t destPort, uint16_t destRange) {
        srcPort = std::atoi(
            configParser_->getValue(std::string("PROTOCOL_SRC_PORT")).c_str());
        srcRange = std::atoi(
            configParser_->getValue(std::string("PROTOCOL_SRC_RANGE")).c_str());
        destPort = std::atoi(
            configParser_->getValue(std::string("PROTOCOL_DEST_PORT")).c_str());
        destRange = std::atoi(
            configParser_->getValue(std::string("PROTOCOL_DEST_RANGE")).c_str());
    }

 private:
    std::shared_ptr<ConfigParser> configParser_;
};

class FirewallEntryCreator : public std::enable_shared_from_this<FirewallEntryCreator> {
 public:
    FirewallEntryCreator(std::shared_ptr<Utils> utils) {
        utils_ = utils;
    }

    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataFwMgr_ = dataFactory.getFirewallManager(utils_->getOperationType(),
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataFwMgr_) {
            std::cout << "Can't get IFirewallManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Firewall service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int addEntry() {
        int proto;
        telux::common::Status status;
        telux::data::IPv4Info v4Info{};
        telux::data::IPv6Info v6Info{};
        telux::data::TcpInfo tcpInfo{};
        telux::data::UdpInfo udpInfo{};
        telux::data::Direction direction;
        telux::data::IpFamilyType ipFamilyType;
        std::shared_ptr<telux::data::IIpFilter> ipFilter;
        std::shared_ptr<telux::data::net::IFirewallEntry> fwEntry;

        proto = utils_->getProtocol();
        direction = utils_->getDirection();
        ipFamilyType = utils_->getIPFamilyType();

        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 4 */
        fwEntry = dataFactory.getNewFirewallEntry(proto, direction, ipFamilyType);
        if (!fwEntry) {
            std::cout << "Can't get new firewall entry" << std::endl;
            return -EIO;
        }

        /* Step - 5 */
        ipFilter = fwEntry->getIProtocolFilter();
        if (!ipFilter) {
            std::cout << "Can't get filter" << std::endl;
            return -EIO;
        }

        /* Step - 6 */
        if (ipFamilyType == telux::data::IpFamilyType::IPV4) {
            utils_->populateIPv4Info(v4Info, proto);
            ipFilter->setIPv4Info(v4Info);
        } else if (ipFamilyType == telux::data::IpFamilyType::IPV6) {
            utils_->populateIPv6Info(v6Info, proto);
            ipFilter->setIPv6Info(v6Info);
        } else {
            std::cout << "Invalid family type " <<
                static_cast<int>(ipFamilyType) << std::endl;
            return -EINVAL;
        }

        /* Step - 7 */
        if (proto == 6) {
            utils_->populateProtocolInfo(
                tcpInfo.src.port, tcpInfo.src.range, tcpInfo.dest.port, tcpInfo.dest.range);
            auto tcpFilter = std::dynamic_pointer_cast<telux::data::ITcpFilter>(ipFilter);
            if(tcpFilter) {
                tcpFilter->setTcpInfo(tcpInfo);
            }
        } else {
            utils_->populateProtocolInfo(
                udpInfo.src.port, udpInfo.src.range, udpInfo.dest.port, udpInfo.dest.range);
            auto udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(ipFilter);
            if(udpFilter) {
                udpFilter->setUdpInfo(udpInfo);
            }
        }

        telux::data::BackhaulInfo bhInfo = {};
        telux::data::net::FirewallEntryInfo entryInfo = {};

        bhInfo.slotId = utils_->getSlotId();
        bhInfo.backhaul = telux::data::BackhaulType::WWAN;
        bhInfo.profileId = utils_->getProfileId();

        entryInfo.bhInfo = bhInfo;
        entryInfo.fwEntry = fwEntry;

        auto respCb = std::bind(
            &FirewallEntryCreator::fwEntryResponse, this, std::placeholders::_1,
            std::placeholders::_2);

        /* Step - 8 */
        status = dataFwMgr_->addFirewallEntry(entryInfo, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't add entry, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    /* Receives response of the addFirewallEntry() request */
    void fwEntryResponse(const uint32_t handle, telux::common::ErrorCode error) {
        std::cout << "\nfwEntryResponse(), err " << static_cast<int>(error) << std::endl;
        std::cout << "\nfwEntryResponse(), handle " << handle << std::endl;
    }

 private:
    std::shared_ptr<Utils> utils_;
    std::shared_ptr<telux::data::net::IFirewallManager> dataFwMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<FirewallEntryCreator> app;

    std::shared_ptr<Utils> utils;

    if (argc != 2) {
        std::cout << "Usage: ./fwl_entry_sample_app <config-file>" << std::endl;
        return -EINVAL;
    }

    try {
        utils = std::make_shared<Utils>(argv[1]);
        app = std::make_shared<FirewallEntryCreator>(utils);
    } catch (const std::exception& e) {
        std::cout << "Can't allocate Utils/FirewallEntryCreator" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->addEntry();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\nFirewall entry creator app exiting" << std::endl;
    return 0;
}
