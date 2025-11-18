/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SensorReportListener.hpp"

namespace telux {
namespace common {

SensorReportListener::SensorReportListener()
: EventManager<::sensorStub::EventDispatcherService>(std::launch::deferred) {
    LOG(DEBUG, __FUNCTION__);
}

SensorReportListener::~SensorReportListener() {
    LOG(DEBUG, __FUNCTION__);
}

SensorReportListener &SensorReportListener::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static SensorReportListener instance;
    return instance;
}

} // end of namespace common
} // end of namespace telux