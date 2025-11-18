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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:

 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * This is a Profile Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to set create, delete, modify, and query modem profiles
 */

#ifndef DATAPROFILEMENU_HPP
#define DATAPROFILEMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>

#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>
#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

#include "../DataListener.hpp"
#include "../DataResponseCallback.hpp"
#include "../MyProfileListener.hpp"

using namespace telux::data;
using namespace telux::common;

class DataProfileMenu : public ConsoleApp {
public:
    // initialize Profile Managers
    bool init();
    // Display Profile Management Menu
    bool displayMenu();

    // Profile Management APIs
    void requestProfileList(std::vector<std::string> inputCommand);
    void createProfile(std::vector<std::string> inputCommand);
    void deleteProfile(std::vector<std::string> inputCommand);
    void modifyProfile(std::vector<std::string> inputCommand);
    void queryProfile(std::vector<std::string> inputCommand);
    void requestProfileById(std::vector<std::string> inputCommand);

    DataProfileMenu(std::string appName, std::string cursor);
    //Initialization Callback
    void onInitCompleted(telux::common::ServiceStatus status);
    ~DataProfileMenu();
private:
    bool initDataProfileManagerAndListener(SlotId slotId);
    bool initalizeDCM(SlotId slotId);
    void getProfileParamsFromUser();
    bool isDefaultProfile(SlotId slotId, int profileId);
    int getDefaultProfile(SlotId slotId, telux::data::OperationType opr);
    ApnTypes getApnMask();

    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::map<SlotId, std::shared_ptr<telux::data::IDataProfileManager>> dataProfileManagerMap_;
    std::map<SlotId, std::shared_ptr<telux::data::IDataConnectionManager>>dataConnectionManagerMap_;
    std::map<SlotId, std::shared_ptr<MyDataProfilesCallback>> myDataProfileListCb_;
    std::map<SlotId, std::shared_ptr<MyDataProfilesCallback>> myDataProfileListCbForQuery_;
    std::map<SlotId, std::shared_ptr<MyDataProfileCallback>> myDataProfileCb_;
    std::map<SlotId, std::shared_ptr<MyDataCreateProfileCallback>> myDataCreateProfileCb_;
    std::map<SlotId, std::shared_ptr<MyDataProfileCallback>> myDataProfileCbForGetProfileById_;
    std::map<SlotId, std::shared_ptr<MyDeleteProfileCallback>> myDeleteProfileCb_;
    std::map<SlotId, std::shared_ptr<MyModifyProfileCallback>> myModifyProfileCb_;
    std::map<SlotId, std::shared_ptr<MyProfileListener>> profileListeners_;
    telux::data::ProfileParams params_;
};
#endif
