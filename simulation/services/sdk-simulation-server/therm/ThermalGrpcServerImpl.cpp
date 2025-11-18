/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ThermalGrpcServerImpl.hpp"

uint16_t ThermalGrpcServerImpl::OnCdevLevelChngNotifyCnt_ = 0;
uint16_t ThermalGrpcServerImpl::OnTripEvntNotifyCnt_ = 0;

#define THERM_SSR_FILTER "thermal_ssr"
#define THERM_TRIP_FILTER "thermal_onTripChange"
#define THERM_CDEV_FILTER "thermal_onCdevChange"

ThermalGrpcServerImpl::ThermalGrpcServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    try {
        jsonHelper_ = std::make_shared<ThermalJsonImpl>();
    } catch (std::bad_alloc &e) {
        LOG(DEBUG, __FUNCTION__, ":: Invalid object ! ");
    }

    auto status = jsonHelper_->readJsonObjects();
    if (status != telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, ":: reading of json failed");
    }
}

ThermalGrpcServerImpl::~ThermalGrpcServerImpl() {
}

void ThermalGrpcServerImpl::onSSREvent(telux::common::ServiceStatus srvStatus) {
    commonStub::GetServiceStatusReply ssrResp;
    ::eventService::EventResponse anyResponse;
    setResponse(srvStatus, &ssrResp);
    anyResponse.set_filter(THERM_SSR_FILTER);
    anyResponse.mutable_any()->PackFrom(ssrResp);
    clientEvent_.updateEventQueue(anyResponse);
}

grpc::Status ThermalGrpcServerImpl::InitService(ServerContext* context,
        const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    grpc::Status gStatus = grpc::Status::OK;
    telux::common::Status status = telux::common::Status::SUCCESS;

    status = registerDefaultIndications();
    if (status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::CANCELLED,
                ":: Could not register indication with EventMgr");
    }

    telux::common::ServiceStatus srvStatus = jsonHelper_->initServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));

    setServiceStatus(srvStatus);

    status = jsonHelper_->getThermalZones();
    if (status != telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, ":: Init of thermal zones failed");
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: init failed");
    }

    status = jsonHelper_->getCoolingDevices();
    if (status != telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, ":: Init of cooling devices failed");
        return grpc::Status(grpc::StatusCode::CANCELLED, ":: init failed");
    }

    return setResponse(srvStatus,response);
}

grpc::Status ThermalGrpcServerImpl::GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ServiceStatus srvStatus = getServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));

    return setResponse(srvStatus,response);
}

grpc::Status ThermalGrpcServerImpl::GetThermalZones(ServerContext* context,
        const ::thermStub::GetThermalZonesRequest* request,
        ::thermStub::GetThermalZonesReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::shared_ptr<telux::therm::ThermalZoneImpl>> tZones;

//    auto status = getThermalZones(response);
    auto status = jsonHelper_->getThermalZones(tZones);
    if (status == telux::common::Status::NOTALLOWED) {
        // Response status is set as ERROR
        return grpc::Status::OK;
    } else if (status != telux::common::Status::SUCCESS) {
        // tZoness not found
        return grpc::Status::OK;
    }

    for (auto tz : tZones) {
        ::thermStub::ThermalZone &grpcTz = *response->add_thermal_zones();
        grpcTz.set_id(tz->getId());
        grpcTz.set_type(tz->getDescription());
        grpcTz.set_current_temp(tz->getCurrentTemp());
        grpcTz.set_passive_temp(tz->getPassiveTemp());
        auto tps = tz->getTripPoints();
        for (auto tp : tps) {
            ::thermStub::TripPoint *grpcTripPoint = grpcTz.add_trip_points();
            grpcTripPoint->set_trip_type(getTripType(tp->getType()));
            grpcTripPoint->set_threshold_temp(tp->getThresholdTemp());
            grpcTripPoint->set_hysteresis(tp->getHysteresis());
            grpcTripPoint->set_trip_id(tp->getTripId());
            grpcTripPoint->set_tzone_id(tp->getTZoneId());
        }

        auto cds = tz->getBoundCoolingDevices();
        for (auto cd : cds) {
            ::thermStub::BoundCoolingDevice *grpcbCdev = grpcTz.add_bound_cooling_devices();
            grpcbCdev->set_cooling_device_id(cd.coolingDeviceId);
            auto bTps = cd.bindingInfo;
            for (auto bTp : bTps) {
                ::thermStub::TripPoint *grpcTripPoint = grpcbCdev->add_trip_points();
                grpcTripPoint->set_trip_type(getTripType(bTp->getType()));
                grpcTripPoint->set_threshold_temp(bTp->getThresholdTemp());
                grpcTripPoint->set_hysteresis(bTp->getHysteresis());
                grpcTripPoint->set_trip_id(bTp->getTripId());
                grpcTripPoint->set_tzone_id(bTp->getTZoneId());
            }
        }
    }

    return grpc::Status::OK;
}

