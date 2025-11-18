/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COOLINGDEVICEIMPL_HPP
#define COOLINGDEVICEIMPL_HPP

#include <telux/therm/ThermalManager.hpp>

namespace telux {
namespace therm {

class CoolingDeviceImpl : public ICoolingDevice {
 public:
    CoolingDeviceImpl();
    int getId() const override;
    std::string getDescription() const override;
    int getMaxCoolingLevel() const override;
    int getCurrentCoolingLevel() const override;
    std::string toString();

    void setId(int instance);
    void setDescription(std::string type);
    void setMaxCoolingLevel(int maxCoolingLevel);
    void setCurrentCoolingLevel(int currentCoolingLevel);

 private:
    int coolingDevInstance_;
    std::string coolingDevType_;
    int maxCoolingLevel_;
    int currentCoolingLevel_;
};

}  // end of namespace therm
}  // end of namespace telux
#endif  // COOLINGDEVICEIMPL_HPP
