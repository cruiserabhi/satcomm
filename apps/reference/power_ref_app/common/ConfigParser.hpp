/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @brief ConfigParser class reads config file and caches the app config
 * settings. It provides utility functions to read the config values (key=value pair).
 */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <telux/common/Log.hpp>

extern "C"
{
#include <limits.h>
#include <unistd.h>
}
#include "define.hpp"

#ifndef DEFAULT_CONFIG_FILE_NAME
#define DEFAULT_CONFIG_FILE_NAME        "/etc/telux_power_refd.conf"
#endif

/*
 * Reference app specific config
 * ConfigParser class caches the config settings from conf file
 * It provides utility methods to get value from configuration file in key,value form.
 *
 * The ConfigParser provide section based key/value pair segregation.
 * This makes co-existing of multiple section with similar name, but different key/value pair.
 */
class ConfigParser
{
  static ConfigParser *instance;

public:
  static ConfigParser *getInstance(std::string configFile = DEFAULT_CONFIG_FILE_NAME);
  ~ConfigParser();
  // Get the user defined value for configured key
  std::string getValue(std::string section, std::string key);
  std::map<std::string, std::string> getSectionValue(std::string section);
  std::map<std::string, std::map<std::string, std::string>> getAllConfig();

private:
  ConfigParser(std::string configFile = DEFAULT_CONFIG_FILE_NAME);
  // Function to read config file containing key value pairs
  void readConfigFile(std::string configFile);
  // Get the path where config file is located
  std::string getConfigFilePath();
  // Hashmap to store all settings as key-value pairs
  std::map<std::string, std::map<std::string, std::string>> configMap_;
};

#endif // CONFIGPARSER_HPP
