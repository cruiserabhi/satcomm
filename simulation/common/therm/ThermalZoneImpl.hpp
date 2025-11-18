/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMALZONEIMPL_HPP
#define THERMALZONEIMPL_HPP

#include <telux/therm/ThermalManager.hpp>

#define INVALID_THERMAL_TEMP -274000
#define INVALID_VALUE -1

namespace telux {
namespace therm {

class TripPointImpl : public ITripPoint {
 public:
    TripPointImpl();
    TripType getType() const override;
    int getThresholdTemp() const override;
    int getHysteresis() const override;
    int getTripId() const override;
    int getTZoneId() const override;
    std::string toString();

    void setType(TripType type);
    void setThresholdTemp(int temp);
    void setHysteresis(int hysteresis);
    void setTripId(int tripId);
    void setTZoneId(int tZoneId);

    bool operator==(const ITripPoint &rHs) const override;

 private:
    TripType type_;
    int temp_;
    int hysteresis_;
    int tripId_;
    int tZoneId_;
};

class ThermalZoneImpl : public IThermalZone {
 public:
    ThermalZoneImpl();
    int getId() const override;
    std::string getDescription() const override;
    int getCurrentTemp() const override;
    int getPassiveTemp() const override;
    std::vector<std::shared_ptr<ITripPoint>> getTripPoints() const override;
    std::vector<BoundCoolingDevice> getBoundCoolingDevices() const override;
    std::string toString();

    void setId(int instance);
    void setDescription(std::string type);
    void setCurrentTemp(int temp);
    void setPassiveTemp(int passiveTemp);
    void setTripPoints(std::vector<std::shared_ptr<TripPointImpl>> tripInfo);
    void setBoundCoolingDevices(std::vector<BoundCoolingDevice> boundCoolingDev);

 private:
    int tzSensorInstance_;
    std::string thermalZoneType_;
    int sensorTemp_;
    int passiveTemp_;
    std::vector<std::shared_ptr<ITripPoint>> tripInfo_;
    std::vector<BoundCoolingDevice> boundCoolingDev_;
};

}  // end of namespace therm
}  // end of namespace telux
#endif  // THERMALZONEIMPL_HPP
