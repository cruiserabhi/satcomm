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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to request data profiles. The steps are as follows:
 *
 *  1. Get a DataFactory instance.
 *  2. Get a IDataProfileManager instance from DataFactory.
 *  3. Wait for the data service to become available.
 *  4. Request data profile List.
 *
 * Usage:
 * # ./data_profile_app <slot-id>
 *
 * Example: ./data_profile_app 1
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <iomanip>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>

class ProfileListGetter : public telux::data::IDataProfileListCallback,
                          public std::enable_shared_from_this<ProfileListGetter> {
 public:
    int init(SlotId slotId) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &dataFactory = telux::data::DataFactory::getInstance();

        /* Step - 2 */
        dataProfileMgr_ = dataFactory.getDataProfileManager(slotId,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!dataProfileMgr_) {
            std::cout << "Can't get IDataProfileManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Profile service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int requestProfiles() {
        telux::common::Status status;

        /* Step - 4 */
        status = dataProfileMgr_->requestProfileList(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't requets profiles, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Profiles requested" << std::endl;
        return 0;
    }

    /* Receives response of the startDataCall() request */
    void onProfileListResponse(
        const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
        telux::common::ErrorCode error) override {

        std::cout << "\nonProfileListResponse()" << std::endl;

        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to get profiles, err " <<
                static_cast<int>(error) << std::endl;
        }

        std::cout << std::setw(2)
        << "+-----------------------------------------------------------------+"
        << std::endl;
        std::cout << std::setw(14) << "| Profile # | " << std::setw(11) << "TechPref | "
        << std::setw(15) << "      APN      " << std::setw(17) << "|  ProfileName  |"
        << std::setw(10) << " IP Type |" << std::endl;
        std::cout << std::setw(2)
        << "+-----------------------------------------------------------------+"
        << std::endl;
        for(auto it : profiles) {
            std::cout << std::left << std::setw(4) << "  " << std::setw(10) << it->getId()
            << std::setw(11) << techPreferenceToString(it->getTechPreference())
            << std::setw(15) << it->getApn() << std::setw(17) << it->getName()
            << std::setw(10) << ipFamilyTypeToString(it->getIpFamilyType()) << std::endl;
        }
   }

 private:
    std::string techPreferenceToString(telux::data::TechPreference techPref) {
        switch(techPref) {
            case telux::data::TechPreference::TP_3GPP:
                return "3gpp";
            case telux::data::TechPreference::TP_3GPP2:
                return "3gpp2";
            case telux::data::TechPreference::TP_ANY:
            default:
                return "Any";
        }
    }

    std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType) {
        switch(ipType) {
            case telux::data::IpFamilyType::IPV4:
                return "IPv4";
            case telux::data::IpFamilyType::IPV6:
                return "IPv6";
            case telux::data::IpFamilyType::IPV4V6:
                return "IPv4v6";
            case telux::data::IpFamilyType::UNKNOWN:
            default:
                return "NA";
        }
    }

    std::shared_ptr<telux::data::IDataProfileManager> dataProfileMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ProfileListGetter> app;

    SlotId slotId;

    if (argc != 2) {
        std::cout << "Usage: ./data_profile_app <slot-id>" << std::endl;
        return -EINVAL;
    }

    slotId = static_cast<SlotId>(std::atoi(argv[1]));

    try {
        app = std::make_shared<ProfileListGetter>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate ProfileListGetter" << std::endl;
        return -ENOMEM;
    }

    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    ret = app->requestProfiles();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses before exiting the application.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "\nData profile app exiting" << std::endl;
    return 0;
}