grpc::Status ThermalGrpcServerImpl::GetCoolingDevices(ServerContext* context,
        const ::thermStub::GetCoolingDevicesRequest* request,
        ::thermStub::GetCoolingDevicesReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::shared_ptr<telux::therm::CoolingDeviceImpl>> cDevs;

    auto status = jsonHelper_->getCoolingDevices(cDevs);
    if (status == telux::common::Status::NOTALLOWED) {
        // Response status is set as ERROR
        return grpc::Status::OK;
    } else if (status != telux::common::Status::SUCCESS) {
        // cDevs not found
        return grpc::Status::OK;
    }

    for (auto cd : cDevs) {
        ::thermStub::CoolingDevice &grpcCdev = *response->add_cooling_devices();
        grpcCdev.set_id(cd->getId());
        grpcCdev.set_type(cd->getDescription());
        grpcCdev.set_max_cooling_state(cd->getMaxCoolingLevel());
        grpcCdev.set_current_cooling_state(cd->getCurrentCoolingLevel());
    }

    return grpc::Status::OK;
}

grpc::Status ThermalGrpcServerImpl::GetThermalZoneById(ServerContext* context,
        const ::thermStub::GetThermalZoneByIdRequest* request,
        ::thermStub::GetThermalZoneByIdReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::shared_ptr<telux::therm::ThermalZoneImpl> tz;
    auto status = jsonHelper_->getThermalZoneById(request->id(), tz);
    if (status == telux::common::Status::NOTALLOWED) {
        // Response status is set as ERROR
        return grpc::Status::OK;
    } else if (status != telux::common::Status::SUCCESS) {
        // tZoneId not found
        return grpc::Status::OK;
    }


    ::thermStub::ThermalZone grpcTz;
    grpcTz.set_id(tz->getId());
    grpcTz.set_type(tz->getDescription());
    grpcTz.set_current_temp(tz->getCurrentTemp());
    grpcTz.set_passive_temp(tz->getPassiveTemp());
    auto tps = tz->getTripPoints();
    for (auto tp : tps) {
        ::thermStub::TripPoint *grpcTripPoint = grpcTz.add_trip_points();
        grpcTripPoint->set_trip_type(getTripType(tp->getType()));
        grpcTripPoint->set_threshold_temp(tp->getThresholdTemp());
        grpcTripPoint->set_hysteresis(tp->getHysteresis());
        grpcTripPoint->set_trip_id(tp->getTripId());
        grpcTripPoint->set_tzone_id(tp->getTZoneId());
    }

    auto cds = tz->getBoundCoolingDevices();
    for (auto cd : cds) {
        ::thermStub::BoundCoolingDevice *grpcbCdev = grpcTz.add_bound_cooling_devices();
        grpcbCdev->set_cooling_device_id(cd.coolingDeviceId);
        auto bTps = cd.bindingInfo;
        for (auto bTp : bTps) {
            ::thermStub::TripPoint *grpcTripPoint = grpcbCdev->add_trip_points();
            grpcTripPoint->set_trip_type(getTripType(bTp->getType()));
            grpcTripPoint->set_threshold_temp(bTp->getThresholdTemp());
            grpcTripPoint->set_hysteresis(bTp->getHysteresis());
            grpcTripPoint->set_trip_id(bTp->getTripId());
            grpcTripPoint->set_tzone_id(bTp->getTZoneId());
        }
    }

    *response->mutable_thermal_zone() = grpcTz;

    return grpc::Status::OK;
}

