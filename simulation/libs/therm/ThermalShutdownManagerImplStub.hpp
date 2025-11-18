/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMALSHUTDOWNMANAGERIMPL_STUB_HPP
#define THERMALSHUTDOWNMANAGERIMPL_STUB_HPP

#include <telux/therm/ThermalDefines.hpp>
#include <telux/therm/ThermalShutdownManager.hpp>

namespace telux {
namespace therm {

class ThermalShutdownManagerImplStub : public IThermalShutdownManager,
                                       public std::enable_shared_from_this<ThermalShutdownManagerImplStub> {
 public:

    bool isReady() override;
    std::future<bool> onReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(
        std::weak_ptr<IThermalShutdownListener> listener) override;
    telux::common::Status deregisterListener(
        std::weak_ptr<IThermalShutdownListener> listener) override;
    telux::common::Status setAutoShutdownMode(AutoShutdownMode mode,
        telux::common::ResponseCallback callback = nullptr,
        uint32_t timeout                         = DEFAULT_TIMEOUT) override;
    telux::common::Status getAutoShutdownMode(GetAutoShutdownModeResponseCb callback) override;

    ThermalShutdownManagerImplStub();
    ~ThermalShutdownManagerImplStub();

 private:
    ThermalShutdownManagerImplStub(ThermalShutdownManagerImplStub const &)            = delete;
    ThermalShutdownManagerImplStub &operator=(ThermalShutdownManagerImplStub const &) = delete;

};

}  // end of namespace therm
}  // end of namespace telux

#endif  // THERMALSHUTDOWNMANAGERIMPL_STUB_HPP
