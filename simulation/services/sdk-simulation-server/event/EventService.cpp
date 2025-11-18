/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EventService.hpp"
#include "libs/common/Logger.hpp"

EventService::EventService() {
    LOG(DEBUG, __FUNCTION__);
}

EventService::~EventService() {
    LOG(DEBUG, __FUNCTION__);
}

EventService &EventService::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static EventService instance;
    return instance;
}