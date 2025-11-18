/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMAL_MANAGER_IMPL_STUB_HPP
#define THERMAL_MANAGER_IMPL_STUB_HPP

#include <telux/therm/ThermalManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include "AsyncTaskQueue.hpp"
#include "SimulationManagerStub.hpp"
#include "ListenerManager.hpp"
#include "event-manager/ClientEventManager.hpp"
#include "protos/proto-src/therm_simulation.grpc.pb.h"

using thermStub::Thermal;

namespace telux {

namespace therm {

class ThermalManagerImplStub : public IThermalManager,
                              public IEventListener,
                              public SimulationManagerStub<Thermal>,
                              public std::enable_shared_from_this<ThermalManagerImplStub> {
 public:

    using SimulationManagerStub::init;

    ThermalManagerImplStub(telux::common::ProcType procType);
    ~ThermalManagerImplStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(
        std::weak_ptr<IThermalListener> listener,
        ThermalNotificationMask mask = 0xFFFF) override;
    telux::common::Status deregisterListener(
        std::weak_ptr<IThermalListener> listener,
        ThermalNotificationMask mask = 0xFFFF) override;

    std::vector<std::shared_ptr<IThermalZone>> getThermalZones() override;
    std::vector<std::shared_ptr<ICoolingDevice>> getCoolingDevices() override;
    std::shared_ptr<IThermalZone> getThermalZone(int thermalZoneId) override;
    std::shared_ptr<ICoolingDevice> getCoolingDevice(int coolingDeviceId) override;

    telux::common::Status initSyncComplete(
            telux::common::ServiceStatus srvcStatus) override;

 protected:
    telux::common::Status init();
    void createListener();
    void cleanup();
    void setInitCbDelay(uint32_t cbDelay);
    uint32_t getInitCbDelay();
    void notifyServiceStatus(telux::common::ServiceStatus srvcStatus);
    telux::common::Status registerDefaultIndications();

 private:
    uint32_t cbDelay_ = 0;
    telux::common::ProcType procType_;
    std::shared_ptr<ListenerManager<IThermalListener, ThermalNotificationMask>> listenerMgr_;
    ClientEventManager &clientEventMgr_;
    std::mutex mgrListenerMtx_;
    telux::common::AsyncTaskQueue<void> taskQ_;

    TripType getTripType(thermStub::TripPoint_TripType grpcTripType);
    TripEvent getTripEvent(thermStub::TripEvent grpcTripEvent);

    void onEventUpdate(google::protobuf::Any event);

    void handleSSREvent(google::protobuf::Any event);
    void handleOnTripEvent(google::protobuf::Any event);
    void handleCdevStateChangeEvent(google::protobuf::Any event);
    void onTeluxThermalServiceStatusChange(telux::common::ServiceStatus srvcStatus);

};
}  // namespace therm

}  // end of namespace telux

#endif  // THERMAL_MANAGER_IMPL_STUB_HPP
