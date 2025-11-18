/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <unistd.h>

#include "ThermalManagerImplStub.hpp"
#include "JsonParser.hpp"
#include "common/therm/ThermalZoneImpl.hpp"
#include "common/therm/CoolingDeviceImpl.hpp"

#define DELAY 100
#define DEFAULT_DELIMITER " "
#define THERM_SSR_FILTER "thermal_ssr"
#define THERM_TRIP_FILTER "thermal_onTripChange"
#define THERM_CDEV_FILTER "thermal_onCdevChange"

namespace telux {

namespace therm {

using telux::common::Status;

ThermalManagerImplStub::ThermalManagerImplStub(ProcType procType)
   : SimulationManagerStub<Thermal>(std::string("IThermalManager"))
   , procType_(procType)
   , clientEventMgr_(ClientEventManager::getInstance()) {
    LOG(INFO, __FUNCTION__);
}

ThermalManagerImplStub::~ThermalManagerImplStub() {
    LOG(DEBUG, __FUNCTION__);
}

void ThermalManagerImplStub::createListener() {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ =
        std::make_shared<ListenerManager<IThermalListener,ThermalNotificationMask>>();
}

void ThermalManagerImplStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void ThermalManagerImplStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t ThermalManagerImplStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
    return cbDelay_;
}

Status ThermalManagerImplStub::init() {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::SUCCESS;

    try {
        createListener();
    } catch (std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ": Invalid listener instance");
        return Status::FAILED;
    }
    status = registerDefaultIndications();
    return status;
}

void ThermalManagerImplStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);
    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        // De-register all indications from server
        std::vector<std::string> filters{THERM_TRIP_FILTER, THERM_CDEV_FILTER};
        clientEventMgr_.deregisterListener(shared_from_this(), filters);
    }
    std::vector<std::weak_ptr<IThermalListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    LOG(DEBUG, __FUNCTION__, ":: Notifying thermal service status: ",
            static_cast<int>(srvcStatus), " to listeners: ", applisteners.size());
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onServiceStatusChange(srvcStatus);
        }
    }
}

ServiceStatus ThermalManagerImplStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status ThermalManagerImplStub::registerDefaultIndications() {
    Status status = Status::FAILED;
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    status = clientEventMgr_.registerListener(shared_from_this(), THERM_SSR_FILTER);
    if ((status != Status::SUCCESS) &&
        (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }
    return status;
}

Status ThermalManagerImplStub::initSyncComplete(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::FAILED;
    registerDefaultIndications();

    ThermalNotificationMask activeInd;

    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, ":: Invalid instance ");
        return Status::FAILED;
    }

    listenerMgr_->getActiveIndications(activeInd);
    LOG(DEBUG, __FUNCTION__, ":: activeInd: ", activeInd.to_string());

    if (activeInd.none()) {
        LOG(INFO, __FUNCTION__, ":: No active indications");
        return Status::SUCCESS;
    }

    if (activeInd.test(TNT_TRIP_UPDATE)) {
        status = clientEventMgr_.registerListener(shared_from_this(), THERM_TRIP_FILTER);
        if ((status != Status::SUCCESS) &&
                (status != Status::ALREADY)) {
            LOG(ERROR, __FUNCTION__, ":: Registering trip change event failed");
            return status;
        }
    }

    if (activeInd.test(TNT_CDEV_LEVEL_UPDATE)) {
        status = clientEventMgr_.registerListener(shared_from_this(), THERM_CDEV_FILTER);
        if ((status != Status::SUCCESS) &&
                (status != Status::ALREADY)) {
            LOG(ERROR, __FUNCTION__, ":: Registering cdev state change event failed");
            return status;
        }
    }

    LOG(INFO, __FUNCTION__, ":: Registering optional indications: ",
            activeInd.to_string());
    return status;
}

void ThermalManagerImplStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    // Execute all events in separate thread
    auto f = std::async(std::launch::deferred, [this, event]() {
            if (event.Is<commonStub::GetServiceStatusReply>()) {
                handleSSREvent(event);
            } else if (event.Is<::thermStub::RegisterOnTripEventReply>()) {
                handleOnTripEvent(event);
            } else if (event.Is<::thermStub::RegisterOnCoolingDeviceLevelChangeReply>()) {
                handleCdevStateChangeEvent(event);
            } else {
                LOG(ERROR, __FUNCTION__, ":: Invalid event");
    }}).share();

    taskQ_.add(f);
}

void ThermalManagerImplStub::handleSSREvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    commonStub::GetServiceStatusReply ssrResp;
    event.UnpackTo(&ssrResp);

    ServiceStatus srvcStatus = ServiceStatus::SERVICE_FAILED;

    if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_AVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_AVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_UNAVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_FAILED) {
        srvcStatus = ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(ERROR, __FUNCTION__, ":: INVALID SSR event");
        return;
    }
    setServiceReady(srvcStatus);
    onTeluxThermalServiceStatusChange(srvcStatus);
}

void ThermalManagerImplStub::handleOnTripEvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    ::thermStub::RegisterOnTripEventReply tripRes;
    event.UnpackTo(&tripRes);

    std::shared_ptr<TripPointImpl> tripPoint = nullptr;
    try {
        tripPoint = std::make_shared<TripPointImpl>();
    } catch(std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ":: Invalid instance");
        return;
    }
    auto gTp = tripRes.trip_point();
    tripPoint->setType(getTripType(gTp.trip_type()));
    tripPoint->setThresholdTemp(gTp.threshold_temp());
    tripPoint->setHysteresis(gTp.hysteresis());
    tripPoint->setTripId(gTp.trip_id());
    tripPoint->setTZoneId(gTp.tzone_id());
    {
        //Notify tripRes to the client
        std::lock_guard<std::mutex> lock(mgrListenerMtx_);
        std::vector<std::weak_ptr<IThermalListener>> applisteners;
        listenerMgr_->getAvailableListeners(TNT_TRIP_UPDATE, applisteners);
        LOG(DEBUG, __FUNCTION__, ":: Notifying thermal trip update event ",
                " to listeners: ", applisteners.size());

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                if (tripRes.has_trip_point()) {
                    sp->onTripEvent(tripPoint, getTripEvent(tripRes.trip_event()));
                }
            }
        }
    }
}

void ThermalManagerImplStub::handleCdevStateChangeEvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    thermStub::RegisterOnCoolingDeviceLevelChangeReply cdevRes;
    event.UnpackTo(&cdevRes);

    std::shared_ptr<CoolingDeviceImpl> cDev = nullptr;
    try {
        cDev = std::make_shared<CoolingDeviceImpl>();
    } catch(std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ":: Invalid instance");
        return;
    }

    auto gCdev = cdevRes.cooling_device();
    cDev->setId(gCdev.id());
    cDev->setDescription(gCdev.type());
    cDev->setMaxCoolingLevel(gCdev.max_cooling_state());
    cDev->setCurrentCoolingLevel(gCdev.current_cooling_state());
    {
        //Notify cDevEvent to the client
        std::lock_guard<std::mutex> lock(mgrListenerMtx_);
        std::vector<std::weak_ptr<IThermalListener>> applisteners;
        listenerMgr_->getAvailableListeners(TNT_CDEV_LEVEL_UPDATE, applisteners);
        LOG(DEBUG, __FUNCTION__, ":: Notifying cooling device level update event ",
                " to listeners: ", applisteners.size());

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                if (cdevRes.has_cooling_device()) {
                    sp->onCoolingDeviceLevelChange(cDev);
                }
            }
        }
    }
}

