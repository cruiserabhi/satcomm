Location, SV and Jammer reports {#location_services}
================================================

This sample app demonstrates how to get location, satellite vehicle and jammer info reports.

### 1. Implement a command response method

   ~~~~~~{.cpp}
   void CmdResponse(ErrorCode error) {
       if (error == ErrorCode::SUCCESS) {
           std::cout << " Command executed successfully" << std::endl;
       }
       else {
           std::cout << " Command failed\n errorCode: " << static_cast<int>(error) << std::endl;
       }
   }
   ~~~~~~

### 2. Implement ILocationListener interface

   ~~~~~~{.cpp}
   class MyLocationListener : public ILocationListener {
       public:
           void onBasicLocationUpdate(const std::shared_ptr<ILocationInfoBase> &locationInfo)
             override;
           void onDetailedLocationUpdate(const std::shared_ptr<ILocationInfoEx> &locationInfo)
              override;
           void onGnssSVInfo(const std::shared_ptr<IGnssSVInfo> &gnssSVInfo) override;
           void onGnssSignalInfo(const std::shared_ptr<IGnssSignalInfo> &gnssDatainfo) override;
   };
   ~~~~~~

### 3. Get the LocationFactory instance

   ~~~~~~{.cpp}
   auto &locationFactory = LocationFactory::getInstance();
   ~~~~~~

### 4. Get LocationManager instance

   ~~~~~~{.cpp}
   std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
   auto locationManager_ = locationFactory.getLocationManager([&](ServiceStatus status) {
        prom.set_value(status);
   });
   ~~~~~~

### 5. Wait for the location subsystem initialization

   ~~~~~~{.cpp}
   ServiceStatus managerStatus = locationManager_->getServiceStatus();
   if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Location subsystem is not ready, Please wait!!!... "<< std::endl;
       managerStatus = prom.get_future().get();
   }
   ~~~~~~

### 6. Exit the application, if SDK is unable to initialize location subsystems

   ~~~~~~{.cpp}
   if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout<< "Subsystem is ready" << std::endl;
   } else {
        std::cout << " *** ERROR - Unable to initialize Location subsystem"<< std::endl;
        return -1;
   }
   ~~~~~~

### 7. Instantiate MyLocationListener

   ~~~~~~{.cpp}
   auto myLocationListener = std::make_shared<MyLocationListener>();
   ~~~~~~

### 8. Register for location, SV and jammer info updates

   ~~~~~~{.cpp}
   locationManager_->registerListenerEx(myLocationListener);
   ~~~~~~

### 9. Start location reports with detailed information

   ~~~~~~{.cpp}
   uint32_t minIntervalInput = 2000; // Default is 1000 milli seconds.
   
   // CmdResponse callback is invoked with error code indicating
   // SUCCESS/FAILURE of the operation
   locationManager_->startDetailedReports(minIntervalInput, CmdResponse);
   ~~~~~~

### 10. Wait for location fix, SV and Jammer info

   ~~~~~~{.cpp}
   void MyLocationListener::onDetailedLocationUpdate(
      const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
        std::cout << "New detailed Location info received" << std::endl;
   }

   void MyLocationListener::onGnssSVInfo(
      const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
        std::cout << "Gnss Satellite Vehicle info received" << std::endl;
   }

   void MyLocationListener::onGnssSignalInfo(
      const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) {
        std::cout << "Gnss Signal info received" << std::endl;
   }
   ~~~~~~

Now observe that changes in the configuration parameters have corresponding effect on how the location, satellite vehicle (SV) and jammer reports are received. The configuration will be same across all the location client applications and the least value of the parameter from all client applications will take effect finally.
