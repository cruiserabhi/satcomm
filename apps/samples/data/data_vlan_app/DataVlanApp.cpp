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
 * This application demonstrates how to create VLAN and bind a VLAN
 * with a particular profile id and slot id. The steps are as follows:
 *
 * 1. Get a DataFactory instance.
 * 2. Get a IVlanManager instance from DataFactory.
 * 3. Wait for the VLAN service to become available.
 * 4. Define parameters to create VLAN and bind VLAN.
 * 5. Create VLAN with given parameters.
 * 6. Bind the VLAN with a particular profile id and slot id.
 *
 * Usage:
 * # ./vlan_sample_app <operation-type> <interface-type> <vlan-id> <slot-id> <profile-id> <is-accelerated>
 *
 * Example - ./vlan_sample_app 1 3 5 1 1 0
 * Creates remote VLAN with id 5 ECM interface with slot 1,
 * profile 1 and no acceleration.
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/VlanManager.hpp>

class VLANCreator : public std::enable_shared_from_this<VLANCreator> {
 public:
    int init(telux::data::OperationType opType) {
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

    int vlanCreate(telux::data::InterfaceType ifaceType,
        int vlanId, bool isAccelerated) {
        telux::common::Status status;
        telux::data::VlanConfig config{};

        auto respCb = std::bind(&VLANCreator::onVLANCreateStatusAvailable,
            this, std::placeholders::_1, std::placeholders::_2);

        config.iface = ifaceType;
        config.vlanId = vlanId;
        config.isAccelerated = isAccelerated;

        /* Step - 5 */
        status = dataVlanMgr_->createVlan(config, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't create VLAN, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Requested VLAN creation" << std::endl;
        return 0;
    }

    int profileBind(int profileId, int vlanId, SlotId slotId) {
        telux::common::Status status;

        auto respCb = std::bind(&VLANCreator::onBindStatusAvailable,
            this, std::placeholders::_1);

        /* Step - 6 */
        status = dataVlanMgr_->bindWithProfile(profileId, vlanId, respCb, slotId);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't bind VLAN, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Requested VLAN binding" << std::endl;
        return 0;
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

    /* Called as a response to bindWithProfile() request */
    void onBindStatusAvailable(telux::common::ErrorCode error) {
        std::cout << "onBindStatusAvailable()" << std::endl;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to bind VLAN, err" <<
                static_cast<int>(error) << std::endl;
            return;
        }

        std::cout << "VLAN binded successfully" << std::endl;
    }

 private:
    std::shared_ptr<telux::data::net::IVlanManager> dataVlanMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<VLANCreator> app;

    int vlanId;
    int profileId;
    SlotId slotId;
    bool isAccelerated;
    telux::data::OperationType opType;
    telux::data::InterfaceType ifaceType;

    if (argc != 7) {
        std::cout << "Usage: ./vlan_sample_app <operation-type> <interface-type> " <<
            "<vlan-id> <slot-id> <profile-id> <is-accelerated>" << std::endl;
        return -EINVAL;
    }

    /* Step - 4 */
    opType = static_cast<telux::data::OperationType>(std::atoi(argv[1]));
    ifaceType = static_cast<telux::data::InterfaceType>(std::atoi(argv[2]));
    vlanId = std::atoi(argv[3]);
    slotId = static_cast<SlotId>(std::atoi(argv[4]));
    profileId = std::atoi(argv[5]);
    isAccelerated = static_cast<bool>(std::atoi(argv[6]));

    try {
        app = std::make_shared<VLANCreator>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate VLANCreator" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(opType);
    if (ret < 0) {
        return ret;
    }

    ret = app->vlanCreate(ifaceType, vlanId, isAccelerated);
    if (ret < 0) {
        return ret;
    }

    std::this_thread::sleep_for(std::chrono::minutes(1));

    ret = app->profileBind(profileId, vlanId, slotId);
    if (ret < 0) {
        return ret;
    }
    /* Wait for receiving all asynchronous responses before exiting the application.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\nVLAN app exiting" << std::endl;
    return 0;
}