void ThermalManagerImplStub::onTeluxThermalServiceStatusChange(
        ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__, ":: Service Status: ", static_cast<int>(srvcStatus));

    if (srvcStatus == getServiceStatus()) {
        return;
    }
    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: Telux thermal Service is UNAVAILABLE");
        setServiceStatus(srvcStatus);
    } else {
        LOG(INFO, __FUNCTION__, ":: Telux thermal Service is AVAILABLE");
        auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
        taskQ_.add(f);
    }
}

Status ThermalManagerImplStub::registerListener(
    std::weak_ptr<IThermalListener> listener, ThermalNotificationMask mask) {
    grpc::Status gStatus;

    if (SimulationManagerStub::getServiceStatus() !=
            ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: thermal service is not available");
        return Status::FAILED;
    }

    std::lock_guard<std::mutex> lock(mgrListenerMtx_);
    Status status = Status::FAILED;
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, ":: Invalid instance ");
        return Status::FAILED;
    }

    // Register listener for SSR indication
    status = listenerMgr_->registerListener(listener);
    if ((status != Status::SUCCESS) &&
            ((status != Status::ALREADY))) {
        LOG(ERROR, __FUNCTION__, ":: Failed to register the ssr indications",
                ", error: ", static_cast<int>(status));
        return status;
    }

    if (mask.none()) {
        return Status::SUCCESS;
    }

    LOG(INFO, __FUNCTION__, ":: Registering optional listener, mask: ", mask.to_string());
    ThermalNotificationMask firstReg;
    status = listenerMgr_->registerListener(listener, mask, firstReg);
    if (status != Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Failed to register the optional listener, mask - ",
                mask.to_string(), ", error: ", static_cast<int>(status));
        return status;
    }

    LOG(DEBUG, __FUNCTION__, ":: firstReg: ", firstReg.to_string());

    if (firstReg.test(TNT_TRIP_UPDATE)) {
        LOG(DEBUG, __FUNCTION__, ":: Registering for trip event update");

        status = clientEventMgr_.registerListener(shared_from_this(), THERM_TRIP_FILTER);
        if ((status != Status::SUCCESS) &&
                (status != Status::ALREADY)) {
            LOG(ERROR, __FUNCTION__, ":: Registering trip change event failed");
            return status;
        }
    }

    if (firstReg.test(TNT_CDEV_LEVEL_UPDATE)) {
        LOG(DEBUG, __FUNCTION__, ":: Registering for cooling device event update");

        status = clientEventMgr_.registerListener(shared_from_this(), THERM_CDEV_FILTER);
        if ((status != Status::SUCCESS) &&
                (status != Status::ALREADY)) {
            deregisterListener(listener, firstReg);
            LOG(ERROR, __FUNCTION__, ":: Registering cdev state change event failed");
            return status;
        }
    }

    // Always success is diff than lib behaviour which is not possible in real scenario
    return Status::SUCCESS;
}

Status ThermalManagerImplStub::deregisterListener(
    std::weak_ptr<IThermalListener> listener, ThermalNotificationMask mask) {
    grpc::Status gStatus;

    if (SimulationManagerStub::getServiceStatus() !=
            ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: thermal service is not available");
        return Status::FAILED;
    }

    std::lock_guard<std::mutex> lock(mgrListenerMtx_);
    Status status = Status::FAILED;
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, ":: Invalid instance ");
        return Status::FAILED;
    }

    if (mask.all() ) {
        // De-Register listener for SSR indication
        status = listenerMgr_->deRegisterListener(listener);
        if ((status != Status::SUCCESS) &&
            (status != Status::NOSUCH)) {
            LOG(ERROR, __FUNCTION__,
                    ": Failed to de-register for SSR notifications, error: ",
                    static_cast<int>(status));
            return status;
        }
    } else if (mask.none()) {
        LOG(ERROR, __FUNCTION__, ":: Invalid mask - ", mask.to_string());
        return status;
    }

    LOG(INFO, __FUNCTION__, ":: De-registering optional listener", " mask: ",
            mask.to_string());
    ThermalNotificationMask lastDereg;
    status = listenerMgr_->deRegisterListener(listener, mask, lastDereg);
    LOG(DEBUG, __FUNCTION__, ":: lastDereg: ", lastDereg.to_string());
    if (status == Status::NOSUCH) {
        return Status::SUCCESS;
    }

    if (status != Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Failed to register for notification mask - ",
                mask.to_string(),
            ", error: ", static_cast<int>(status));
        return status;
    }

    if (lastDereg.test(TNT_TRIP_UPDATE)) {
        LOG(DEBUG, __FUNCTION__, ":: Deregistering for trip event update");
        ::commonStub::DeRegisterNotificationRequest request;
        ::google::protobuf::Empty response;
        ClientContext context;
        stub_->DeRegisterOnTripEvent(&context, request, &response);
    }

    if (lastDereg.test(TNT_CDEV_LEVEL_UPDATE)) {
        LOG(DEBUG, __FUNCTION__, ":: Deregistering for cooling device event update");
        ::commonStub::DeRegisterNotificationRequest request;
        ::google::protobuf::Empty response;
        ClientContext context;
        stub_->DeRegisterOnCoolingDeviceLevelChange(&context, request, &response);
    }

    return Status::SUCCESS;
}

