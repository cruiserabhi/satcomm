/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef MYAPSIMPROFILEHANDLER_HPP
#define MYAPSIMPROFILEHANDLER_HPP

#include <telux/common/CommonDefines.hpp>

class MyApSimProfileCallback {
public:
    static void onResponseCallback(telux::common::ErrorCode error);
};

#endif  // MYAPSIMPROFILEHANDLER_HPP
