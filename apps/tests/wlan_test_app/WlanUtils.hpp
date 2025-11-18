/*
 * Copyright (c) 2022-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * WlanUtility helper class
 * @brief WlanUtils class performs common functions in Wlan.
 */

#ifndef WLANUTILS_HPP
#define WLANUTILS_HPP

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanFactory.hpp>
#include "../../common/utils/Utils.hpp"

class WlanUtils {
public:

   template <typename T>
   static void validateInput(T& input, std::initializer_list<T> list) {
      return Utils::validateInput(input, list);
   }
   static std::string getWlanDeviceName(telux::wlan::HwDeviceType device);
   static std::string getWlanApType(telux::wlan::ApType apType);
   static std::string getWlanId(telux::wlan::Id id);
   static std::string getStaConnectionStatus(telux::wlan::StaInterfaceStatus status);
   static std::string apAccessToString(telux::wlan::ApInterworking interworking);
   static std::string apRadioTypeToString(telux::wlan::BandType radio);
   static std::string apSecurityModeToString(telux::wlan::SecMode mode);
   static std::string apSecurityAuthToString(telux::wlan::SecAuth auth);
   static std::string apSecurityEncryptToString(telux::wlan::SecEncrypt encrypt);
   static void printAPStatus(std::vector<telux::wlan::ApStatus>& apStatus);
   static void printStaStatus(std::vector<telux::wlan::StaStatus>& staStatus);
   static void printDeviceInfo(std::vector<telux::wlan::DeviceInfo>& info);
   static void printApElementInfo(telux::wlan::ApElementInfoConfig ElementInfoConfig);
   static std::string apElementInfoAccessTypeToString(telux::wlan::NetAccessType accessType);
   static telux::wlan::Id convertIntToWlanId(int id);
   static telux::wlan::ApType convertIntToApType(int type);
   static telux::wlan::ApInterworking convertIntToInterworking(int interworking);
   static telux::wlan::SecMode convertIntToSecMode(int mode);
   static telux::wlan::SecAuth convertIntToSecAuth(int auth);
   static telux::wlan::SecEncrypt convertIntToSecEncrypt(int encrypt);
};

#endif
