/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>

#include "../common/Logger.hpp"

#include "ThermalZoneImpl.hpp"

namespace telux {
namespace therm {

TripPointImpl::TripPointImpl()
   : type_(TripType::UNKNOWN)
   , temp_(INVALID_THERMAL_TEMP)
   , hysteresis_(INVALID_THERMAL_TEMP)
   , tripId_(INVALID_VALUE)
   , tZoneId_(INVALID_VALUE) {
    LOG(INFO, __FUNCTION__);
}

TripType TripPointImpl::getType() const {
    return type_;
}

int TripPointImpl::getThresholdTemp() const {
    return temp_;
}

int TripPointImpl::getHysteresis() const {
    return hysteresis_;
}

int TripPointImpl::getTripId() const {
    return tripId_;
}

int TripPointImpl::getTZoneId() const {
    return tZoneId_;
}

bool TripPointImpl::operator==(const ITripPoint &rHs) const {
    if ((getType() == rHs.getType()) && (getThresholdTemp() == rHs.getThresholdTemp())
        && (getHysteresis() == rHs.getHysteresis())) {
        return true;
    }
    return false;
}

std::string TripPointImpl::toString() {
    std::stringstream ss;
    ss << " Trip type: " << static_cast<int>(type_) << ", Trip temp: " << temp_
       << ", Hysteresis: " << hysteresis_ << ", Trip id: " << tripId_ << ", Tzone id: " << tZoneId_;
    return ss.str();
}

void TripPointImpl::setType(TripType type) {
    type_ = type;
}

void TripPointImpl::setThresholdTemp(int temp) {
    temp_ = temp;
}

void TripPointImpl::setHysteresis(int hysteresis) {
    hysteresis_ = hysteresis;
}

void TripPointImpl::setTripId(int tripId) {
    tripId_ = tripId;
}

void TripPointImpl::setTZoneId(int tZoneId) {
    tZoneId_ = tZoneId;
}

ThermalZoneImpl::ThermalZoneImpl()
   : tzSensorInstance_(INVALID_VALUE)
   , thermalZoneType_("")
   , sensorTemp_(INVALID_THERMAL_TEMP)
   , passiveTemp_(INVALID_THERMAL_TEMP) {
    LOG(DEBUG, __FUNCTION__);
}

int ThermalZoneImpl::getId() const {
    return tzSensorInstance_;
}

std::string ThermalZoneImpl::getDescription() const {
    return thermalZoneType_;
}

int ThermalZoneImpl::getCurrentTemp() const {
    return sensorTemp_;
}

int ThermalZoneImpl::getPassiveTemp() const {
    return passiveTemp_;
}

std::vector<std::shared_ptr<ITripPoint>> ThermalZoneImpl::getTripPoints() const {
    return tripInfo_;
}

std::string ThermalZoneImpl::toString() {
    std::stringstream ss;
    ss << " Tzone Id: " << tzSensorInstance_ << ", Tzone name: " << thermalZoneType_
       << ", Current temp: " << sensorTemp_ << ", Passive temp: " << passiveTemp_ << ",";
    for (auto trip : getTripPoints()) {
        ss << std::static_pointer_cast<TripPointImpl>(trip)->toString();
    }

    for (auto boundCdev : getBoundCoolingDevices()) {
        ss << "Bound cdev Id: " << boundCdev.coolingDeviceId;
        for (auto boundTrip : boundCdev.bindingInfo) {
            ss << ", Trip type: " << static_cast<int>(boundTrip->getType())
               << ", Trip temp: " << boundTrip->getThresholdTemp()
               << ", Hysteresis: " << boundTrip->getHysteresis();
        }
    }
    return ss.str();
}

std::vector<BoundCoolingDevice> ThermalZoneImpl::getBoundCoolingDevices() const {
    return boundCoolingDev_;
}

void ThermalZoneImpl::setId(int instance) {
    tzSensorInstance_ = instance;
}

void ThermalZoneImpl::setDescription(std::string type) {
    thermalZoneType_ = type;
}

void ThermalZoneImpl::setCurrentTemp(int temp) {
    sensorTemp_ = temp;
}

void ThermalZoneImpl::setPassiveTemp(int passiveTemp) {
    passiveTemp_ = passiveTemp;
}

void ThermalZoneImpl::setTripPoints(std::vector<std::shared_ptr<TripPointImpl>> tripInfo) {
    for (auto trip : tripInfo) {
        tripInfo_.emplace_back(trip);
    }
}

void ThermalZoneImpl::setBoundCoolingDevices(std::vector<BoundCoolingDevice> boundCoolingDev) {
    for (auto boundCdev : boundCoolingDev) {
        boundCoolingDev_.emplace_back(boundCdev);
    }
}

}  // end of namespace therm
}  // end of namespace telux
