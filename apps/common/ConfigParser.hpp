/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

/**
 * @brief ConfigParser class reads config file and caches the app config
 * settings. It provides utility functions to read the config values.
 */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <map>
#include <string>

#define DEFAULT_CONFIG_FILE_NAME "SampleAppConfig.conf"
#define DEFAULT_CONFIG_FILE_PATH "/etc"

/*
 * ConfigParser class caches the config settings from conf file
 * It provides utility methods to get value of a configured settings
 */
class ConfigParser {
public:
  ConfigParser(std::string configFile = DEFAULT_CONFIG_FILE_NAME,
                    std::string configFilePath = DEFAULT_CONFIG_FILE_PATH);
  ~ConfigParser();
  // Get the user defined value for configured key
  std::string getValue(std::string key);

private:
  // Function to read config file containing key value pairs
  void readConfigFile(std::string configFile);

  // Get the path where config file is located
  std::string getConfigFilePath();

  // Hashmap to store all settings as key-value pairs
  std::map<std::string, std::string> configMap_;
};

#endif // CONFIGPARSER_HPP
