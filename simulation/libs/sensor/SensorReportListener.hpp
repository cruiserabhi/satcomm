/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SENSOR_REPORT_LISTENER_HPP
#define SENSOR_REPORT_LISTENER_HPP

#include "common/event-manager/EventManager.hpp"
#include "protos/proto-src/sensor_simulation.grpc.pb.h"

namespace telux {
namespace common {

class SensorReportListener :
    public EventManager<::sensorStub::EventDispatcherService> {

public:
    static SensorReportListener &getInstance();

private:
    SensorReportListener();
    virtual ~SensorReportListener();
};

} // end of namespace common
} // end of namespace telux

#endif //SENSOR_REPORT_LISTENER_HPP