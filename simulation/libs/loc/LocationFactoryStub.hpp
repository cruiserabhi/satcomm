/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef LOCATIONFACTORYSTUB_HPP
#define LOCATIONFACTORYSTUB_HPP

#include <telux/loc/LocationFactory.hpp>
#include "LocationManagerStub.hpp"
#include "LocationConfiguratorStub.hpp"
#include "DgnssManagerStub.hpp"


namespace telux {

namespace loc {

class LocationFactoryStub : public LocationFactory {

public:
   static LocationFactory &getInstance();

   std::shared_ptr<ILocationManager> getLocationManager(telux::common::InitResponseCb
        callback = nullptr);

   std::shared_ptr<ILocationConfigurator> getLocationConfigurator(telux::common::InitResponseCb
        callback = nullptr);

   std::shared_ptr<IDgnssManager> getDgnssManager(
           DgnssDataFormat dataFormat = DgnssDataFormat::DATA_FORMAT_RTCM_3,
                    telux::common::InitResponseCb callback = nullptr);

private:
   LocationFactoryStub();
   ~LocationFactoryStub();
   void onGetConfiguratorResponse(telux::common::ServiceStatus status);
   void onGetDgnssManagerResponse(telux::common::ServiceStatus status);

   std::shared_ptr<LocationManagerStub> locationManager_;
   std::shared_ptr<LocationConfiguratorStub> locConfigurator_;
   std::shared_ptr<DgnssManagerStub> dgnssManager_;
   std::mutex locationFactoryMutex_;
   std::vector<telux::common::InitResponseCb> configuratorCallbacks_;
   std::vector<telux::common::InitResponseCb> dgnssCallbacks_;
   telux::common::ServiceStatus configuratorInitStatus_;
   telux::common::ServiceStatus dgnssInitStatus_;
   std::condition_variable cv_;
};

}  // end of namespace loc

}  // end of namespace telux

#endif  // LOCATIONFACTORYSTUB_HPP