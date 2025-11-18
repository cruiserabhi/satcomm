/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LOC_REPORT_LISTENER_HPP
#define LOC_REPORT_LISTENER_HPP

#include "common/event-manager/EventManager.hpp"
#include "protos/proto-src/loc_simulation.grpc.pb.h"

namespace telux {
namespace common {

class LocationReportListener :
    public EventManager<::locStub::EventDispatcherService> {

public:
    static LocationReportListener &getInstance();

private:
    LocationReportListener();
    virtual ~LocationReportListener();
};

} // end of namespace common
} // end of namespace telux

#endif //LOC_REPORT_LISTENER_HPP