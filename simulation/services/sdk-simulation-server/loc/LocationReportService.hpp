/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LOC_REPORT_SERVER_HPP
#define LOC_REPORT_SERVER_HPP

#include "event/EventServiceHelper.hpp"
#include "protos/proto-src/loc_simulation.grpc.pb.h"

class LocationReportService final:
    public EventServiceHelper<::locStub::EventDispatcherService> {
    /**
    * @brief This class acts as the report event service for the location framework
    *        on the server side & is responsible for forwarding the reports
    *        to the eventManager on the client side via writing to the stream.
    */
public:
    static LocationReportService &getInstance();

private:
    LocationReportService();
    ~LocationReportService();
};

#endif // LOC_REPORT_SERVER_HPP