/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef USERUTILS_HPP
#define USERUTILS_HPP

#include <string>

class UserUtils {
 public:
    bool getYesNoFromUser(std::string choiceToDisplay);
    bool getLocalRemoteFromUser();
};

#endif // USERUTILS_HPP
