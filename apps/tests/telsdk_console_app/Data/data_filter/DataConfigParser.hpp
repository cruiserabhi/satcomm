/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @brief DataConfigParser class reads config file and caches the app config
 * settings. It provides utility functions to read the config values (key=value pair).
 */

#ifndef DATACONFIGPARSER_HPP
#define DATACONFIGPARSER_HPP

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>

extern "C" {
#include <limits.h>
#include <unistd.h>
}


#define DEFAULT_DATA_CONFIG_FILE_NAME "/etc/Datafilter.conf"

/*
 * DataConfigParser class caches the config settings from conf file
 * It provides utility methods to get value from configuration file in key,value form.
 *
 * The DataConfigParser provide section based key/value pair segregation.
 * This makes co-existing of muliple section with similar name, but different key/value pair.
 */
class DataConfigParser {
public:
  DataConfigParser(std::string section, std::string configFile = DEFAULT_DATA_CONFIG_FILE_NAME);
  ~DataConfigParser();
  // Get the user defined value for configured key
  std::string getValue(std::map<std::string, std::string> pairMap_, std::string key);
  std::vector< std::map < std::string, std::string>>  getFilters();

private:
  std::string section_;
  // Function to read config file containing key value pairs
  void readConfigFile(std::string configFile);
  // Get the path where config file is located
  std::string getConfigFilePath();
  // Hashmap to store all settings as key-value pairs
  std::vector<std::map<std::string, std::string>> configVector_;
};

#endif // DATACONFIGPARSER_HPP