TripEvent ThermalManagerImplStub::getTripEvent(thermStub::TripEvent grpcTripEvent) {
    switch (grpcTripEvent) {
        case thermStub::TripEvent::CROSSED_UNDER:
            return TripEvent::CROSSED_UNDER;
        case thermStub::TripEvent::CROSSED_OVER:
            return TripEvent::CROSSED_OVER;
        default:
            return TripEvent::NONE;
    }
}

TripType ThermalManagerImplStub::getTripType(thermStub::TripPoint_TripType grpcTripType) {
    switch (grpcTripType) {
        case thermStub::TripPoint_TripType_UNKNOWN:
            return TripType::UNKNOWN;
        case thermStub::TripPoint_TripType_CRITICAL:
            return TripType::CRITICAL;
        case thermStub::TripPoint_TripType_HOT:
            return TripType::HOT;
        case thermStub::TripPoint_TripType_PASSIVE:
            return TripType::PASSIVE;
        case thermStub::TripPoint_TripType_ACTIVE:
            return TripType::ACTIVE;
        case thermStub::TripPoint_TripType_CONFIGURABLE_HIGH:
            return TripType::CONFIGURABLE_HIGH;
        case thermStub::TripPoint_TripType_CONFIGURABLE_LOW:
            return TripType::CONFIGURABLE_LOW;
        default:
            return TripType::UNKNOWN;
    }
}