grpc::Status ThermalGrpcServerImpl::GetCoolingDeviceById(ServerContext* context,
        const ::thermStub::GetCoolingDeviceByIdRequest* request,
        ::thermStub::GetCoolingDeviceByIdReply* response) {
    LOG(DEBUG, __FUNCTION__);

    std::shared_ptr<telux::therm::CoolingDeviceImpl> cd;
    auto status = jsonHelper_->getCoolingDeviceById(request->id(), cd);

    if (status == telux::common::Status::NOTALLOWED) {
        // Response status is set as ERROR
        return grpc::Status::OK;
    } else if (status != telux::common::Status::SUCCESS) {
        // cdevId not found
        return grpc::Status::OK;
    }


    ::thermStub::CoolingDevice grpcCdev;
    grpcCdev.set_id(cd->getId());
    grpcCdev.set_type(cd->getDescription());
    grpcCdev.set_max_cooling_state(cd->getMaxCoolingLevel());
    grpcCdev.set_current_cooling_state(cd->getCurrentCoolingLevel());

    *response->mutable_cooling_device() = grpcCdev;

    return grpc::Status::OK;
}

grpc::Status ThermalGrpcServerImpl::RegisterOnCoolingDeviceLevelChange(
        ServerContext* context,
        const google::protobuf::Empty* request,
        ServerWriter<::thermStub::RegisterOnCoolingDeviceLevelChangeReply>* response) {
    LOG(DEBUG, __FUNCTION__);

    auto status = registerCdevStateChangeEvent(response);
    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, ":: Failed to register cdev state change event !");
        return grpc::Status(grpc::StatusCode::INTERNAL,
                ":: Failed to register cdev state change event !");
    }

    return status;
}

grpc::Status ThermalGrpcServerImpl::DeRegisterOnCoolingDeviceLevelChange(
        ServerContext* context,
        const ::commonStub::DeRegisterNotificationRequest* request,
        google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);

    auto status = deRegisterCdevStateChangeEvent(request->client_id());
    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, ":: Failed to de-register cdev state change event !");
        return grpc::Status(grpc::StatusCode::INTERNAL,
                ":: Failed to de-register cdev state change event !");
    }

    return status;
}

grpc::Status ThermalGrpcServerImpl::RegisterOnTripEvent(
        ServerContext* context,
        const google::protobuf::Empty* request,
        ServerWriter<::thermStub::RegisterOnTripEventReply>* response) {
    LOG(DEBUG, __FUNCTION__);

    auto status = registerTripEvent(response);
    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, ":: Failed to register trip event !");
        return grpc::Status(grpc::StatusCode::INTERNAL,
                ":: Failed to register trip change event !");
    }

    return status;
}

grpc::Status ThermalGrpcServerImpl::DeRegisterOnTripEvent(ServerContext* context,
        const ::commonStub::DeRegisterNotificationRequest* request,
        google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);

    auto status = deRegisterTripEvent(request->client_id());
    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, ":: Failed to de-register trip event !");
        return grpc::Status(grpc::StatusCode::INTERNAL,
                ":: Failed to de-register trip change event !");
    }

    return status;
}

/* ################################# Helper Methods ############################### */

grpc::Status ThermalGrpcServerImpl::setResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response) {
    auto subSysDelay =  jsonHelper_->getSubsystemReadyDelay();

    switch(srvStatus) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
            break;
        default:
            LOG(ERROR, __FUNCTION__, ":: Invalid service status");
            return grpc::Status(grpc::StatusCode::CANCELLED, ":: set service status failed");
    }
    response->set_delay(subSysDelay);
    return grpc::Status::OK;
}

::thermStub::TripPoint_TripType ThermalGrpcServerImpl::getTripType(telux::therm::TripType tripType) {
    if (tripType == telux::therm::TripType::CRITICAL) {
        return ::thermStub::TripPoint_TripType_CRITICAL;
    } else if (tripType == telux::therm::TripType::HOT) {
        return ::thermStub::TripPoint_TripType_HOT;
    } else if (tripType == telux::therm::TripType::PASSIVE) {
        return ::thermStub::TripPoint_TripType_PASSIVE;
    } else if (tripType == telux::therm::TripType::ACTIVE) {
        return ::thermStub::TripPoint_TripType_ACTIVE;
    } else if (tripType == telux::therm::TripType::CONFIGURABLE_HIGH) {
        return ::thermStub::TripPoint_TripType_CONFIGURABLE_HIGH;
    } else if (tripType == telux::therm::TripType::CONFIGURABLE_LOW) {
        return ::thermStub::TripPoint_TripType_CONFIGURABLE_LOW;
    } else {
        return ::thermStub::TripPoint_TripType_UNKNOWN;
    }
}

