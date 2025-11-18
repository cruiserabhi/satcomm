/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ThermalJsonImpl.hpp"
#include "../../../libs/common/JsonParser.hpp"

#define THERMAL_MANAGER_API_JSON "api/therm/IThermalManager.json"
#define THERMAL_STATE_JSON "system-info/therm/ThermalState.json"

ThermalJsonImpl::ThermalJsonImpl(){
    LOG(DEBUG, __FUNCTION__);
}

ThermalJsonImpl::~ThermalJsonImpl(){
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status ThermalJsonImpl::readJsonObjects() {
    LOG(DEBUG, __FUNCTION__, ":: State Json Path: ", THERMAL_STATE_JSON,
            " Api Json Path: ", THERMAL_MANAGER_API_JSON);

    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(thermState_, THERMAL_STATE_JSON);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return telux::common::Status::NOSUCH;
    }

    error =
        JsonParser::readFromJsonFile(thermMgrApi_, THERMAL_MANAGER_API_JSON);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return telux::common::Status::NOSUCH;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::ServiceStatus ThermalJsonImpl::initServiceStatus() {
    readJsonObjects();
    std::string srvStatus = thermMgrApi_["IThermalManager"]["IsSubsystemReady"].asString();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", srvStatus);
    std::lock_guard<std::mutex> lock(mutex_);
    serviceStatus_ = CommonUtils::mapServiceStatus(srvStatus);
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(serviceStatus_));
    return serviceStatus_;
}

int ThermalJsonImpl::getSubsystemReadyDelay() {
    readJsonObjects();
    int subSysDelay = thermMgrApi_["IThermalManager"]["IsSubsystemReadyDelay"].asInt();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemDelay: ", subSysDelay);
    return subSysDelay;
}

telux::therm::TripType ThermalJsonImpl::getTripType(std::string tripType) {
    std::transform(tripType.begin(), tripType.end(), tripType.begin(), ::toupper);
    if (tripType == "CRITICAL") {
        return telux::therm::TripType::CRITICAL;
    } else if (tripType == "HOT") {
        return telux::therm::TripType::HOT;
    } else if (tripType == "PASSIVE") {
        return telux::therm::TripType::PASSIVE;
    } else if (tripType == "ACTIVE") {
        return telux::therm::TripType::ACTIVE;
    } else if (tripType == "CONFIGURABLE_HIGH") {
        return telux::therm::TripType::CONFIGURABLE_HIGH;
    } else if (tripType == "CONFIGURABLE_LOW") {
        return telux::therm::TripType::CONFIGURABLE_LOW;
    } else {
        return telux::therm::TripType::UNKNOWN;
    }
}

telux::common::Status ThermalJsonImpl::getThermalZones(
        std::vector<std::shared_ptr<telux::therm::ThermalZoneImpl>> &tZones) {

    auto error =
        JsonParser::readFromJsonFile(thermMgrApi_, THERMAL_MANAGER_API_JSON);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return telux::common::Status::NOSUCH;
    }

    auto tzsApi = thermMgrApi_["IThermalManager"]["getThermalZones"];
    if (tzsApi["error"] != "SUCCESS") {
        return telux::common::Status::NOTALLOWED;
    }

    tZones = tZoneList_;
    return telux::common::Status::SUCCESS;
}

telux::common::Status ThermalJsonImpl::getCoolingDevices(
        std::vector<std::shared_ptr<telux::therm::CoolingDeviceImpl>> &cDevs) {

    auto error =
        JsonParser::readFromJsonFile(thermMgrApi_, THERMAL_MANAGER_API_JSON);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return telux::common::Status::NOSUCH;
    }

    auto cdevApi = thermMgrApi_["IThermalManager"]["getCoolingDevices"];
    if (cdevApi["error"] != "SUCCESS") {
        return telux::common::Status::NOTALLOWED;
    }

    cDevs = cDevList_;
    return telux::common::Status::SUCCESS;
}

