/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

/**
 * @brief MsdSettings class reads config file and caches the config msdSettings
 * It provides utility functions to read the config values
 */

#ifndef MSDSETTINGS_HPP
#define MSDSETTINGS_HPP

#include <map>
#include <string>

#include <telux/tel/ECallDefines.hpp>

/*
 * MsdSettings class caches the config msdSettings from tel.conf file
 * It provides utility methods to get value of a config setting by passing the key
 */
class MsdSettings {
public:
   // Get the user defined value for any configuration setting
   static std::string getValue(std::string key);

   // Function to read msdSettings config file containing key value pairs
   static void readMsdSettingsFile();

   // Print all the key value pairs in the cache
   static void printMsdSettings();

   telux::tel::ECallMsdData readMsdFromFile(std::string filename);

   telux::tel::ECallOptionalEuroNcapData readEuroNcapOptionalAdditionalDataContent(
       std::string filename);

   void setOptionalAdditionalDataContent(std::vector<uint8_t> optionalAdditionalDataContent);
   std::vector<uint8_t> getOptionalAdditionalDataContent();

private:
   // Hashmap to store all msdSettings as key-value pairs
   static std::map<std::string, std::string> msdSettingsMap_;
   static std::string filename_;
   std::vector<uint8_t> encodedOptionalAdditionalDataContent_;
};  // end of class MsdSettings

#endif  // MSDSETTINGS_HPP