std::vector<std::shared_ptr<IThermalZone>> ThermalManagerImplStub::getThermalZones() {
    std::vector<std::shared_ptr<IThermalZone>> tZones;
    thermStub::GetThermalZonesRequest request;
    thermStub::GetThermalZonesReply response;
    ClientContext context;

    if (SimulationManagerStub::getServiceStatus() !=
            ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: thermal service is not available");
        return tZones;
    }

    request.set_oper_type(thermStub::ProcType::LOCAL_PROC);

    const grpc::Status status = stub_->GetThermalZones(&context, request, &response);
    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, ":: Server request failed, error: ", status.error_message());
        return tZones;
    }
    LOG(DEBUG, __FUNCTION__, ":: Received Thermal Zones: ", response.thermal_zones_size());

    auto grpcTzones = response.thermal_zones();
    for(const thermStub::ThermalZone &grpcTzone : grpcTzones) {
        std::shared_ptr<ThermalZoneImpl> tZone = std::make_shared<ThermalZoneImpl>();
        tZone->setId(grpcTzone.id());
        tZone->setDescription(grpcTzone.type());
        tZone->setCurrentTemp(grpcTzone.current_temp());
        tZone->setPassiveTemp(grpcTzone.passive_temp());
        std::vector<std::shared_ptr<TripPointImpl>> tripInfo;
        auto grpcTripPoints = grpcTzone.trip_points();
        for(auto grpcTripPoint : grpcTripPoints) {
            std::shared_ptr<TripPointImpl> tripPoint = std::make_shared<TripPointImpl>();
            tripPoint->setType(getTripType(grpcTripPoint.trip_type()));
            tripPoint->setThresholdTemp(grpcTripPoint.threshold_temp());
            tripPoint->setHysteresis(grpcTripPoint.hysteresis());
            tripPoint->setTripId(grpcTripPoint.trip_id());
            tripPoint->setTZoneId(grpcTripPoint.tzone_id());
            tripInfo.push_back(tripPoint);
        }
        tZone->setTripPoints(tripInfo);
        std::vector<BoundCoolingDevice> boundCoolingDevices;
        auto grpcCdevs = grpcTzone.bound_cooling_devices();
        for(auto grpcCdev : grpcCdevs) {
            BoundCoolingDevice boundCoolingDevice;
            boundCoolingDevice.coolingDeviceId = grpcCdev.cooling_device_id();
            std::vector<std::shared_ptr<ITripPoint>> bindingInfo;
            auto grpcCdevTripPoints = grpcCdev.trip_points();
            for(auto grpcCdevTripPoint : grpcCdevTripPoints) {
                std::shared_ptr<TripPointImpl> tripPoint = std::make_shared<TripPointImpl>();
                tripPoint->setType(getTripType(grpcCdevTripPoint.trip_type()));
                tripPoint->setThresholdTemp(grpcCdevTripPoint.threshold_temp());
                tripPoint->setHysteresis(grpcCdevTripPoint.hysteresis());
                tripPoint->setTripId(grpcCdevTripPoint.trip_id());
                tripPoint->setTZoneId(grpcCdevTripPoint.tzone_id());
                bindingInfo.push_back(tripPoint);
            }
            boundCoolingDevice.bindingInfo = bindingInfo;
            boundCoolingDevices.push_back(boundCoolingDevice);
        }
        tZone->setBoundCoolingDevices(boundCoolingDevices);
        tZones.push_back(tZone);
    }
    return tZones;
}

std::vector<std::shared_ptr<ICoolingDevice>> ThermalManagerImplStub::getCoolingDevices() {
    std::vector<std::shared_ptr<ICoolingDevice>> cdevs;
    thermStub::GetCoolingDevicesRequest request;
    thermStub::GetCoolingDevicesReply response;
    ClientContext context;

    if (SimulationManagerStub::getServiceStatus() !=
            ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: thermal service is not available");
        return cdevs;
    }

    request.set_oper_type(thermStub::ProcType::LOCAL_PROC);

    const grpc::Status status = stub_->GetCoolingDevices(&context, request, &response);
    LOG(DEBUG, __FUNCTION__, ":: Received Cooling devices: ",
            response.cooling_devices_size());

    auto grpcCdevs = response.cooling_devices();
    for(const thermStub::CoolingDevice &grpcCdev : grpcCdevs) {
        std::shared_ptr<CoolingDeviceImpl> cDev = std::make_shared<CoolingDeviceImpl>();
        cDev->setId(grpcCdev.id());
        cDev->setDescription(grpcCdev.type());
        cDev->setMaxCoolingLevel(grpcCdev.max_cooling_state());
        cDev->setCurrentCoolingLevel(grpcCdev.current_cooling_state());
        cdevs.push_back(cDev);
    }
    return cdevs;
}

