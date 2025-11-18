/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * This application demonstrates how to enable IP passthrough for a data call that is running in
 * peer NAD. The steps are as follows:
 *
 * NAD-2:
 * 1. Get DataSettingsManager from DataFactory.
 * 2. Get VlanManager from DataFactory.
 * 3. Get DataConnectionManager from Datafactory.
 * 4. Create Vlan for the LAN network type.
 * 5. Trigger data call.
 * 6. Bind a VLAN with a WWAN backhaul.
 * 7. Wait until data call status changes to CONNECTED.
 * 8. Enable IP passthrough for a VLAN and profile ID to access a data call in peer NAD.
 *
 * NAD-1:
 * 9. Get DataSettingsManager from DataFactory.
 * 10. Get VlanManager from DataFactory.
 * 11. Create two Vlans, first for the LAN network type that connects to a main unit and second for
 *     the WAN network type that connects to an ETH backhaul.
 * 12. Bind LAN VLAN and WAN VLAN to an ETH backhaul.
 * 13. Enable dynamic IP configuration on WAN VLAN that gives access to a data call that is running
 * in NAD-2.
 * 14. De-init application on end of usecase.
 *
 * Usage:
 * # On NAD-1: ./data_ip_passthrough_app_nad1 <NAD>
 *
 * Example:
 * # On NAD-1: ./data_ip_passthrough_app_nad1 "NAD-1"
 * # On NAD-2: ./data_ip_passthrough_app_nad1 "NAD-2"
 */

#include <errno.h>

#include <string.h>
#include <unistd.h>

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
#include <telux/data/DataSettingsManager.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/net/VlanManager.hpp>

#define PROFILE_ID  1
#define LAN_VLAN_ID 1
#define WAN_VLAN_ID 4
#define SLOT_ID DEFAULT_SLOT_ID

