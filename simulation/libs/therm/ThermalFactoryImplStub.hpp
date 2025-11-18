/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMAL_FACTORY_IMPL_STUB_HPP
#define THERMAL_FACTORY_IMPL_STUB_HPP

#include <telux/therm/ThermalFactory.hpp>
#include "../common/FactoryHelper.hpp"
#include <map>

namespace telux {

namespace therm {

class ThermalFactoryImplStub : public ThermalFactory, public telux::common::FactoryHelper {
 public:
    static ThermalFactory &getInstance();

    virtual std::shared_ptr<IThermalManager> getThermalManager(
        telux::common::InitResponseCb callback = nullptr,
        telux::common::ProcType operType = telux::common::ProcType::LOCAL_PROC) override;

    virtual std::shared_ptr<IThermalShutdownManager> getThermalShutdownManager(
        telux::common::InitResponseCb callback = nullptr) override;

    virtual ~ThermalFactoryImplStub();

 private:
    std::shared_ptr<IThermalShutdownManager> thermalShutdownManager_;

    ThermalFactoryImplStub();
    std::map<telux::common::ProcType, std::vector<telux::common::InitResponseCb>>
        thermalManagerCallbacks_;
    std::map<telux::common::ProcType, std::weak_ptr<telux::therm::IThermalManager>>
        thermalManagerMap_;
    std::mutex thermalFactoryMutex_;
};

}  // end of namespace therm

}  // end of namespace telux

#endif  // THERMAL_FACTORY_IMPL_STUB_HPP