std::shared_ptr<IThermalZone> ThermalManagerImplStub::getThermalZone(int thermalZoneId) {
    thermStub::GetThermalZoneByIdRequest request;
    thermStub::GetThermalZoneByIdReply response;
    ClientContext context;

    std::shared_ptr<ThermalZoneImpl> tZone = std::make_shared<ThermalZoneImpl>();
    if (SimulationManagerStub::getServiceStatus() !=
            ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: thermal service is not available");
        return nullptr;
    }

    request.set_id(thermalZoneId);
    request.set_oper_type(thermStub::ProcType::LOCAL_PROC);

    const grpc::Status status = stub_->GetThermalZoneById(&context, request, &response);
    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, ":: request failed");
        return tZone;
    }

    if (!response.has_thermal_zone()) {
        return nullptr;
    }

    auto grpcTz = response.thermal_zone();

    tZone->setId(grpcTz.id());
    tZone->setDescription(grpcTz.type());
    tZone->setCurrentTemp(grpcTz.current_temp());
    tZone->setPassiveTemp(grpcTz.passive_temp());
    std::vector<std::shared_ptr<TripPointImpl>> tripInfo;
    auto grpcTripPoints = grpcTz.trip_points();
    for(auto grpcTripPoint : grpcTripPoints) {
        std::shared_ptr<TripPointImpl> tripPoint = std::make_shared<TripPointImpl>();
        tripPoint->setType(getTripType(grpcTripPoint.trip_type()));
        tripPoint->setThresholdTemp(grpcTripPoint.threshold_temp());
        tripPoint->setHysteresis(grpcTripPoint.hysteresis());
        tripPoint->setTripId(grpcTripPoint.trip_id());
        tripPoint->setTZoneId(grpcTripPoint.tzone_id());
        tripInfo.push_back(tripPoint);
    }
    tZone->setTripPoints(tripInfo);
    std::vector<BoundCoolingDevice> boundCoolingDevices;
    auto grpcCdevs = grpcTz.bound_cooling_devices();
    for(auto grpcCdev : grpcCdevs) {
        BoundCoolingDevice boundCoolingDevice;
        boundCoolingDevice.coolingDeviceId = grpcCdev.cooling_device_id();
        std::vector<std::shared_ptr<ITripPoint>> bindingInfo;
        auto grpcCdevTripPoints = grpcCdev.trip_points();
        for(auto grpcCdevTripPoint : grpcCdevTripPoints) {
            std::shared_ptr<TripPointImpl> tripPoint = std::make_shared<TripPointImpl>();
            tripPoint->setType(getTripType(grpcCdevTripPoint.trip_type()));
            tripPoint->setThresholdTemp(grpcCdevTripPoint.threshold_temp());
            tripPoint->setHysteresis(grpcCdevTripPoint.hysteresis());
            tripPoint->setTripId(grpcCdevTripPoint.trip_id());
            tripPoint->setTZoneId(grpcCdevTripPoint.tzone_id());
            bindingInfo.push_back(tripPoint);
        }
        boundCoolingDevice.bindingInfo = bindingInfo;
        boundCoolingDevices.push_back(boundCoolingDevice);
    }
    tZone->setBoundCoolingDevices(boundCoolingDevices);
    return tZone;
}

std::shared_ptr<ICoolingDevice> ThermalManagerImplStub::getCoolingDevice(int coolingDeviceId) {
    thermStub::GetCoolingDeviceByIdRequest request;
    thermStub::GetCoolingDeviceByIdReply response;
    ClientContext context;

    std::shared_ptr<CoolingDeviceImpl> cDev = std::make_shared<CoolingDeviceImpl>();
    if (SimulationManagerStub::getServiceStatus() !=
            ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: thermal service is not available");
        return nullptr;
    }

    request.set_id(coolingDeviceId);
    request.set_oper_type(thermStub::ProcType::LOCAL_PROC);

    const grpc::Status status = stub_->GetCoolingDeviceById(&context, request, &response);
    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, ":: request failed");
        return cDev;
    }

    if (!response.has_cooling_device()) {
        return nullptr;
    }

    auto grpcCdev = response.cooling_device();
    cDev->setId(grpcCdev.id());
    cDev->setDescription(grpcCdev.type());
    cDev->setMaxCoolingLevel(grpcCdev.max_cooling_state());
    cDev->setCurrentCoolingLevel(grpcCdev.current_cooling_state());
    return cDev;
}

}  // end of namespace therm

}  // end of namespace telux
