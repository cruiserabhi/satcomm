/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef MYSERVINGSYSTEMHANDLER_HPP
#define MYSERVINGSYSTEMHANDLER_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/ServingSystemManager.hpp>

class MyRatPreferenceResponseCallback {
public:
   static void ratPreferenceResponse(telux::tel::RatPreference preference,
                                     telux::common::ErrorCode error);
};

class MyServiceDomainPrefResponseCallback {
public:
   static void serviceDomainPrefResponse(telux::tel::ServiceDomainPreference preference,
                                     telux::common::ErrorCode error);
};

class NetworkTimeResponseCallback {
public:
   static void networkTimeResponse(telux::tel::NetworkTimeInfo info,
      telux::common::ErrorCode error);
};

class RFBandInfoResponseCallback {
public:
   static void rfBandInfoResponse(telux::tel::RFBandInfo bandInfo,
      telux::common::ErrorCode error);
};

class MyServingSystemResponsecallback {
public:
   static void servingSystemResponse(telux::common::ErrorCode error);
};

class RFBandPrefResponseCallback {
public:
   static void rfBandPrefResponse(std::shared_ptr<telux::tel::IRFBandList> prefList,
      telux::common::ErrorCode error);
   static void setRFBandPrefResponse(telux::common::ErrorCode error);
};

class RFBandCapabilityResponseCallback {
public:
   static void rfBandCapabilityResponse(
       std::shared_ptr<telux::tel::IRFBandList> capabilityList,
       telux::common::ErrorCode error);
};

class MyServingSystemHelper {
public:
   static std::string getRatPreference(telux::tel::RatPreference preference);
   static std::string getServiceDomainPref(telux::tel::ServiceDomainPreference preference);
   static std::string getServiceDomain(telux::tel::ServiceDomain domain);
   static std::string getRadioTechnology(telux::tel::RadioTechnology radioTech);
   static std::string getServiceState(telux::tel::ServiceRegistrationState state);
   static std::string getEndcAvailability(telux::tel::EndcAvailability isAvailable);
   static std::string getDcnrRestriction(telux::tel::DcnrRestriction isRestricted);
   static void logNetworkInfo(telux::tel::NetworkTimeInfo info);
   static std::string RFBandtoString(telux::tel::RFBand band);
   static std::string RFBandWidthtoString(telux::tel::RFBandWidth bandWidth);
   static void logRFBandInfo(telux::tel::RFBandInfo info);
   static std::string getCallBarringType(telux::tel::CallsAllowedInCell type);
   static std::string getSmsDomain(telux::tel::SmsDomain domain);
   static std::string getNtnSmsStatus(telux::tel::NtnSmsStatus status);
   static std::string getLteCsCapability(telux::tel::LteCsCapability capability);
   static std::string gsmRFBandtoString(telux::tel::GsmRFBand gsmBand);
   static std::string wcdmaRFBandtoString(telux::tel::WcdmaRFBand wcdmaBand);
   static void logRFBandList(std::shared_ptr<telux::tel::IRFBandList> list, bool isPref);
};

class MyServingSystemListener : public telux::tel::IServingSystemListener {
public:
   void onServiceStatusChange(telux::common::ServiceStatus status) override;
   void onRatPreferenceChanged(telux::tel::RatPreference preference) override;
   void onServiceDomainPreferenceChanged(telux::tel::ServiceDomainPreference preference) override;
   void onSystemInfoChanged(telux::tel::ServingSystemInfo sysInfo) override;
   void onDcStatusChanged(telux::tel::DcStatus dcStatus) override;
   void onNetworkTimeChanged(telux::tel::NetworkTimeInfo info) override;
   void onNetworkTimeChanged(telux::tel::RadioTechnology radioTech,
       telux::tel::NetworkTimeInfo info) override;
   void onRFBandInfoChanged(telux::tel::RFBandInfo bandInfo) override;
   void onNetworkRejection(telux::tel::NetworkRejectInfo rejectInfo) override;
   void onCallBarringInfoChanged(std::vector<telux::tel::CallBarringInfo> barringInfo) override;
   void onSmsCapabilityChanged(telux::tel::SmsCapability smsCapability) override;
   void onLteCsCapabilityChanged(telux::tel::LteCsCapability lteCapability) override;
   void onRFBandPreferenceChanged(std::shared_ptr<telux::tel::IRFBandList> capabilityList) override;
};

#endif  // MYSERVINGSYSTEMHANDLER_HPP
