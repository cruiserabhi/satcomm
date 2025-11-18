/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "ThermalManagerServerImpl.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "common/CommonUtils.hpp"
#define THERM "therm"

ThermalManagerServerImpl::ThermalManagerServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
    LOG(DEBUG, __FUNCTION__);

}

ThermalManagerServerImpl::~ThermalManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status ThermalManagerServerImpl::registerDefaultIndications() {
    LOG(DEBUG, __FUNCTION__);

    auto status = serverEvent_.registerListener(shared_from_this(), THERM);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications with QMI failed");
        return status;
    }

    return status;
}

void ThermalManagerServerImpl::notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr) {
    LOG(DEBUG, __FUNCTION__, ":: Service status Changed to ", srvStatusStr);
    onSSREvent(srvStatus);
}

telux::common::ServiceStatus ThermalManagerServerImpl::getServiceStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void ThermalManagerServerImpl::setServiceStatus(telux::common::ServiceStatus srvStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serviceStatus_ != srvStatus) {
        serviceStatus_ = srvStatus;
        std::string srvStrStatus = CommonUtils::mapServiceString(srvStatus);
        notifyServiceStateChanged(serviceStatus_, srvStrStatus);
    }
}

void ThermalManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    if (event.filter() == THERM) {
        onEventUpdate(event.event());
    }
}

/* Get Notification for THERM_TRIP_FILTER or THERM_CDEV_FILTER or SSR*/
void ThermalManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The thermal event: ", event);
    std::string token;

    /** INPUT-event:
     * (1) ssr SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     * (2) setTemp <tZoneId> <temp>
     * OUTPUT-token:
     * (1) ssr
     * (2) setTemp
     **/
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    /** INPUT-token:
     * (1) ssr
     * (2) setTemp
     * INPUT-event:
     * (1) SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     * (2) <tZoneId> <temp>
     **/
    handleEvent(token, event);
}

/** INPUT-token:
  * (1) ssr
  * (2) setTemp
  * INPUT-event:
  * (1) SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
  * (2) <tZoneId> <temp>
 */
void ThermalManagerServerImpl::handleEvent(std::string token,std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The thermal event type is: ", token,
            "The leftover string is: ", event);

    if (token == "ssr") {
        //INPUT-token: ssr
        //INPUT-event: SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
        handleSSREvent(event);
    }
    else if (token == "setTemp") {
        //INPUT-token: setTemp
        //INPUT-event: <tZoneId> <temp>
        setTempEvent(token, event);
    } else {
        LOG(DEBUG, __FUNCTION__, ":: Invalid event ! Ignoring token: ",
                token, ", event: ", event);
    }
}

void ThermalManagerServerImpl::handleSSREvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: SSR event: ", eventParams);

    telux::common::ServiceStatus srvcStatus =
        telux::common::ServiceStatus::SERVICE_FAILED;
    if (eventParams == "SERVICE_AVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    } else if (eventParams == "SERVICE_UNAVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (eventParams == "SERVICE_FAILED") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(DEBUG, __FUNCTION__, ":: INVALID SSR event: ", eventParams);
        return;
    }

    setServiceStatus(srvcStatus);
}

/** INPUT-token: setTemp
 *  INPUT-event: <tZoneId> <temp>
 **/
void ThermalManagerServerImpl::setTempEvent(std::string token,
        std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: event: ", token, "param: ", event);

    int tZoneId = -1, temp = -1;

    try {
        tZoneId = std::stoi(EventParserUtil::getNextToken(event, DEFAULT_DELIMITER));
        temp    = std::stoi(EventParserUtil::getNextToken(event, DEFAULT_DELIMITER));
    } catch (exception const & ex) {
        LOG(ERROR, __FUNCTION__, ":: Exception Occured: ", ex.what());
        return;
    }

    auto status = setThermalZone(tZoneId, temp);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Unable to set temprature for tZone: ", tZoneId);
    }
}
