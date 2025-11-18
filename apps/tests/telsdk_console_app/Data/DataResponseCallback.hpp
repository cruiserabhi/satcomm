/*
 *  Copyright (c) 2018-2019,2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATARESPONSECALLBACK_HPP
#define DATARESPONSECALLBACK_HPP

#include <telux/tel/PhoneDefines.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfileManager.hpp>

class MyDataProfilesCallback : public telux::data::IDataProfileListCallback {
public:
   void onProfileListResponse(const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
                              telux::common::ErrorCode error) override;
};

class MyDataProfileCallback : public telux::data::IDataProfileCallback {
   void onResponse(const std::shared_ptr<telux::data::DataProfile> &profile,
                   telux::common::ErrorCode error) override;
};

class MyDataCreateProfileCallback : public telux::data::IDataCreateProfileCallback {
   void onResponse(int profileId, telux::common::ErrorCode error) override;
};

class MyDeleteProfileCallback : public telux::common::ICommandResponseCallback {
   void commandResponse(telux::common::ErrorCode error) override;
};

class MyModifyProfileCallback : public telux::common::ICommandResponseCallback {
   void commandResponse(telux::common::ErrorCode error) override;
};

class MyDataCallResponseCallback {
public:
   static void startDataCallResponseCallBack(
      const std::shared_ptr<telux::data::IDataCall> &dataCall, telux::common::ErrorCode error);
   static void stopDataCallResponseCallBack(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                                            telux::common::ErrorCode error);
   static void dataCallListResponseCb(const std::vector<std::shared_ptr<telux::data::IDataCall>> &dataCallList,
                                      telux::common::ErrorCode error);
   static void requestThrottledApnInfoCb(
      const std::vector<telux::data::APNThrottleInfo> &throttleInfoList,
         telux::common::ErrorCode error);
};

class DataCallStatisticsResponseCb {
public:
   static void requestStatisticsResponse(const telux::data::DataCallStats dCallStats,
                                         telux::common::ErrorCode error);
   static void resetStatisticsResponse(telux::common::ErrorCode error);
};

class DataFilterModeResponseCb {
public:
  static void requestDataRestrictModeResponse(telux::data::DataRestrictMode mode,
                                              telux::common::ErrorCode error);
};

// Profile List callback class to be used for DCM default profile use-case
class MyDefaultProfilesCallback : public telux::data::IDataProfileListCallback {
public:
   void onProfileListResponse(
      const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
         telux::common::ErrorCode error) override;
    std::promise<telux::common::ErrorCode> prom_{};
    std::vector<std::shared_ptr<telux::data::DataProfile>> profileList_{};
};

#endif
