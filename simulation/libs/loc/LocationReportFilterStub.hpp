/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LOCATIONREPORTFILTER_HPP
#define LOCATIONREPORTFILTER_HPP

#include <mutex>
#include <memory>
#include <map>
#include <telux/loc/LocationDefines.hpp>

using namespace telux::common;

namespace telux {
namespace loc {

enum class ReportType {
    UNKNOWN = -1,
    FUSED,
    SPE,
    PPE,
    VPE
};

class TimeWindow {
public:
    TimeWindow(uint64_t timeInterval);
    bool isInWindow(const uint64_t currentTimestamp);
    void setTimeInterval(uint64_t timeInterval);
    void resetWindow();

private:
    bool isTimeStampValid();
    void updateTimeStamp(uint64_t timestamp);
    uint32_t timeInterval_;
    uint64_t previousTimeStamp_;
    std::mutex mutex_;
};

class LocationReportFilter {
public:
    LocationReportFilter();
    Status startReportFilter(uint64_t timeInterval, ReportType reportType);
    /* returns a boolean which conveys the following:
     *      true - drop the report
     *      false - send the report
     */
    bool isReportIgnored(const uint64_t timestamp, const ReportType reportType);
    void resetAllFilters();
private:
    std::map<ReportType, std::shared_ptr<TimeWindow>> windows_;
    std::mutex mutex_;
};

} // end of namespace loc
} // end of namespace telux

#endif // end of LOCATIONREPORTFILTER_HPP
