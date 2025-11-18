/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <fstream>
#include <iostream>
#include <string>

extern "C" {
#include <limits.h>
#include <unistd.h>
}

#include "EnvUtils.hpp"

#include <utility>
#include "Logger.hpp"

namespace telux {

namespace common {

/**
 * Get current running process path.
 */
std::string EnvUtils::getCurrentProcessPath() {
    char path[PATH_MAX];
    ssize_t count        = readlink("/proc/self/exe", path, PATH_MAX);
    std::string fullPath = std::string(path, (count > 0) ? count : 0);
    return fullPath;
}

/**
 * Get current running application name.
 */
std::string EnvUtils::getCurrentAppName() {
    std::string path = getCurrentProcessPath();
    auto const pos   = path.find_last_of('/');
    return path.substr(pos + 1);
}

}  // end namespace common

}  // end namespace telux