telux::common::Status ThermalJsonImpl::getThermalZones() {

    auto tzs = thermState_["thermalZones"];
    for (Json::Value tz : tzs) {
        std::shared_ptr<telux::therm::ThermalZoneImpl> tZone =
            std::make_shared<telux::therm::ThermalZoneImpl>();
        tZone->setId(tz["id"].asInt());
        tZone->setDescription(tz["desc"].asString());
        tZone->setCurrentTemp(tz["temp"].asInt());
        tZone->setPassiveTemp(tz["passiveTemp"].asInt());
        std::vector<std::shared_ptr<telux::therm::TripPointImpl>> tripInfo;
        auto tps = tz["tripPoints"];
        for (Json::Value tp : tps) {
            std::shared_ptr<telux::therm::TripPointImpl> tripPoint =
                std::make_shared<telux::therm::TripPointImpl>();
            tripPoint->setType(getTripType(tp["type"].asString()));
            tripPoint->setThresholdTemp(tp["temp"].asInt());
            tripPoint->setHysteresis(tp["hyst"].asInt());
            tripPoint->setTripId(tp["id"].asInt());
            tripPoint->setTZoneId(tZone->getId());
            tripInfo.push_back(tripPoint);
        }
        tZone->setTripPoints(tripInfo);

        std::vector<telux::therm::BoundCoolingDevice> boundCoolingDevices;
        auto cds = tz["boundCoolingDevices"];
        for (Json::Value cd : cds) {
            telux::therm::BoundCoolingDevice boundCoolingDevice;
            boundCoolingDevice.coolingDeviceId = cd["id"].asInt();
            std::vector<std::shared_ptr<telux::therm::ITripPoint>> bindingInfo;
            auto bTps = cd["tripPoints"];
            for (auto bTp : bTps) {
                std::shared_ptr<telux::therm::TripPointImpl> tripPoint =
                    std::make_shared<telux::therm::TripPointImpl>();
                tripPoint->setTZoneId(tZone->getId());
                for(auto tp : tripInfo) {
                    if (tp->getTripId() == bTp["id"].asInt()) {
                        tp->setTZoneId(tZone->getId());
                        bindingInfo.push_back(tp);
                        break;
                    }
                }
            }
            boundCoolingDevice.bindingInfo = bindingInfo;
            boundCoolingDevices.push_back(boundCoolingDevice);
        }
        tZone->setBoundCoolingDevices(boundCoolingDevices);
        tZoneList_.push_back(tZone);
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status ThermalJsonImpl::getCoolingDevices() {

    auto cds = thermState_["coolingDevices"];
    for (Json::Value cd : cds) {
        std::shared_ptr<telux::therm::CoolingDeviceImpl> cDev
            = std::make_shared<telux::therm::CoolingDeviceImpl>();
        cDev->setId(cd["id"].asInt());
        cDev->setDescription(cd["desc"].asString());
        cDev->setMaxCoolingLevel(cd["maxCoolingLevel"].asInt());
        cDev->setCurrentCoolingLevel(cd["currentCoolingLevel"].asInt());
        cDevList_.push_back(cDev);
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status ThermalJsonImpl::getThermalZoneById(int tZoneId,
        std::shared_ptr<telux::therm::ThermalZoneImpl> &tz) {

    auto status = findId("getThermalZone", tZoneId);
    if (status != telux::common::Status::SUCCESS) {
        return telux::common::Status::NOTALLOWED;
    }

    auto itr = std::find_if(tZoneList_.begin(), tZoneList_.end(), [tZoneId](
                std::shared_ptr<telux::therm::ThermalZoneImpl> tz)
            { return (tz->getId() == tZoneId);
            });
    if (itr == tZoneList_.end()) {
        return telux::common::Status::FAILED;
    }

    tz = *itr;
    return telux::common::Status::SUCCESS;
}


telux::common::Status ThermalJsonImpl::getCoolingDeviceById(int cDevId,
        std::shared_ptr<telux::therm::CoolingDeviceImpl> &cDev) {

    auto status = findId("getCoolingDevice", cDevId);
    if (status != telux::common::Status::SUCCESS) {
        return telux::common::Status::NOTALLOWED;
    }

    auto itr = std::find_if(cDevList_.begin(), cDevList_.end(), [cDevId](
                std::shared_ptr<telux::therm::CoolingDeviceImpl> cd)
            { return (cd->getId() == cDevId);
            });
    if (itr == cDevList_.end()) {
        return telux::common::Status::FAILED;
    }

    cDev = *itr;
    return telux::common::Status::SUCCESS;
}

telux::common::Status ThermalJsonImpl::findId(std::string api, int id) {

    auto error =
        JsonParser::readFromJsonFile(thermMgrApi_, THERMAL_MANAGER_API_JSON);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Reading JSON File failed! " );
        return telux::common::Status::NOSUCH;
    }

    std::string status;
    auto resp = thermMgrApi_["IThermalManager"][api];

    if (resp["error"].asString() == "SUCCESS") {
        return telux::common::Status::SUCCESS;
    } else if(resp["error"].asString() == "FAILED") {
        return telux::common::Status::NOTALLOWED;
    }
    return telux::common::Status::NOTALLOWED;
}

std::map<int, int> ThermalJsonImpl::getCoolingDeviceLevel(
        int tZoneId, int tripId, int trend, int cDevId) {
    LOG(DEBUG, __FUNCTION__, "tZoneId: ", tZoneId, ", tripId: ", tripId,
            ", trend: ", trend);

    // map of cdevId and cDevNextLevel
    std::map<int, int> cDevLevels;
    auto tzs = thermState_["thermalZones"];
    for (auto tz : tzs) {
        if (tz["id"].asInt() == tZoneId) {
            auto cds = tz["boundCoolingDevices"];
            for (auto cd : cds) {
                // get cDev level/clr for specific cdev
                if (cDevId != -1) {
                    if (cd["id"].asInt() != cDevId) {
                        continue;
                    }
                }

                auto tps = cd["tripPoints"];
                for (auto tp : tps) {
                    if (tp["id"].asInt() == tripId) {
                        if (trend == TREND_RAISING) {
                            cDevLevels[cd["id"].asInt()] = tp["level"].asInt();
                        } else if (trend == TREND_DROPPING) {
                            cDevLevels[cd["id"].asInt()] = tp["clr"].asInt();
                        } else {
                            // STABLE - Don't do anything
                        }
                    }
                }
            }
            break;
        }
    }
    return cDevLevels;
}
