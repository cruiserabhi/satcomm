/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "FirewallMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

#define PROTO_ICMP 1
#define PROTO_IGMP 2
#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_ESP 50
#define PROTO_ICMP6 58

FirewallMenu::FirewallMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    firewallManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

FirewallMenu::~FirewallMenu() {
}

bool FirewallMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (firewallManager_ == nullptr) {
        auto initCb = std::bind(&FirewallMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();
        auto localFirewallMgr = dataFactory.getFirewallManager(
            telux::data::OperationType::DATA_LOCAL, initCb);
        if (localFirewallMgr) {
            firewallManager_ = localFirewallMgr;
        }
        auto remoteFirewallMgr = dataFactory.getFirewallManager(
            telux::data::OperationType::DATA_REMOTE, initCb);
        if (remoteFirewallMgr) {
            firewallManager_ = remoteFirewallMgr;
        }
        if(firewallManager_ == nullptr ) {
            //Return immediately
            std::cout << "\nError encountered in initializing Firewall Manager" << std::endl;
            return false;
        }
        firewallManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        //Firewall Manager is guaranteed to be valid pointer at this point. If manager
        //initialization fails and factory invalidated it's own pointer to firewall manager before
        //reaching this point, reference count of Firewall manager should still be 1
        telux::common::ServiceStatus subSystemStatus = firewallManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Firewall Manager, Please wait" << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = firewallManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nFirewall Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nFirewall Manager initialization failed" << std::endl;
            firewallManager_ = nullptr;
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> setFirewall
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "set_firewall", {},
                std::bind(&FirewallMenu::setFirewall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestFirewallStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "request_firewall_status",
            {}, std::bind(&FirewallMenu::requestFirewallStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addFirewallEntry =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "add_firewall_entry", {},
            std::bind(&FirewallMenu::addFirewallEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeFirewallEntry =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "remove_firewall_entry", {},
            std::bind(&FirewallMenu::removeFirewallEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestFirewallEntries =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "request_firewall_entries",
            {}, std::bind(&FirewallMenu::requestFirewallEntries, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> enableDmz =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "enable_dmz", {},
            std::bind(&FirewallMenu::enableDmz, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> disableDmz =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "disable_dmz",{},
            std::bind(&FirewallMenu::disableDmz, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestDmzEntry =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "request_dmz_entry", {},
            std::bind(&FirewallMenu::requestDmzEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addHwAccelerationFirewallEntry =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9",
            "add_hardware_acceleration_firewall_entry", {},std::bind(
            &FirewallMenu::addHwAccelerationFirewallEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestHwAccelerationFirewallEntries =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10",
            "request_hardware_acceleration_firewall_entries", {},std::bind(
            &FirewallMenu::requestHwAccelerationFirewallEntries, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {setFirewall,
            requestFirewallStatus, addFirewallEntry, removeFirewallEntry, requestFirewallEntries,
            enableDmz, disableDmz, requestDmzEntry, addHwAccelerationFirewallEntry,
            requestHwAccelerationFirewallEntries};

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void FirewallMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void FirewallMenu::parseProtoInfo(std::shared_ptr<IIpFilter> filter,
    telux::data::IpProtocol protocol, int &srcPort, int &dstPort, int &srcPortRange,
        int &dstPortRange, std::string &protoStr) {

    if (protocol == PROTO_TCP) {
        auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(filter);
        if(tcpFilter) {
            TcpInfo tcpInfo = tcpFilter->getTcpInfo();
            srcPort = tcpInfo.src.port;
            srcPortRange = tcpInfo.src.range;
            dstPort = tcpInfo.dest.port;
            dstPortRange = tcpInfo.dest.range;
            protoStr = "TCP";
        } else {
            std::cout << " TCP filter is NULL so couldn't get TCP info\n ";
        }
    } else if (protocol == PROTO_UDP) {
        auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(filter);
        if(udpFilter) {
            UdpInfo udpInfo = udpFilter->getUdpInfo();
            srcPort = udpInfo.src.port;
            srcPortRange = udpInfo.src.range;
            dstPort = udpInfo.dest.port;
            dstPortRange = udpInfo.dest.range;
            protoStr = "UDP";
        } else {
            std::cout << " UDP filter is NULL so couldn't get UDP info\n ";
        }
    } else if (protocol == PROTO_ICMP || protocol == PROTO_ICMP6) {
        auto icmpFilter = std::dynamic_pointer_cast<IIcmpFilter>(filter);
        if(icmpFilter) {
            IcmpInfo icmpInfo = icmpFilter->getIcmpInfo();
            protoStr = protocol == PROTO_ICMP?"ICMP":"ICMP6";
            std::cout << "Protocol : " << protoStr << std::endl;
            std::cout << "Icmp Type : " << (int)icmpInfo.type << std::endl;
            std::cout << "Icmp Code : " << (int)icmpInfo.code << std::endl;
        } else {
            std::cout << " ICMP filter is NULL so couldn't get ICMP info\n ";
        }
    } else if (protocol == PROTO_IGMP) {
        protoStr = "IGMP";
    } else if (protocol == PROTO_ESP) {
        protoStr = "ESP";
    } else {
       std::cout << "Error: invalid protocol \n ";
    }
    return;
}

void FirewallMenu::setFirewall(std::vector<std::string> inputCommand) {
    bool fwEnable = false;
    bool allowPackets = false;
    telux::common::Status retStat;

    std::cout << "Set Firewall\n";
    telux::data::net::FirewallConfig firewallConfig  = {};
    DataUtils::populateBackhaulInfo(firewallConfig.bhInfo);

    int enableFwFlag;
    std::cout << "Enter Enable Firewall (1 - On, 0 - Off): ";
    std::cin >> enableFwFlag;
    Utils::validateInput(enableFwFlag, {0, 1});
    if (enableFwFlag) {
        firewallConfig.enable = true;
    } else {
        firewallConfig.enable = false;
    }

    if (firewallConfig.enable) {
        int allowPacketsFlag;
        std::cout << "Enter Packets Allowed (1 - Accept, 0 - Drop): ";
        std::cin >> allowPacketsFlag;
        Utils::validateInput(allowPacketsFlag,  {0, 1});
        if (allowPacketsFlag) {
            firewallConfig.allowPackets = true;
        }
    } else {
        firewallConfig.allowPackets = false;
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setFirewall Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = firewallManager_->setFirewallConfig(firewallConfig, respCb);
    Utils::printStatus(retStat);
}

void FirewallMenu::requestFirewallStatus(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "request Firewall Status\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);
    auto respCb = [](FirewallConfig fwConfig, telux::common::ErrorCode error) {
       std::cout << std::endl << std::endl;
       std::cout << "CALLBACK: "
                 << "requestFirewallConfig Response"
                 << (error == telux::common::ErrorCode::SUCCESS
                         ? " is successful"
                         : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error)
                 << std::endl;
       std::cout << "Firewall "
                 << (fwConfig.enable ? "is enabled" : "not enabled") << "\n";
       if (fwConfig.enable) {
          std::cout << "Firewall enabled to "
                    << (fwConfig.allowPackets ? "Accept Packets"
                                              : "Drop packets")
                    << "\n";
          std::cout << "On Backhaul: "
                    << DataUtils::backhaulToString(fwConfig.bhInfo.backhaul);
          if (fwConfig.bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
             std::cout << ", Profile id: " << fwConfig.bhInfo.profileId;
          }
       }
       std::cout << "\n";
    };
    retStat = firewallManager_->requestFirewallConfig(bhInfo, respCb);
    Utils::printStatus(retStat);
}

void FirewallMenu::getIPV4ParamsFromUser(telux::data::IpProtocol proto,
    std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp) {
    std::string srcAddr = "", srcSubnetMask = "", destAddr = "", destSubnetMask = "";
    int tosVal = 0, tosMask = 0;
    char delimiter = '\n';

    int option;
    std::cout << "Do you want to enter IPV4 source address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter IPv4 Source address: ";
        std::getline(std::cin, srcAddr, delimiter);
        std::cout << "Enter IPv4 Source subnet mask: ";
        std::getline(std::cin, srcSubnetMask, delimiter);
    }

    std::cout << "Do you want to enter IPV4 destination address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter IPv4 Destination address: ";
        std::getline(std::cin, destAddr, delimiter);
        std::cout << "Enter IPv4 Destination subnet mask: ";
        std::getline(std::cin, destSubnetMask, delimiter);
    }

    std::cout << "Do you want to enter IPV4 TOS value and TOS mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter Type of service value [0 to 255]: ";
        while (true) {
            std::cin >> tosVal;
            Utils::validateInput(tosVal);
            if(tosVal > 255 || tosVal < 0) {
                std::cout << "Invalid value expected value [0 to 255]:";
                continue;
            }
            break;
        }

        std::cout << "Enter Type of service mask [0 to 255]: ";
        while (true) {
            std::cin >> tosMask;
            Utils::validateInput(tosMask);
            if(tosMask > 255 || tosMask < 0) {
                std::cout << "Invalid value expected value [0 to 255]:";
                continue;
            }
            break;
        }
    }

    IPv4Info info;
    info.srcAddr = srcAddr;
    info.srcSubnetMask = srcSubnetMask;
    info.destAddr = destAddr;
    info.destSubnetMask = destSubnetMask;
    info.value = (uint8_t)tosVal;
    info.mask = (uint8_t)tosMask;
    info.nextProtoId = proto;

    if (proto == 253) {
        info.nextProtoId = 6;
        ipFilter->setIPv4Info(info);
        info.nextProtoId = 17;
        ipFilterTcpUdp->setIPv4Info(info);
    } else {
        ipFilter->setIPv4Info(info);
    }
}

void FirewallMenu::getIPV6ParamsFromUser(telux::data::IpProtocol proto,
    std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp) {
    std::string srcAddr = "", destAddr = "";
    int srcPrefixLen = 0, dstPrefixLen = 0;
    int trfVal = 0, trfMask = 0, flowLabel = 0;
    char delimiter = '\n';

    int option;
    std::cout << "Do you want to enter IPV6 source address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter IPv6 Source address: ";
        std::getline(std::cin, srcAddr, delimiter);
        std::cout << "Enter IPv6 Source prefix length: ";
        std::cin >> srcPrefixLen;
        Utils::validateInput(srcPrefixLen);
    }

    std::cout << "Do you want to enter IPv6 destination address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter IPv6 Destination address: ";
        std::getline(std::cin, destAddr, delimiter);
        std::cout << "Enter IPv6 Destination prefix length: ";
        std::cin >> dstPrefixLen;
        Utils::validateInput(dstPrefixLen);
    }

    std::cout << "Do you want to enter IPV6 Traffic Class value and mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter IPv6 Traffic class value: ";
        std::cin >> trfVal;
        Utils::validateInput(trfVal);

        std::cout << "Enter IPv6 Traffic class mask: ";
        std::cin >> trfMask;
        Utils::validateInput(trfMask);

        std::cout << "Enter IPv6 flow label : ";
        std::cin >> flowLabel;
        Utils::validateInput(flowLabel);
    }


    int natEnabled;
    std::cout << "Enter IPv6 nat enabled (1-Enable, 0-Disabled): ";
    std::cin >> natEnabled;
    Utils::validateInput(natEnabled, {0, 1});

    IPv6Info info;
    info.srcAddr = srcAddr;
    info.destAddr = destAddr;
    info.srcPrefixLen = (uint8_t)srcPrefixLen;
    info.dstPrefixLen = (uint8_t)dstPrefixLen;
    info.nextProtoId = proto;
    info.val = (uint8_t)trfVal;
    info.mask = (uint8_t)trfMask;
    info.flowLabel = (uint32_t)flowLabel;
    info.natEnabled = (uint8_t)natEnabled;

    if (proto == 253) {
        info.nextProtoId = 6;
        ipFilter->setIPv6Info(info);
        info.nextProtoId = 17;
        ipFilterTcpUdp->setIPv6Info(info);
    } else {
        ipFilter->setIPv6Info(info);
    }
}

void FirewallMenu::getProtocolParamsFromUser(std::string proto, int &srcPort,
    int &srcRange, int &destPort, int &destRange) {
    char delimiter = '\n';
    int option;
    std::cout << "Do you want to enter Source Port and Range [1-YES 0-NO]";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter "<< proto <<" source port: ";
        std::cin >> srcPort;
        Utils::validateInput(srcPort);
        std::cout << "Enter "<< proto <<" source range: ";
        std::cin >> srcRange;
        Utils::validateInput(srcRange);
    }
    std::cout << "Do you want to enter Destination Port and Range [1-YES 0-NO]";
    std::cin >> option;
    Utils::validateInput(option, {1, 0});
    if (option == 1) {
        std::cout << "Enter "<< proto <<" destination port: ";
        std::cin >> destPort;
        Utils::validateInput(destPort);
        std::cout << "Enter "<< proto <<" destination range: ";
        std::cin >> destRange;
        Utils::validateInput(destRange);
    }
}

void FirewallMenu::getProtocolParams(telux::data::IpProtocol proto,
    std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp) {
    switch (proto) {
    case 6:  // TCP
    {
        TcpInfo tcpInfo;
        int srcPort = 0, srcRange = 0;
        int destPort = 0, destRange = (uint16_t)0;

        getProtocolParamsFromUser("TCP", srcPort, srcRange, destPort, destRange);
        tcpInfo.src.port = static_cast<uint16_t>(srcPort);
        tcpInfo.src.range = static_cast<uint16_t>(srcRange);
        tcpInfo.dest.port = static_cast<uint16_t>(destPort);
        tcpInfo.dest.range = static_cast<uint16_t>(destRange);

        auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(ipFilter);
        if(tcpFilter) {
            tcpFilter->setTcpInfo(tcpInfo);
        }
    } break;
    case 17:  // UDP
    {
        UdpInfo info;
        int srcPort = 0, srcRange = 0;
        int destPort = 0, destRange = 0;

        getProtocolParamsFromUser("UDP", srcPort, srcRange, destPort, destRange);
        info.src.port = static_cast<uint16_t>(srcPort);
        info.src.range = static_cast<uint16_t>(srcRange);
        info.dest.port = static_cast<uint16_t>(destPort);
        info.dest.range = static_cast<uint16_t>(destRange);

        auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(ipFilter);
        if(udpFilter) {
            udpFilter->setUdpInfo(info);
        }
    } break;
    case 253:  // TCP_UDP
    {
        TcpInfo tcpInfo;
        UdpInfo udpInfo;
        int srcPort = 0, srcRange = 0;
        int destPort = 0, destRange = 0;

        getProtocolParamsFromUser("", srcPort, srcRange, destPort, destRange);
        tcpInfo.src.port = static_cast<uint16_t>(srcPort);
        tcpInfo.src.range = static_cast<uint16_t>(srcRange);
        tcpInfo.dest.port = static_cast<uint16_t>(destPort);
        tcpInfo.dest.range = static_cast<uint16_t>(destRange);

        udpInfo.src.port = static_cast<uint16_t>(srcPort);
        udpInfo.src.range = static_cast<uint16_t>(srcRange);
        udpInfo.dest.port = static_cast<uint16_t>(destPort);
        udpInfo.dest.range = static_cast<uint16_t>(destRange);
        auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(ipFilter);
        if(tcpFilter) {
            tcpFilter->setTcpInfo(tcpInfo);
        }

        auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(ipFilterTcpUdp);
        if(udpFilter) {
            udpFilter->setUdpInfo(udpInfo);
        }
    } break;
    case 1:
    case 58:  // ICMP
    {
        int icmpType = 0;
        int icmpCode = 0;
        int option = 0;
        std::string protoStr = (proto == PROTO_ICMP)?"ICMP":"ICMP6";
        std::cout << "Do you want to enter "<< protoStr <<" Type [1-YES 0-NO] ";
        std::cin >> option;
        Utils::validateInput(option, {1, 0});
        if (option ==1) {
            std::cout << "enter the "<< protoStr <<" Type value: ";
            std::cin >> icmpType;
            Utils::validateInput(icmpType);
        }
        option = 0;
        std::cout << "Do you want to enter "<< protoStr <<" Code [1-YES 0-NO] ";
        std::cin >> option;
        if (option ==1) {
            std::cout << "enter the "<< protoStr <<" Code value: ";
            std::cin >> icmpCode;
            Utils::validateInput(icmpCode);
        }
        IcmpInfo icmpInfo {};
        icmpInfo.type = static_cast<uint8_t>(icmpType);
        icmpInfo.code = static_cast<uint8_t>(icmpCode);
        auto icmpFilter = std::dynamic_pointer_cast<IIcmpFilter>(ipFilter);
        if(icmpFilter) {
            icmpFilter->setIcmpInfo(icmpInfo);
        }
    } break;
    case 50:  // ESP
    {
        int esp_spi = 0;
        int option = 0;
        std::cout << "Do you want to enter ESP SPI [1-YES 0-NO] ";
        std::cin >> option;
        Utils::validateInput(option, {1, 0});
        if (option ==1) {
            std::cout << "enter ESP SPI value: ";
            std::cin >> esp_spi;
            Utils::validateInput(esp_spi);
        }
        EspInfo espInfo {};
        espInfo.spi = static_cast<uint32_t>(esp_spi);
        auto espFilter = std::dynamic_pointer_cast<IEspFilter>(ipFilter);
        if(espFilter) {
            espFilter->setEspInfo(espInfo);
        }
    } break;
    default:
        break;
    }
}

std::vector<std::shared_ptr<IFirewallEntry>> FirewallMenu::configureNewFirewallEntry() {
    std::vector<std::shared_ptr<IFirewallEntry>> fwEntries;
    int fwDirection;
    std::cout << "Enter Firewall Direction (1-Uplink, 2-Downlink): ";
    std::cin >> fwDirection;
    Utils::validateInput(fwDirection, {1, 2});
    telux::data::Direction fwDir = static_cast<telux::data::Direction>(fwDirection);
    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
        static_cast<int>(telux::data::IpFamilyType::IPV6)});
    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);

    char delimiter = '\n';
    std::string protoStr;
    if (ipFamilyType == 4) {
        std::cout << "Enter Protocol (TCP, UDP, TCP_UDP, ICMP, ESP): ";
    } else if (ipFamilyType == 6) {
        std::cout << "Enter Protocol (TCP, UDP, TCP_UDP, ICMP6, ESP): ";
    }
    std::getline(std::cin, protoStr, delimiter);
    telux::data::IpProtocol proto = DataUtils::getProtcol(protoStr);

    std::shared_ptr<telux::data::net::IFirewallEntry> fwEntry = nullptr;
    // To handle creation of TCP_UDP firewall entry
    std::shared_ptr<telux::data::net::IFirewallEntry> fwEntryTcpUdp = nullptr;
    auto &dataFactory = telux::data::DataFactory::getInstance();

    if (proto == 253) {
        fwEntry = dataFactory.getNewFirewallEntry(6, fwDir, ipFamType);
        fwEntryTcpUdp = dataFactory.getNewFirewallEntry(17, fwDir, ipFamType);
        fwEntries.emplace_back(fwEntry);
        fwEntries.emplace_back(fwEntryTcpUdp);
    } else {
        fwEntry = dataFactory.getNewFirewallEntry(proto, fwDir, ipFamType);
        fwEntries.emplace_back(fwEntry);
    }

    if (fwEntry) {
        std::shared_ptr<IIpFilter> ipFilter = fwEntry->getIProtocolFilter();
        std::shared_ptr<IIpFilter> ipFilterTcpUdp = nullptr;
        if (proto == 253) {
            ipFilterTcpUdp = fwEntryTcpUdp->getIProtocolFilter();
        }

        if (ipFamilyType == 4) {
            getIPV4ParamsFromUser(proto,ipFilter, ipFilterTcpUdp);
        }
        if (ipFamilyType == 6) {
            getIPV6ParamsFromUser(proto,ipFilter, ipFilterTcpUdp);
        }
        getProtocolParams(proto,ipFilter, ipFilterTcpUdp);
    } else {
        std::cout << "\nERROR: unable to get firewall entry instance\n";
    }
    return fwEntries;
}

void FirewallMenu::addHwAccelerationFirewallEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "Add hardware acceleration firewall entry \n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);

    std::vector<std::shared_ptr<IFirewallEntry>> fwEntries = configureNewFirewallEntry();

    auto respCb = [](const uint32_t handle, telux::common::ErrorCode error) {
        std::cout << std::endl
                  << std::endl;
        std::cout << "CALLBACK: "
                  << "addHwAccelerationFirewallEntry Response";
        if (error == telux::common::ErrorCode::SUCCESS)
            std::cout << " is successful. Handle of the firewall entry = " << handle << std::endl;
        else
            std::cout << " failed. ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    for (uint8_t i = 0; i < fwEntries.size(); i++) {
        FirewallEntryInfo bhFirewallEntry = {};
        bhFirewallEntry.bhInfo = bhInfo;
        bhFirewallEntry.fwEntry = fwEntries[i];
        retStat = firewallManager_->addHwAccelerationFirewallEntry(
            bhFirewallEntry, respCb);
        Utils::printStatus(retStat);
    }
}

void FirewallMenu::addFirewallEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "add Firewall Entry\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);
    std::vector<std::shared_ptr<IFirewallEntry>> fwEntries = configureNewFirewallEntry();
    auto respCb = [](const uint32_t handle, telux::common::ErrorCode error) {
       std::cout << std::endl << std::endl;
       std::cout << "CALLBACK: "
                 << "addFirewallEntry Response";
       if (error == telux::common::ErrorCode::SUCCESS)
          std::cout << " is successful. Handle of the firewall entry = "
                    << handle << std::endl;
       else
          std::cout << " failed. ErrorCode: " << static_cast<int>(error)
                    << ", description: " << Utils::getErrorCodeAsString(error)
                    << std::endl;
    };
    for (uint8_t i = 0; i < fwEntries.size(); i++) {
        FirewallEntryInfo bhFirewallEntry = {};
        bhFirewallEntry.bhInfo = bhInfo;
        bhFirewallEntry.fwEntry = fwEntries[i];
        retStat = firewallManager_->addFirewallEntry(bhFirewallEntry, respCb);
        Utils::printStatus(retStat);
    }
}

void FirewallMenu::requestHwAccelerationFirewallEntries(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "request hardware acceleration firewall entry\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);

    auto respCb = [this](std::vector<FirewallEntryInfo> entries,
                         telux::common::ErrorCode error) {
       std::cout << std::endl << std::endl;
       std::cout << "CALLBACK: "
                 << "requestFirewallEntries Response"
                 << (error == telux::common::ErrorCode::SUCCESS
                         ? " is successful"
                         : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error)
                 << std::endl;

       std::cout << "Found " << entries.size() << " entries\n";
       this->fwEntries_ = entries;
       if (entries.size() > 0) {
          this->displayFirewallEntry();
       }
    };
    retStat =
        firewallManager_->requestHwAccelerationFirewallEntries(bhInfo, respCb);

    Utils::printStatus(retStat);
}

void FirewallMenu::requestFirewallEntries(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "request Firewall Entry\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);
    auto respCb = [this](std::vector<FirewallEntryInfo> entries,
                         telux::common::ErrorCode error) {
       std::cout << std::endl << std::endl;
       std::cout << "CALLBACK: "
                 << "requestFirewallEntries Response"
                 << (error == telux::common::ErrorCode::SUCCESS
                         ? " is successful"
                         : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error)
                 << std::endl;

       std::cout << "Found " << entries.size() << " entries\n";
       this->fwEntries_ = entries;
       if (entries.size() > 0) {
          this->displayFirewallEntry();
       }
    };
    retStat = firewallManager_->requestFirewallEntries(bhInfo, respCb);
    Utils::printStatus(retStat);
}

void FirewallMenu::displayFirewallEntry() {
    for (uint8_t i = 0; i < fwEntries_.size(); i++) {
        std::shared_ptr<IIpFilter> ipfilter = fwEntries_[i].fwEntry->getIProtocolFilter();
        telux::data::IpFamilyType ipFamType = fwEntries_[i].fwEntry->getIpFamilyType();;

        IPv4Info ipv4Info = ipfilter->getIPv4Info();
        IPv6Info ipv6Info = ipfilter->getIPv6Info();
        IpProtocol proto = ipfilter->getIpProtocol();

        int srcPort, destPort, srcPortRange, dstPortRange;
        srcPort = destPort = srcPortRange = dstPortRange = 0;
        std::string protoStr;

        std::cout << "### Start Displaying firewall configuration of handle  = "
            << fwEntries_[i].fwEntry->getHandle() << " ###" << std::endl;
        std::cout << "Backhaul Type: "
            << DataUtils::backhaulToString(fwEntries_[i].bhInfo.backhaul);
        std::string dir  = (static_cast<uint32_t>(
                    fwEntries_[i].fwEntry->getDirection()) == 1)? "UPLINK":"DOWNLINK";
        std::cout << dir << " Firewall Rule"<< std::endl;

        if (ipFamType == IpFamilyType::IPV4) {
            std::cout << "Ip version : IPv4" << std::endl;
            if (ipv4Info.srcAddr.empty()) {
                std::cout << "SRC Addr : Any" << std::endl;
            } else {
                std::cout << "SRC Addr : " << ipv4Info.srcAddr << std::endl;
                std::cout << "SRC Addr Mask : " << ipv4Info.srcSubnetMask << std::endl;
            }

            if (ipv4Info.destAddr.empty()) {
                std::cout << "DST Addr : Any" << std::endl;
            } else {
                std::cout << "DST Addr : " << ipv4Info.destAddr << std::endl;
                std::cout << "DST Addr Mask : " << ipv4Info.destSubnetMask << std::endl;
            }

            if (!ipv4Info.value) {
                std::cout << "Tos value : Any" << std::endl;
            } else {
                std::cout << "Tos value : " << (uint32_t)ipv4Info.value << std::endl;
                std::cout << "Tos Mask : " << (uint32_t)ipv4Info.mask << std::endl;
            }
        } else if (ipFamType == IpFamilyType::IPV6) {
            std::cout << "Ip version : IPv6" << std::endl;
            if (ipv6Info.srcAddr.empty()) {
                std::cout << "SRC Addr : Any" << std::endl;
            } else {
                std::cout << "SRC Addr : " << ipv6Info.srcAddr << std::endl;
                std::cout << "SRC Addr prefix length : "
                          << (uint32_t)ipv6Info.srcPrefixLen << std::endl;
            }

            if (ipv6Info.destAddr.empty()) {
                std::cout << "DST Addr : Any" << std::endl;
            } else {
                std::cout << "DST Addr : " << ipv6Info.destAddr << std::endl;
                std::cout << "DST Addr prefix length : "
                          << (uint32_t)ipv6Info.dstPrefixLen << std::endl;
            }

            if (!ipv6Info.val) {
                std::cout << "Traffic class value : Any" << std::endl;
            } else {
                std::cout << "Traffic class value : " << (uint32_t)ipv6Info.val << std::endl;
                std::cout << "Traffic class Mask : " << (uint32_t)ipv6Info.mask << std::endl;
            }
            std::cout << "Ipv6 nat enabled fw entry is " <<
                (uint32_t)ipv6Info.natEnabled << std::endl;
        }
        parseProtoInfo(ipfilter, proto, srcPort, destPort, srcPortRange, dstPortRange, protoStr);
        if (protoStr=="TCP" || protoStr=="UDP") {
            std::cout << "Protocol : " << protoStr << std::endl;
            std::cout << "Src port : " << srcPort << std::endl;
            std::cout << "Src portrange  : " << srcPortRange << std::endl;
            std::cout << "Dst port  : " << destPort << std::endl;
            std::cout << "Dst portrange : " << dstPortRange << std::endl;
        }
        std::cout << "### End of Firewall configuration of handle  = "
            << fwEntries_[i].fwEntry->getHandle() << " ###" << std::endl << std::endl;
    }
}

void FirewallMenu::removeFirewallEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "remove Firewall Entry\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);

    int entryHandle;
    std::cout << "Enter handle of firewall entry to be removed: ";
    std::cin >> entryHandle;
    Utils::validateInput(entryHandle);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "removeFirewallEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat =
        firewallManager_->removeFirewallEntry(bhInfo, entryHandle, respCb);
    Utils::printStatus(retStat);
}

