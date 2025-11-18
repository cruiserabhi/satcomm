/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "LocationReportService.hpp"
#include "libs/common/Logger.hpp"

LocationReportService::LocationReportService() {
    LOG(DEBUG, __FUNCTION__);
}
LocationReportService::~LocationReportService() {
    LOG(DEBUG, __FUNCTION__);
}
LocationReportService &LocationReportService::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static LocationReportService instance;
    return instance;
}