telux::common::Status ThermalGrpcServerImpl::sendTripUpdateEvent(
        std::shared_ptr<telux::therm::ITripPoint> &tp, int tZoneId, int event) {
    LOG(DEBUG, __FUNCTION__, "\n\n ************** TRIP-UPDATE ************** " );
    LOG(DEBUG, __FUNCTION__, ":: tZoneId: ", tZoneId, ", tripId: ", tp->getTripId(),
            " EVENT: ", ((event == CROSSED_OVER) ? "CROSSED_OVER" : "CROSSED_UNDER"));

    thermStub::RegisterOnTripEventReply reply;
    ::eventService::EventResponse anyResponse;
    thermStub::TripPoint t_point;

    t_point.set_trip_type(static_cast<thermStub::TripPoint_TripType>(tp->getType()));
    t_point.set_threshold_temp(tp->getThresholdTemp());
    t_point.set_hysteresis(tp->getHysteresis());
    t_point.set_trip_id(tp->getTripId());
    t_point.set_tzone_id(tZoneId);
    *reply.mutable_trip_point() = t_point;
    reply.set_trip_event(static_cast<thermStub::TripEvent>(event));

    anyResponse.set_filter(THERM_TRIP_FILTER);
    anyResponse.mutable_any()->PackFrom(reply);
    clientEvent_.updateEventQueue(anyResponse);
    return telux::common::Status::SUCCESS;
}

telux::common::Status ThermalGrpcServerImpl::sendCdevUpdateEvent(
        std::shared_ptr<telux::therm::CoolingDeviceImpl> &cd, unsigned int newState) {
    LOG(DEBUG, __FUNCTION__, "\n\n ************** CDEV-UPDATE ************** " );
    LOG(DEBUG, __FUNCTION__, ":: cDevId: ", cd->getId(), ", newState: ", newState);

    thermStub::RegisterOnCoolingDeviceLevelChangeReply reply;
    ::eventService::EventResponse anyResponse;
    thermStub::CoolingDevice device;

    device.set_id(cd->getId());
    device.set_type(cd->getDescription());
    device.set_max_cooling_state(cd->getMaxCoolingLevel());
    device.set_current_cooling_state(newState);
    *reply.mutable_cooling_device() = device;

    anyResponse.set_filter(THERM_CDEV_FILTER);
    anyResponse.mutable_any()->PackFrom(reply);
    clientEvent_.updateEventQueue(anyResponse);
    return telux::common::Status::SUCCESS;
}


telux::common::Status ThermalGrpcServerImpl::getNewCdevStateUpdate(int trend, int tZoneId,
        int tripId) {
    telux::common::Status rStatus = telux::common::Status::SUCCESS;
    LOG(DEBUG, __FUNCTION__, ":: tZoneId: ", tZoneId, ", tripId: ", tripId);

    // Map of cdevId and cDevNextLevel
    std::map<int, int> cDevs;

    switch(trend) {
        case TREND_RAISING: {
            LOG(DEBUG, __FUNCTION__);
            cDevs = jsonHelper_->getCoolingDeviceLevel(tZoneId, tripId, TREND_RAISING);
            break;
        }
        case TREND_DROPPING: {
            LOG(DEBUG, __FUNCTION__);
            cDevs = jsonHelper_->getCoolingDeviceLevel(tZoneId, tripId, TREND_DROPPING);
            break;
        }
        default:
            return telux::common::Status::SUCCESS;
    }

    if (cDevs.empty()) {
        LOG(DEBUG, __FUNCTION__, ":: No bound cooling devices ",
                ", tripId: ", tripId, ", tZoneId: ", tZoneId);
        return telux::common::Status::NOTALLOWED;
    }

    for(auto cDev : cDevs) {
        auto cd = setCoolingDevice(tZoneId, tripId, cDev.first, trend, cDev.second);
        if (cd->getId() != -1) {
            rStatus = sendCdevUpdateEvent(cd, cDev.second);
            if (rStatus != telux::common::Status::SUCCESS) {
                LOG(ERROR, __FUNCTION__,
                        ":: sending cooling device state change event failed" );
            }
        }
    }
    return rStatus;
}

