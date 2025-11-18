/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "LocationReportFilterStub.hpp"

const uint64_t GRACE_TIME_MS = 50;

namespace telux {
namespace loc {

TimeWindow::TimeWindow(uint64_t timeInterval) {
    LOG(DEBUG, __FUNCTION__, " timeInterval: ", timeInterval);
    timeInterval_ = timeInterval;
    previousTimeStamp_ = telux::loc::UNKNOWN_TIMESTAMP;
}

bool TimeWindow::isTimeStampValid() {
    bool status = (previousTimeStamp_ != telux::loc::UNKNOWN_TIMESTAMP) ? true : false;
    return status;
}

void TimeWindow::setTimeInterval(uint64_t timeInterval) {
    LOG(DEBUG, __FUNCTION__, "timeInterval: ", timeInterval);
    std::lock_guard<std::mutex> lock(mutex_);
    timeInterval_ = timeInterval;
    return;
}

void TimeWindow::resetWindow() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    previousTimeStamp_ = telux::loc::UNKNOWN_TIMESTAMP;
    return;
}

void TimeWindow::updateTimeStamp(uint64_t timestamp) {
    previousTimeStamp_ = timestamp;
    return;
}

bool TimeWindow::isInWindow(uint64_t currentTimeStamp) {
    LOG(DEBUG, __FUNCTION__, " Current Timestamp: ", currentTimeStamp);
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t timeDifference = currentTimeStamp - previousTimeStamp_;
    uint64_t lowerBound = timeInterval_ - GRACE_TIME_MS;
    uint64_t upperBound = timeInterval_ + GRACE_TIME_MS;
    bool isFiltered = false;
    if (!isTimeStampValid()) {
        LOG(WARNING, __FUNCTION__, " Window doesn't have a valid previous timestamp");
        updateTimeStamp(currentTimeStamp);
        return isFiltered;
    }
    if ((timeDifference > lowerBound) && (timeDifference < upperBound)) {
        updateTimeStamp(currentTimeStamp);
    } else if (timeDifference > upperBound) {
        LOG(DEBUG, __FUNCTION__, " A glitch has occured, updating windows");
        updateTimeStamp(currentTimeStamp);
    } else {
        LOG(DEBUG, __FUNCTION__, " Filtering the report");
        isFiltered = true;
    }
    return isFiltered;
}

// LocationReportFilter class

LocationReportFilter::LocationReportFilter() {
}

Status LocationReportFilter::startReportFilter(uint64_t timeInterval, ReportType reportType) {
    std::lock_guard<std::mutex> lock(mutex_);
    Status status = Status::FAILED;
    if (windows_.find(reportType) == windows_.end()) {
        try {
            windows_[reportType] = std::make_shared<TimeWindow>(timeInterval);
            status = Status::SUCCESS;
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__, " TimeWindow: ", e.what());
            status = Status::NOMEMORY;
        }
    } else {
        windows_[reportType]->resetWindow();
        windows_[reportType]->setTimeInterval(timeInterval);
    }
    return status;
}

void LocationReportFilter::resetAllFilters() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto itr : windows_) {
        itr.second->resetWindow();
    }
    return;
}

bool LocationReportFilter::isReportIgnored(const uint64_t timestamp, const ReportType reportType) {
    /*
     * Acquire mutex and check if the report type exists in the map before
     * checking the timing window.
     * This check is done because if LCA invokes onLocationCb prior to onResponseCb,
     * accessing the uninitialized shared_ptr would result in a crash.
     *
     */
    std::lock_guard<std::mutex> lock(mutex_);
    if(windows_.find(reportType) != windows_.end()) {
        if (timestamp == telux::loc::UNKNOWN_TIMESTAMP) {
            LOG(ERROR, __FUNCTION__, " Unknown timestamp is reported");
            return false;
        }
        auto TimeWindow = windows_[reportType];
        bool status = TimeWindow->isInWindow(timestamp);
        return status;
    } else {
        LOG(ERROR, __FUNCTION__, " Window not yet initialized");
        return true;
    }
}

} // end of namespace loc

} // end of namespace telux