void FirewallMenu::enableDmz(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "Add DMZ\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);

    char delimiter = '\n';
    std::string ipAddr;
    std::cout << "Enter IP address: ";
    std::getline(std::cin, ipAddr, delimiter);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "enableDmz Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    telux::data::net::DmzConfig config;
    config.bhInfo = bhInfo;
    config.ipAddr = ipAddr;
    retStat = firewallManager_->enableDmz(config, respCb);
    Utils::printStatus(retStat);
}

void FirewallMenu::disableDmz(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "Remove DMZ\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);

    char delimiter = '\n';
    int ipType;
    std::cout << "Enter IP Type (4-IPv4, 6-IPv6): ";
    std::cin >> ipType;
    Utils::validateInput(ipType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
        static_cast<int>(telux::data::IpFamilyType::IPV6)});

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "disableDmz Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat = firewallManager_->disableDmz(
        bhInfo, static_cast<telux::data::IpFamilyType>(ipType), respCb);
    Utils::printStatus(retStat);
}

void FirewallMenu::requestDmzEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "request Dmz Entries\n";
    BackhaulInfo bhInfo = {};
    DataUtils::populateBackhaulInfo(bhInfo);

    auto respCb = [](std::vector<telux::data::net::DmzConfig> dmzEntries,
                     telux::common::ErrorCode error) {
       std::cout << std::endl << std::endl;
       std::cout << "CALLBACK: "
                 << "requestDmzEntry Response"
                 << (error == telux::common::ErrorCode::SUCCESS
                         ? " is successful"
                         : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error)
                 << std::endl;

       if (dmzEntries.size() > 0) {
          std::cout << "=============================================\n";
       }
       for (auto entry : dmzEntries) {
          std::cout << "On Backhaul: "
                    << DataUtils::backhaulToString(entry.bhInfo.backhaul);
          if (entry.bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
             std::cout << " And Profile id: " << entry.bhInfo.profileId;
          }
          std::cout << std::endl;
          std::cout << "address: " << entry.ipAddr
                    << "\n=============================================\n";
       }
    };
    retStat = firewallManager_->requestDmzEntry(bhInfo, respCb);
    Utils::printStatus(retStat);
}