telux::common::Status ThermalGrpcServerImpl::getTripUpdate(int prevTemp, int newTemp,
        int tZoneId, std::shared_ptr<telux::therm::ITripPoint> &tp) {
    LOG(DEBUG, __FUNCTION__, ":: tZoneId: ", tZoneId,
            ", prevTemp: ", prevTemp, ", newTemp: ", newTemp);
    telux::common::Status status = telux::common::Status::ALREADY;

    auto tripTemp     = tp->getThresholdTemp();
    auto tripHystTemp = tp->getHysteresis();
    auto tripId       = tp->getTripId();

    // Generate Trip update event
    if ((prevTemp < tripTemp) && (newTemp >= tripTemp)) {
        // CROSSED_OVER
        LOG(DEBUG, __FUNCTION__, ":: prevTemp: ", prevTemp,
            ", tripTemp: ", tripTemp, ", newTemp: ", newTemp,
            ", tripId: ", tripId);

        status = sendTripUpdateEvent(tp, tZoneId, CROSSED_OVER);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, ":: sending trip update event failed" );
        }
        getNewCdevStateUpdate(TREND_RAISING, tZoneId, tripId);
    } else if ( (newTemp < (tripTemp - tripHystTemp)) &&
                (prevTemp >= (tripTemp - tripHystTemp)) ) {
        // CROSSED_UNDER
        LOG(DEBUG, __FUNCTION__, ":: prevTemp: ", prevTemp,
            ", tripTemp: ", tripTemp, ", newTemp: ", newTemp,
            ", tripHystTemp: ", tripHystTemp,
            ", tripId: ", tripId);

        status = sendTripUpdateEvent(tp, tZoneId, CROSSED_UNDER);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, ":: sending trip update event failed" );
        }
        getNewCdevStateUpdate(TREND_DROPPING, tZoneId, tripId);
    } else {
        // STABLE - don't do anything
        LOG(DEBUG, __FUNCTION__, ":: prevTemp: ", prevTemp,
            ", tripTemp: ", tripTemp, ", newTemp: ", newTemp,
            ", tripId: ", tripId, ", tripHystTemp: ", tripHystTemp);
    }
    return status;
}

// Call this when Temp changes for particular Tzone
telux::common::Status ThermalGrpcServerImpl::getTripAndCdevUpdate(int tZoneId, int prevTzoneTemp,
        int newTzoneTemp, std::shared_ptr<telux::therm::ITripPoint> tp) {
    LOG(DEBUG, __FUNCTION__, ":: ZoneId: ", tZoneId, ", tripId: ", tp->getTripId());
    telux::common::Status rStatus = telux::common::Status::SUCCESS;

    rStatus = getTripUpdate(prevTzoneTemp, newTzoneTemp, tZoneId, tp);
    return rStatus;
}

telux::common::Status ThermalGrpcServerImpl::setThermalZone(int tZoneId, int newTemp) {
    LOG(DEBUG, __FUNCTION__, ":: setting tZone: ", tZoneId, " to temp: ", newTemp);
    telux::common::Status status;

    std::lock_guard<std::mutex> lk(setTempMutex_);
    auto itr = std::find_if(jsonHelper_->tZoneList_.begin(), jsonHelper_->tZoneList_.end(),
            [tZoneId](std::shared_ptr<telux::therm::ThermalZoneImpl> tz)
            { return (tz->getId() == tZoneId);
            });
    if (itr == jsonHelper_->tZoneList_.end()) {
        LOG(ERROR, __FUNCTION__, ":: thermal zone: ", tZoneId, " not found");
        return telux::common::Status::FAILED;
    }

    auto tz = (*itr);
    auto prevTemp = tz->getCurrentTemp();
    int lowestTemp = INT_MAX;
    tz->setCurrentTemp(newTemp);
    auto tps = tz->getTripPoints();
    for (auto tp : tps) {
        (tp->getThresholdTemp() <= lowestTemp) ? (lowestTemp = tp->getThresholdTemp())
            : (lowestTemp = lowestTemp);
        status = getTripAndCdevUpdate(tZoneId, prevTemp, newTemp, tp);
    }

    if (status == telux::common::Status::SUCCESS) {
        return status;
    }

    /** Trip points are STABLE however Cdev need to be updated
     **  E.g.  (1) CROSS_OVER event: prevTemp[32,600], newTemp[126,000]
     **            suppose newTemp is above of all 4 trips,
     **            so all trips + cDevs associated have been triggered.
     **        (2) CROSS_UNDER event: prevTemp[126,000], newTemp[32,000]
     **            suppose newTemp is below of all 4 trips,
     **            so all trips shall triggered however cDev level comes
     **            down gradually (one step down for e.g. from 255 to 11)
     **        (3) STABLE: prevTemp[32,000] and newTemp[31,000]
     **            no trip event since newTemp is already lower than trip temp
     **            however cDev level comes down gradually
     **            (one step down for e.g. from 11 to 0)
     */
    if ((newTemp < lowestTemp) && (newTemp < prevTemp)) {
        for (auto tp : tps) {
            LOG(DEBUG, __FUNCTION__, ":: prevTemp: ", prevTemp,
                    ", lowestTemp: ", lowestTemp, ", newTemp: ", newTemp);
            status = getNewCdevStateUpdate(TREND_DROPPING, tZoneId, tp->getTripId());
            if (status == telux::common::Status::SUCCESS) {
                break;
            }
        }
    }

    return status;
}

