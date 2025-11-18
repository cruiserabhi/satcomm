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

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

extern "C" {
#include <limits.h>
#include <unistd.h>
}

#include "ConfigParser.hpp"

/**
 * Check if a file exists.
 */
inline bool fileExists(const std::string &configFile) {
  std::ifstream f(configFile.c_str());
  return f.good();
}

ConfigParser::ConfigParser(std::string configFile, std::string confFilePath) {
  if (configMap_.size() == 0) {
    std::string configFilePath = getConfigFilePath() + "/" + configFile;
    // Check if the file is present in the same directory where the application is running
    if (fileExists(configFilePath)) {
      readConfigFile(configFilePath);
    } else {
      // Check if the file is present in the provided confFilePath
      configFilePath = confFilePath + "/" + configFile;
      if(fileExists(configFilePath)) {
         readConfigFile(configFilePath);
      } else {
         std::cout << "Config file " << configFile << " neither exists in same folder nor at "
                << configFilePath << std::endl;
      }
    }
  }
}

ConfigParser::~ConfigParser() {}

/**
 * Order of search for the key value from config file:
 * Search for key value from user supplied config file which is present under
 * current running app path.
 * else search for key value from default config file which is present under
 * current running app path.
 */
std::string ConfigParser::getValue(std::string key) {
  auto settingsIterator = configMap_.find(key);
  if (settingsIterator != configMap_.end()) {
    return settingsIterator->second;
  } else {
    return std::string(
        ""); // return an empty string when the setting is not configured.
  }
}

/**
 * Get the config file path. Config file is expected to be present in the same
 * location from where application is running.
 */
std::string ConfigParser::getConfigFilePath() {
  char path[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
  std::string fullPath = std::string(path, (count > 0) ? count : 0);
  auto const pos = fullPath.find_last_of('/');
  return fullPath.substr(0, pos);
}

/**
 * Utility function to read config file with key value pairs
 * Prepares a map of key value pairs from Key=Value format
 * Discards leading spaces, blank lines and lines starting with #
 * Removes any leading or trailing spaces around Key and Value if any.
 */
void ConfigParser::readConfigFile(std::string configFile) {

  // Create a file stream from the file name
  std::ifstream configFileStream(configFile);

  // Iterate through each parameter in the file and read the key value pairs
  std::string param;
  while (std::getline(configFileStream >> std::ws, param)) {
    // trim all carriage return \r from the end of string.
    param.erase(std::remove(param.begin(), param.end(), '\r'), param.end());
    std::string key;
    std::istringstream paramStream(param);
    if (std::getline(paramStream, key, '=')) {
      // Ignore lines starting with # character
      if (key[0] == '#') {
        continue;
      }
      key = std::regex_replace(key, std::regex(" +$"), "");
      if (key.length() > 0) {
        std::string value;
        if (std::getline(paramStream, value)) {
          value = std::regex_replace(value, std::regex("^ +| +$"), "");
          configMap_[key] = value;
        }
      }
    }
  }
}
