/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SensorReportService.hpp"
#include "libs/common/Logger.hpp"

SensorReportService::SensorReportService() {
    LOG(DEBUG, __FUNCTION__);
}
SensorReportService::~SensorReportService() {
    LOG(DEBUG, __FUNCTION__);
}
SensorReportService &SensorReportService::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static SensorReportService instance;
    return instance;
}