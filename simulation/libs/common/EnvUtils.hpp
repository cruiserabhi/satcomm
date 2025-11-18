/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @brief EnvUtils provides utility functions to get configuration file path,
 * application name and current running application path.
 */

#ifndef ENVUTILS_HPP
#define ENVUTILS_HPP

#include <telux/common/CommonDefines.hpp>

#include <string>

#define DEFAULT_CONFIG_FILE "tel.conf"
#define DEFAULT_CONFIG_PATH "/etc/"

namespace telux {

namespace common {

/*
 * EnvUtils class provides utility methods to get current running application and
 * config file path.
 */
class EnvUtils {
 public:
    // Get current running application name.
    static std::string getCurrentAppName();

    // Get current running process path.
    static std::string getCurrentProcessPath();

 private:
    // avoid creating instance for this object
    EnvUtils() {
    }

};  // end of class EnvUtils

}  // End of namespace common

}  // End of namespace telux

#endif  // ENVUTILS_HPP