class IpPassThrough : public telux::data::IDataConnectionListener,
                      public std::enable_shared_from_this<IpPassThrough> {
 public:
    int initDataSettingsManager(telux::data::OperationType opType) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataSettingsMgr_  = dataFactory.getDataSettingsManager(opType,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataSettingsMgr_) {
            std::cout << "Can't get IDataSettingsManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int initVlanManager(telux::data::OperationType opType) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataVlanMgr_ = dataFactory.getVlanManager(
                opType, [&p](telux::common::ServiceStatus status) {
                p.set_value(status);
                });

        if (!dataVlanMgr_) {
            std::cout << "Can't get IVlanManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "VLAN service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int initDataConnectionManager() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataConMgr_  = dataFactory.getDataConnectionManager(SLOT_ID,
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

    void userInputForVlan(std::string nad) {
        if (nad == "NAD-1") {
            wanVlanConfig_.iface = telux::data::InterfaceType::ETH;
            wanVlanConfig_.vlanId = WAN_VLAN_ID;
            wanVlanConfig_.priority = false;
            wanVlanConfig_.isAccelerated = true;
            wanVlanConfig_.createBridge  = false;
            wanVlanConfig_.nwType  = telux::data::NetworkType::WAN;

            nad1LanVlanConfig_.iface = telux::data::InterfaceType::ETH;
            nad1LanVlanConfig_.vlanId = LAN_VLAN_ID;
            nad1LanVlanConfig_.priority = false;
            nad1LanVlanConfig_.isAccelerated = true;
            nad1LanVlanConfig_.createBridge  = true;
            nad1LanVlanConfig_.nwType  = telux::data::NetworkType::LAN;
        } else if (nad == "NAD-2") {
            nad2LanVlanConfig_.iface = telux::data::InterfaceType::ETH;
            nad2LanVlanConfig_.vlanId = LAN_VLAN_ID;
            nad2LanVlanConfig_.priority = false;
            nad2LanVlanConfig_.isAccelerated = true;
            nad2LanVlanConfig_.createBridge  = true;
            nad2LanVlanConfig_.nwType  = telux::data::NetworkType::LAN;
        }
    }

    void userInputForIpConfig() {
        ipConfigParams_.ifType       = telux::data::InterfaceType::ETH;
        // The user can choose IPV6 address for the IPV6 data call
        ipConfigParams_.ipFamilyType = telux::data::IpFamilyType::IPV4;
        // WAN vlan id
        ipConfigParams_.vlanId = wanVlanConfig_.vlanId;
        // In case of IpAssignType is STATIC_IP, user must give data call IP address which is
        // running in NAD-2
        ipConfig_.ipType = telux::data::IpAssignType::DYNAMIC_IP;
        // In case of disable IP configuration, user can provide IpAssignOperation to DISABLE
        ipConfig_.ipOpr = telux::data::IpAssignOperation::ENABLE;
    }

    void userInputForIpPassThrough() {
        ipptParams_.profileId                = PROFILE_ID;
        ipptParams_.vlanId                   = nad2LanVlanConfig_.vlanId;
        ipptParams_.slotId                   = SLOT_ID;
        ipptConfig_.ipptOpr                  = telux::data::Operation::ENABLE;
        ipptConfig_.devConfig.nwInterface    = telux::data::InterfaceType::ETH;
        ipptConfig_.devConfig.macAddr        = "1a:2b:3c:4d:5e:6f";
    }

    /* Called as a response to createVlan() request */
    void onVLANCreateStatusAvailable(
        bool isAccelerated, telux::common::ErrorCode error) {
        std::cout << "onVLANCreateStatusAvailable()" << std::endl;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to create VLAN, err" <<
                static_cast<int>(error) << std::endl;
            return;
        }

        std::cout << "VLAN created successfully" << std::endl;
    }

    int createVlan(const std::string nad) {

        auto respCb = std::bind(&IpPassThrough::onVLANCreateStatusAvailable,
                this, std::placeholders::_1, std::placeholders::_2);

        if (nad == "NAD-1") {
            userInputForVlan(nad);

            /** Create Vlan for LAN network type */
            auto retStatus = dataVlanMgr_->createVlan(nad1LanVlanConfig_, respCb);
            if (retStatus != telux::common::Status::SUCCESS) {
                std::cout << "Can't create VLAN, err " <<
                    static_cast<int>(retStatus) << std::endl;
                return -EIO;
            }

            /** Create Vlan for WAN network type */
            retStatus = dataVlanMgr_->createVlan(wanVlanConfig_, respCb);
            if (retStatus != telux::common::Status::SUCCESS) {
                std::cout << "Can't create VLAN, err " <<
                    static_cast<int>(retStatus) << std::endl;
                return -EIO;
            }
        } else if (nad == "NAD-2"){
            userInputForVlan(nad);

            /** Create Vlan for LAN network type */
            auto retStatus = dataVlanMgr_->createVlan(nad2LanVlanConfig_, respCb);
            if (retStatus != telux::common::Status::SUCCESS) {
                std::cout << "Can't create VLAN, err " <<
                    static_cast<int>(retStatus) << std::endl;
                return -EIO;
            }
        }
        return 0;
    }

    /* Called as a response to bindToBackhaul() request */
    void onBindStatusAvailable(telux::common::ErrorCode error) {
        std::cout << "onBindStatusAvailable()" << std::endl;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to bind VLAN, err" <<
                static_cast<int>(error) << std::endl;
            return;
        }

        std::cout << "VLAN binded successfully" << std::endl;
    }

    int bindWwanBackhaul() {

        telux::common::Status status;
        telux::data::net::VlanBindConfig vlanBindConfig;

        // Assuming Data call is running in NAD-2
        vlanBindConfig.bhInfo.backhaul = telux::data::BackhaulType::WWAN;
        vlanBindConfig.vlanId = nad2LanVlanConfig_.vlanId;
        vlanBindConfig.bhInfo.profileId = PROFILE_ID;
        vlanBindConfig.bhInfo.slotId = SLOT_ID;

        auto respCb = std::bind(&IpPassThrough::onBindStatusAvailable,
            this, std::placeholders::_1);

        status = dataVlanMgr_->bindToBackhaul(vlanBindConfig, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't bind VLAN, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Requested VLAN binding" << std::endl;
        return 0;
    }

    int bindEthBackhaul() {

        telux::common::Status status;
        telux::data::net::VlanBindConfig vlanBindConfig;

        vlanBindConfig.vlanId = nad1LanVlanConfig_.vlanId;
        vlanBindConfig.bhInfo.backhaul = telux::data::BackhaulType::ETH;
        vlanBindConfig.bhInfo.vlanId = wanVlanConfig_.vlanId;


        auto respCb = std::bind(&IpPassThrough::onBindStatusAvailable,
            this, std::placeholders::_1);

        status = dataVlanMgr_->bindToBackhaul(vlanBindConfig, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't bind VLAN, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Requested VLAN binding" << std::endl;
        return 0;
    }

    int setIpConfigToVlan() {

        telux::common::ErrorCode errCode;

        userInputForIpConfig();

        errCode = dataSettingsMgr_->setIpConfig(ipConfigParams_, ipConfig_);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't assign IP to VLAN, err " <<
                static_cast<int>(errCode) << std::endl;
            return -EIO;
        }

        std::cout << "Set IP configuration to VLAN sent" << std::endl;
        return 0;
    }

    int setIpPassThrough() {

        telux::common::ErrorCode errCode;

        userInputForIpPassThrough();

        errCode = dataSettingsMgr_->setIpPassThroughConfig(ipptParams_, ipptConfig_);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set IP Passthrough state, err " <<
                static_cast<int>(errCode) << std::endl;
            return -EIO;
        }

        std::cout << "Set IP passthrough request sent" << std::endl;
        return 0;
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

    /** Step - 6 */
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
                << "\n ifMask: " << it.ifMask
                << "\n gwAddress: " << it.gwAddress
                << "\n ifMask: " << it.ifMask
                << "\n primaryDnsAddress: " << it.primaryDnsAddress
                << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << std::endl;
        }

        std::cout << " IP family type: " <<
            static_cast<int>(dataCall->getIpFamilyType()) << std::endl;
        std::cout << " Tech preference: " <<
            static_cast<int>(dataCall->getTechPreference()) << std::endl;
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

    int triggerDataCall(telux::data::OperationType opType) {
        telux::common::Status status;

        auto responseCb = std::bind(&IpPassThrough::onDataCallResponseAvailable,
            this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 5 */
        status = dataConMgr_->startDataCall(
            PROFILE_ID, telux::data::IpFamilyType::IPV4, responseCb, opType);
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

 private:
    std::mutex updateMutex_;
    std::condition_variable updateCV_;
    telux::common::ErrorCode errorCode_;
    telux::data::IpConfigParams ipConfigParams_;
    telux::data::IpConfig ipConfig_;
    telux::data::IpptParams ipptParams_;
    telux::data::IpptConfig ipptConfig_;
    telux::data::VlanConfig wanVlanConfig_;
    telux::data::VlanConfig nad1LanVlanConfig_;
    telux::data::VlanConfig nad2LanVlanConfig_;
    std::shared_ptr<telux::data::IDataCall> dataCall_;
    std::shared_ptr<telux::data::IDataSettingsManager> dataSettingsMgr_;
    std::shared_ptr<telux::data::IDataConnectionManager> dataConMgr_;
    std::shared_ptr<telux::data::net::IVlanManager> dataVlanMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<IpPassThrough> app;

    std::string nad;
    telux::data::OperationType opType;

    if (argc != 2) {
        std::cout << "./data_ip_passthrough_app_nad1 <NAD-1/NAD-2>" << std::endl;
        return -EINVAL;
    }

    nad = argv[1];
    if ((nad != "NAD-1") || (nad != "NAD-2")) {
        std::cout << " Invalid arguments, Valid: NAD-1/NAD-2" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<IpPassThrough>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate IpPassThrough" << std::endl;
        return -ENOMEM;
    }

    if (nad == "NAD-2") {
        /** Step - 1 */
        ret = app->initDataSettingsManager(opType);
        if (ret < 0) {
            return ret;
        }

        /** Step - 2 */
        ret = app->initVlanManager(opType);
        if (ret < 0) {
            return ret;
        }
        /** Step - 3 */
        ret = app->initDataConnectionManager();
        if (ret < 0) {
            return ret;
        }

        /** Step - 4 */
        ret = app->createVlan(nad);
        if (ret < 0) {
            return ret;
        }

        /** Step - 5 */
        ret = app->triggerDataCall(opType);
        if (ret < 0) {
            app->deinit();
            return ret;
        }

        /** Step - 7 */
        ret = app->bindWwanBackhaul();
        if (ret < 0) {
            return ret;
        }

        /** Step - 8 */
        ret = app->setIpPassThrough();
        if (ret < 0) {
            return ret;
        }
    }

    if (nad == "NAD-1") {
        /** Step - 9 */
        ret = app->initDataSettingsManager(opType);
        if (ret < 0) {
            return ret;
        }

        /** Step - 10 */
        ret = app->initVlanManager(opType);
        if (ret < 0) {
            return ret;
        }

        /** Step - 11 */
        ret = app->createVlan(nad);
        if (ret < 0) {
            return ret;
        }

        /** Step - 12 */
        ret = app->bindEthBackhaul();
        if (ret < 0) {
            return ret;
        }

        /** Step - 13 */
        ret = app->setIpConfigToVlan();
        if (ret < 0) {
            return ret;
        }
    }

    /** Step - 14 */
    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nIp-Passthrough app exiting" << std::endl;
    return 0;
}
