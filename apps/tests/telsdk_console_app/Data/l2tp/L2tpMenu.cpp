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
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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


extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "L2tpMenu.hpp"

using namespace std;

L2tpMenu::L2tpMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    l2tpManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

L2tpMenu::~L2tpMenu() {
    l2tpManager_ = nullptr;
}

bool L2tpMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (l2tpManager_ == nullptr) {
        auto initCb = std::bind(&L2tpMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();
        l2tpManager_ = dataFactory.getL2tpManager(initCb);
        if (l2tpManager_ == nullptr) {
            std::cout << "\nError encountered in initializing Bridge Manager" << std::endl;
            return false;
        }
        l2tpManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        //L2TP Manager is guaranteed to be valid pointer at this point. If manager initialization
        //fails and factory invalidated it's own pointer to L2TP manager before reaching this
        //point, reference count of L2TP manager should still be 1
        telux::common::ServiceStatus subSystemStatus = l2tpManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing L2TP Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = l2tpManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nL2TP Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nL2TP Manager initialization failed" << std::endl;
            l2tpManager_ = nullptr;
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> setConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Set_Configuration",
                {}, std::bind(&L2tpMenu::setConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addTunnel
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Add_Tunnel", {},
                std::bind(&L2tpMenu::addTunnel, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Request_Configuration", {},
                std::bind(&L2tpMenu::requestConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeTunnel
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Remove_Tunnel", {},
                std::bind(&L2tpMenu::removeTunnel, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addSessionToTunnel
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Add_Session_To_Tunnel",
                {}, std::bind(&L2tpMenu::addSessionToTunnel, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeSessionFromTunnel
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
                "6", "Remove_Session_From_Tunnel", {}, std::bind(
                &L2tpMenu::removeSessionFromTunnel, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> bindSessionToBackhaul
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
                "7", "Bind_Session_To_Backhaul", {}, std::bind(
                &L2tpMenu::bindSessionToBackhaul, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> unbindSessionFromBackhaul
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
                "8", "Unbind_Session_From_Backhaul", {}, std::bind(
                &L2tpMenu::unbindSessionFromBackhaul, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> querySessionToBackhaulMapping
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
                "9", "Query_Session_To_Backhaul_Mappings", {}, std::bind(
                &L2tpMenu::querySessionToBackhaulMapping, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {setConfig,
            addTunnel, requestConfig, removeTunnel, addSessionToTunnel, removeSessionFromTunnel,
            bindSessionToBackhaul, unbindSessionFromBackhaul, querySessionToBackhaulMapping};

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void L2tpMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void L2tpMenu::setConfig(std::vector<std::string> inputCommand) {
    std::cout << "Set L2TP Unamanged Tunnel\n";
    telux::common::Status retStat;
    bool enable = true;
    bool enableMss =  false;
    bool enableMtu = false;
    uint32_t mtuSize = 0;
    int inputFlag;
    std::cout << "Enable/Disable L2TP for unmanaged tunnels\n (1-enable, 0-disable): ";
    std::cin >> inputFlag;
    Utils::validateInput(inputFlag);
    if(inputFlag == 0) {
        enable = false;
    }
    else {
        std::cout << "Enable/Disable TCP MSS clampping on L2TP interfaces to avoid segmentation\n"
            "(1-enable, 0-disable): ";
        std::cin >> inputFlag;
        Utils::validateInput(inputFlag);
        if(inputFlag) {
            enableMss = true;
        }
        std::cout << "Enable/Disable MTU size setting on underlying interfaces to avoid "
            "segmentation" << std::endl << "(1-enable, 0-disable): ";
        std::cin >> inputFlag;
        Utils::validateInput(inputFlag);
        if(inputFlag) {
            enableMtu = true;
            std::cout << "Use Default MTU size - 1422 bytes? (1-yes, 0-no): ";
            std::cin >> inputFlag;
            Utils::validateInput(inputFlag);
            if(inputFlag == 0) {
                std::cout << "Enter MTU size : ";
                std::cin >> mtuSize;
                Utils::validateInput(mtuSize);
            }
        }
    }
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Set L2TP Unamanged Tunnel Response is"
                  << (telux::common::ErrorCode::SUCCESS == error ? " successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat = l2tpManager_->setConfig(enable, enableMss, enableMtu, respCb, mtuSize);
    Utils::printStatus(retStat);
}

void L2tpMenu::addTunnel(std::vector<std::string> inputCommand) {
    std::cout << "Set L2TP Configuration\n";
    telux::common::Status retStat;
    telux::data::net::L2tpTunnelConfig l2tpTunnelConfig;
    char delimiter = '\n';
    std::cin.get();
    std::cout << "Enter interface name to create L2TP tunnel on: ";
    std::getline(std::cin, l2tpTunnelConfig.locIface, delimiter);

    uint32_t tempInt;
    std::cout << "Enter local tunnel id: ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    l2tpTunnelConfig.locId = tempInt;
    std::cout << "Enter peer tunnel id: ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    l2tpTunnelConfig.peerId = tempInt;
    std::cout << "Enter peer ip version (4-IPv4, 6-IPv6): ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    if (4 == tempInt) {
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV4;
        std::cin.get();
        std::cout << "Enter peer ipv4 address : ";
        std::getline(std::cin, l2tpTunnelConfig.peerIpv4Addr, delimiter);
        std::cout << "Do you want to enter peer ipv4 gateway address? (0-No, 1-Yes): ";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        if(tempInt) {
            std::cin.get();
            std::cout << "Enter peer ipv4 gateway address : ";
            std::getline(std::cin, l2tpTunnelConfig.peerIpv4GwAddr, delimiter);
        } else {
            l2tpTunnelConfig.peerIpv4GwAddr = "";
        }
    }
    else if (6 == tempInt) {
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV6;
        std::cin.get();
        std::cout << "Enter peer ipv6 address : ";
        std::getline(std::cin, l2tpTunnelConfig.peerIpv6Addr, delimiter);
        std::cout << "Do you want to enter peer ipv6 gateway address? (0-No, 1-Yes): ";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        if(tempInt) {
            std::cin.get();
            std::cout << "Enter peer ipv6 gateway address : ";
            std::getline(std::cin, l2tpTunnelConfig.peerIpv6GwAddr, delimiter);
        } else {
            l2tpTunnelConfig.peerIpv6GwAddr = "";
        }
    }
    else  {
        std::cout << "Invalid IP type entered .. exiting ..." <<std::endl;
        return;
    }
    std::cout << "Enter encapsulation protocol (0-IP, 1-UDP): ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    if (0 == tempInt) {
        l2tpTunnelConfig.prot = L2tpProtocol::IP;
    }
    else if (1 == tempInt) {
        l2tpTunnelConfig.prot = L2tpProtocol::UDP;
        std::cout << "Enter local udp port: ";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpTunnelConfig.localUdpPort = tempInt;
        std::cout << "Enter peer udp port: ";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpTunnelConfig.peerUdpPort = tempInt;
    }
    else  {
        std::cout << "Invalid protocol entered .. exiting ..." <<std::endl;
        return;
    }
    std::cout << "Enter number of sessions for this tunnel (max allowed 4): ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    if (tempInt > 4) {
        std::cout << "Invalid number of sessions .. exiting ..." <<std::endl;
        return;
    }
    int num_sessions = tempInt;
    for (int i=0; i<num_sessions; i++) {
        telux::data::net::L2tpSessionConfig l2tpSessionConfig;
        std::cout << "Enter local session id for session " << i+1 << " :";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpSessionConfig.locId = tempInt;
        std::cout << "Enter peer session id for session " << i+1 << " :";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpSessionConfig.peerId = tempInt;
        l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig);
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Set L2TP Config Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::NOT_SUPPORTED) {
            std::cout << "L2TP config not supported.";
        }
        else if (error == telux::common::ErrorCode::INCOMPATIBLE_STATE) {
            std::cout << "L2TP config can not be enabled...\n";
        }
        else if (error == telux::common::ErrorCode::NO_EFFECT) {
            std::cout << "L2TP Config already set";
        }
    };
    retStat = l2tpManager_->addTunnel(l2tpTunnelConfig, respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::requestConfig(std::vector<std::string> inputCommand) {
    auto respCb = [](const L2tpSysConfig &l2tpSysConfig, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        if (error == telux::common::ErrorCode::NOT_SUPPORTED) {
            std::cout << "L2TP Unmanaged tunnel state is not enabled" <<std::endl;
            return;
        }
        std::cout << "CALLBACK: "
                  << "Get L2TP Config Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        std::cout << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout <<  "MTU Config is " <<
                (true == l2tpSysConfig.enableMtu ? "Enabled" : "Disabled") << std::endl;
            if (l2tpSysConfig.mtuSize > 0) {
                std::cout <<  "MTU Size is " << l2tpSysConfig.mtuSize << std::endl;
            }
            std::cout <<  "TCP MSS Config is " <<
                (true == l2tpSysConfig.enableTcpMss ? "Enabled" : "Disabled") << std::endl;
            if (l2tpSysConfig.configList.empty()) {
                std::cout <<  "No Tunnel Configurations Detected" << std::endl;
            }
            else {
                std::cout <<  "Current Tunnel Configurations" << std::endl;
            }
            for (auto tnl : l2tpSysConfig.configList) {
                std::cout <<  "=========== Tunnel Configuration ===========" << std::endl;
                std::cout << "\tPhysical Interface: " << tnl.locIface << std::endl;
                std::cout << "\tLocal Tunnel ID: " << tnl.locId << std::endl;
                std::cout << "\tPeer Tunnel ID: " << tnl.peerId << std::endl;
                if (telux::data::IpFamilyType::IPV4 == tnl.ipType) {
                    std::cout << "\tIP Version: IPv4" << std::endl;
                    std::cout << "\tPeer IPv4 Address :" << tnl.peerIpv4Addr << std::endl;
                }
                else if (telux::data::IpFamilyType::IPV6 == tnl.ipType) {
                    std::cout << "\tIP Version: IPv6" << std::endl;
                    std::cout << "\tPeer IPv6 Address :" << tnl.peerIpv6Addr << std::endl;
                }
                else {
                    std::cout << "\tIP Version: Unknown" << std::endl;
                }
                if (telux::data::net::L2tpProtocol::IP == tnl.prot) {
                    std::cout << "\tEncapsulation Protocol: IP" << std::endl;
                }
                else if (telux::data::net::L2tpProtocol::UDP == tnl.prot) {
                    std::cout << "\tEncapsulation Protocol: UDP" << std::endl;
                    std::cout << "\tLocal UDP Port : " << tnl.localUdpPort << std::endl;
                    std::cout << "\tPeer UDP Port : " << tnl.peerUdpPort << std::endl;
                }
                else {
                    std::cout << "\tEncapsulation Protocol: Unknown" << std::endl;
                }
                int cnt = 1;
                for (auto session : tnl.sessionConfig) {
                    std::cout << "\tSession: " << cnt << std::endl;
                    std::cout << "\t    Session ID : " << session.locId << std::endl;
                    std::cout << "\t    Peer Session ID : " << session.peerId << std::endl;
                    cnt ++;
                }
            }
            std::cout << std::endl;
        }
    };

    std::cout << "Request L2TP Configuration\n";
    telux::common::Status retStat;
    retStat = l2tpManager_->requestConfig(respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::removeTunnel(std::vector<std::string> inputCommand) {
    std::cout << "Remove L2TP Tunnel\n";
    telux::common::Status retStat;
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Remove L2TP Configuration Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    uint32_t tempInt;
    std::cout << "Enter Tunnel ID to be deleted: ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);

    retStat = l2tpManager_->removeTunnel(tempInt, respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::addSessionToTunnel(std::vector<std::string> inputCommand) {
    std::cout << "Add Session To Tunnel\n";
    telux::common::Status retStat;
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Add L2TP Session to Tunnel Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    uint32_t tunnelId;
    std::cout << "Enter tunnel ID to add session to: ";
    std::cin >> tunnelId;
    Utils::validateInput(tunnelId);

    uint32_t userInput;
    telux::data::net::L2tpSessionConfig sessionConfig{};
    std::cout << "Enter local ID of new session: ";
    std::cin >> userInput;
    Utils::validateInput(userInput);
    sessionConfig.locId = userInput;
    std::cout << "Enter peer ID of new session: ";
    std::cin >> userInput;
    Utils::validateInput(userInput);
    sessionConfig.peerId = userInput;
    retStat = l2tpManager_->addSession(tunnelId, sessionConfig, respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::removeSessionFromTunnel(std::vector<std::string> inputCommand) {
    std::cout << "Remove Session From Tunnel\n";
    telux::common::Status retStat;
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Remove L2TP Session From Tunnel Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    uint32_t tunnelId;
    std::cout << "Enter tunnel ID to remove session from: ";
    std::cin >> tunnelId;
    Utils::validateInput(tunnelId);

    uint32_t sessionId;
    std::cout << "Enter local ID of session to be removed: ";
    std::cin >> sessionId;
    Utils::validateInput(sessionId);
    retStat = l2tpManager_->removeSession(tunnelId, sessionId, respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::bindSessionToBackhaul(std::vector<std::string> inputCommand) {
    std::cout << "Bind Session To Backhaul\n";
    telux::common::Status retStat;

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Bind L2TP Session To Backhaul Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    telux::data::net::L2tpSessionBindConfig bindConfig{};

    uint32_t sessionId;
    std::cout << "Enter local ID of session: ";
    std::cin >> sessionId;
    Utils::validateInput(sessionId);
    bindConfig.locId = sessionId;

    int bhType;
    bindConfig.bhInfo.backhaul = BackhaulType::WWAN;
    std::cout << "Enter backhaul type (1-WWAN, 2-ETH): ";
    std::cin >> bhType;
    Utils::validateInput(bhType, {1, 2});
    bindConfig.bhInfo.backhaul = ( bhType == 1 ? BackhaulType::WWAN : BackhaulType::ETH);

    if (bindConfig.bhInfo.backhaul == BackhaulType::WWAN) {
        int profileId;
        std::cout << "Enter Profile Id to bind session to: ";
        std::cin >> profileId;
        Utils::validateInput(profileId);
        bindConfig.bhInfo.profileId = profileId;
        int slotId = DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
            slotId = Utils::getValidSlotId();
        }
        bindConfig.bhInfo.slotId = static_cast<SlotId>(slotId);
    } else if (bindConfig.bhInfo.backhaul == BackhaulType::ETH) {
        int vlanId;
        std::cout << "Enter Vlan Id to bind session to: ";
        std::cin >> vlanId;
        Utils::validateInput(vlanId);
        bindConfig.bhInfo.vlanId = vlanId;
    }
        retStat = l2tpManager_->bindSessionToBackhaul(bindConfig, respCb);
        Utils::printStatus(retStat);
}

void L2tpMenu::unbindSessionFromBackhaul(std::vector<std::string> inputCommand) {
    std::cout << "Unbind Session From Backhaul\n";
    telux::common::Status retStat;

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Unbind L2TP Session From Backhaul Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    telux::data::net::L2tpSessionBindConfig bindConfig{};

    uint32_t sessionId;
    std::cout << "Enter local ID of session: ";
    std::cin >> sessionId;
    Utils::validateInput(sessionId);
    bindConfig.locId = sessionId;

    int bhType;
    bindConfig.bhInfo.backhaul = BackhaulType::WWAN;
    std::cout << "Enter backhaul type (1-WWAN, 2-ETH): ";
    std::cin >> bhType;
    Utils::validateInput(bhType, {1, 2});
    bindConfig.bhInfo.backhaul = ( bhType == 1 ? BackhaulType::WWAN : BackhaulType::ETH);

    if (bindConfig.bhInfo.backhaul == BackhaulType::WWAN) {
        int profileId;
        std::cout << "Enter Profile Id to unbind session from: ";
        std::cin >> profileId;
        Utils::validateInput(profileId);
        bindConfig.bhInfo.profileId = profileId;
        int slotId = DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
            slotId = Utils::getValidSlotId();
        }
        bindConfig.bhInfo.slotId = static_cast<SlotId>(slotId);
    } else if (bindConfig.bhInfo.backhaul == BackhaulType::ETH) {
        int vlanId;
        std::cout << "Enter Vlan Id to bind session to: ";
        std::cin >> vlanId;
        Utils::validateInput(vlanId);
        bindConfig.bhInfo.vlanId = vlanId;
    }

    retStat = l2tpManager_->unbindSessionFromBackhaul(bindConfig, respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::querySessionToBackhaulMapping(std::vector<std::string> inputCommand) {
    std::cout << "Query Session To Backhaul Mappings\n";
    telux::common::Status retStat;
    telux::data::BackhaulType backhaulType = BackhaulType::WWAN;

    int bhType;
    std::cout << "Enter backhaul type (1-WWAN, 2-ETH): ";
    std::cin >> bhType;
    Utils::validateInput(bhType, {1, 2});
    backhaulType = ( bhType == 1 ? BackhaulType::WWAN : BackhaulType::ETH);

    auto respCb = [](const std::vector<telux::data::net::L2tpSessionBindConfig> bindings,
        telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Query Session To Backhaul Mappings Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if(error == telux::common::ErrorCode::SUCCESS) {
            if(bindings.size() > 0) {
                for (auto c : bindings) {
                    std::string bh = (c.bhInfo.backhaul == telux::data::BackhaulType::ETH ?
                            "ETH" : (c.bhInfo.backhaul == telux::data::BackhaulType::WWAN ? "WWAN" :
                                "UNKNOWN"));
                    if (c.bhInfo.backhaul == telux::data::BackhaulType::WWAN) {
                        std::cout << "Backhaul: " << bh << ", profId: " << (int)c.bhInfo.profileId
                            << ", slotId: " << c.bhInfo.slotId << ", Local id: " << c.locId << "\n";
                    } else if (c.bhInfo.backhaul == telux::data::BackhaulType::ETH) {
                        std::cout << "Backhaul: " << bh << ", vlanId associated with session: "
                            << (int)c.bhInfo.vlanId << ", Local id: " << c.locId << "\n";
                    }
                }
            } else {
                std::cout << "No bindings found" << std::endl;
            }
        }
    };
    retStat = l2tpManager_->querySessionToBackhaulBindings(backhaulType, respCb);
    Utils::printStatus(retStat);
}
