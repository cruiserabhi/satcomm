/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "CoolingDeviceImpl.hpp"

#define INVALID -1

namespace telux {
namespace therm {

CoolingDeviceImpl::CoolingDeviceImpl()
   : coolingDevInstance_(INVALID)
   , coolingDevType_("")
   , maxCoolingLevel_(INVALID)
   , currentCoolingLevel_(INVALID) {
    LOG(INFO, __FUNCTION__);
}

int CoolingDeviceImpl::getId() const {
    return coolingDevInstance_;
}

std::string CoolingDeviceImpl::getDescription() const {
    return coolingDevType_;
}

int CoolingDeviceImpl::getMaxCoolingLevel() const {
    return maxCoolingLevel_;
}

int CoolingDeviceImpl::getCurrentCoolingLevel() const {
    return currentCoolingLevel_;
}

std::string CoolingDeviceImpl::toString() {
    std::stringstream ss;
    ss << " cdev Id: " << coolingDevInstance_ << ", cdev name: " << coolingDevType_
       << ", max cooling level: " << maxCoolingLevel_
       << ", cur cooling level: " << currentCoolingLevel_;
    return ss.str();
}

void CoolingDeviceImpl::setId(int instance) {
    coolingDevInstance_ = instance;
}

void CoolingDeviceImpl::setDescription(std::string type) {
    coolingDevType_ = type;
}

void CoolingDeviceImpl::setMaxCoolingLevel(int maxCoolingLevel) {
    maxCoolingLevel_ = maxCoolingLevel;
}

void CoolingDeviceImpl::setCurrentCoolingLevel(int currentCoolingLevel) {
    currentCoolingLevel_ = currentCoolingLevel;
}

}  // end of namespace therm
}  // end of namespace telux
