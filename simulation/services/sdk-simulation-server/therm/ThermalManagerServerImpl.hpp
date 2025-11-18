/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include "../event/ServerEventManager.hpp"
#include "../event/EventService.hpp"

#define DEFAULT_DELIMITER " "

class ThermalManagerServerImpl : public IServerEventListener,
                                 public std::enable_shared_from_this
                                        <ThermalManagerServerImpl> {
public:
    ThermalManagerServerImpl();
    ~ThermalManagerServerImpl();


protected:
    virtual telux::common::Status setThermalZone(int tZoneId, int temp) = 0;
    virtual void onSSREvent(telux::common::ServiceStatus srvStatus) = 0;

    telux::common::Status registerDefaultIndications();
    void notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr);
    void setServiceStatus(telux::common::ServiceStatus srvStatus);
    telux::common::ServiceStatus getServiceStatus();

    void onEventUpdate(::eventService::UnsolicitedEvent event);
    void onEventUpdate(std::string event);
    void handleEvent(std::string token,std::string event);
    void handleSSREvent(std::string eventParams);
    void setTempEvent(std::string token, std::string event);

    telux::common::ServiceStatus serviceStatus_ =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex mutex_;
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;
};
