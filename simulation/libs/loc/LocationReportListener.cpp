/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #include "LocationReportListener.hpp"

namespace telux {
namespace common {

LocationReportListener::LocationReportListener()
: EventManager<::locStub::EventDispatcherService>(std::launch::deferred) {
    LOG(DEBUG, __FUNCTION__);
}

LocationReportListener::~LocationReportListener() {
    LOG(DEBUG, __FUNCTION__);
}

LocationReportListener &LocationReportListener::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static LocationReportListener instance;
    return instance;
}

} // end of namespace common
} // end of namespace telux