grpc::Status ThermalGrpcServerImpl::registerTripEvent(
        ServerWriter<::thermStub::RegisterOnTripEventReply>* response) {

    registerNotification<thermStub::RegisterOnTripEventReply>
        (response, onTripEventReplyWriters_, onTripEventMutex_,
         onTripEventCv_,
         ThermalGrpcServerImpl::OnTripEvntNotifyCnt_);

    return grpc::Status::OK;
}

grpc::Status ThermalGrpcServerImpl::deRegisterTripEvent(int clientId) {

    auto status = deRegisterNotification<thermStub::RegisterOnTripEventReply>(
            onTripEventReplyWriters_, onTripEventMutex_,
            onTripEventCv_, clientId);

    return status;
}

grpc::Status ThermalGrpcServerImpl::registerCdevStateChangeEvent(
        ServerWriter<::thermStub::RegisterOnCoolingDeviceLevelChangeReply>* response) {

    registerNotification<thermStub::RegisterOnCoolingDeviceLevelChangeReply>
        (response, onCoolingDeviceLevelChangeReplyWriters_,
         onCoolingDeviceLevelChangeMutex_, onCoolingDeviceLevelChangeCv_,
         ThermalGrpcServerImpl::OnCdevLevelChngNotifyCnt_);

    return grpc::Status::OK;
}

grpc::Status ThermalGrpcServerImpl::deRegisterCdevStateChangeEvent(int clientId) {

    auto status = deRegisterNotification<thermStub::RegisterOnCoolingDeviceLevelChangeReply>(
            onCoolingDeviceLevelChangeReplyWriters_, onCoolingDeviceLevelChangeMutex_,
            onCoolingDeviceLevelChangeCv_, clientId);

    return status;
}

std::shared_ptr<telux::therm::CoolingDeviceImpl> ThermalGrpcServerImpl::setCoolingDevice(
        int tZoneId, int tripId, int cDevId, int trend, int nextCdevState) {
    LOG(DEBUG, __FUNCTION__, ":: setting cooling device: ", cDevId,
            " to state: ", nextCdevState);

    std::lock_guard<std::mutex> lk(setCdevMutex_);
    auto itr = std::find_if(jsonHelper_->cDevList_.begin(),
            jsonHelper_->cDevList_.end(), [cDevId](
                std::shared_ptr<telux::therm::CoolingDeviceImpl> cd)
            { return (cd->getId() == cDevId);
            });
    if (itr == jsonHelper_->cDevList_.end()) {
        LOG(ERROR, __FUNCTION__, ":: cooling device: ", cDevId, " not found");
        return std::make_shared<telux::therm::CoolingDeviceImpl>();
    }

    auto cd = (*itr);
    if (trend == TREND_DROPPING) {
        // Note: Temp decrease gradually
        auto currLevel = cd->getCurrentCoolingLevel();
        auto cDevs = jsonHelper_->getCoolingDeviceLevel(tZoneId, tripId, TREND_RAISING, cDevId);
        for (auto cdev : cDevs) {
            auto currState = cdev.second;
            LOG(DEBUG, __FUNCTION__,
                ":: currLevel: ", currLevel, ", currState: ", currState);
            if ((currLevel != currState) || (currLevel == 0)) {
                return std::make_shared<telux::therm::CoolingDeviceImpl>();
            }
        }
    }

    cd->setCurrentCoolingLevel(static_cast<int>(nextCdevState));

    return cd;
}
