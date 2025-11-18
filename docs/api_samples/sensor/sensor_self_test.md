Using Sensor APIs to configure and acquire sensor data {#sensor_data_acquisition}
=============================================================

# Using Sensor APIs to initiate a self test and acquires the self test result

Please follow below steps as a guide to initiate a self test and acquires the self test result

### 1. Get sensor factory ###

   ~~~~~~{.cpp}
   auto &sensorFactory = telux::sensor::SensorFactory::getInstance();
   ~~~~~~

### 2. Prepare a callback that is invoked when the sensor sub-system initialization is complete ###

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
      std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
      p.set_value(status);
   };
   ~~~~~~

### 3. Get the sensor manager. If initialization fails, perform necessary error handling ###

   ~~~~~~{.cpp}
   std::shared_ptr<telux::sensor::ISensorManager> sensorManager
      = sensorFactory.getSensorManager(initCb);
   if (sensorManager == nullptr) {
      std::cout << "sensor manager is nullptr" << std::endl;
      exit(1);
   }
   std::cout << "obtained sensor manager" << std::endl;
   ~~~~~~

### 4. Wait until initialization is complete ###

   ~~~~~~{.cpp}
   p.get_future().get();
   if (sensorManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Sensor service not available" << std::endl;
      exit(1);
   }
   ~~~~~~

### 5. Get information regarding available sensors in the system ###

   ~~~~~~{.cpp}
   std::cout << "Sensor service is now available" << std::endl;
   std::vector<telux::sensor::SensorInfo> sensorInfo;
   telux::common::Status status = sensorManager->getAvailableSensorInfo(sensorInfo);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get information on available sensors" << static_cast<int>(status)
                << std::endl;
      exit(1);
   }
   std::cout << "Received sensor information" << std::endl;
   for (auto info : sensorInfo) {
      printSensorInfo(info);
   }
   ~~~~~~

### 6. Request the ISensorManager for the desired sensor ###

   ~~~~~~{.cpp}
   std::shared_ptr<telux::sensor::ISensorClient> sensor;
   std::cout << "Getting sensor: " << name << std::endl;
   status = sensorManager->getSensorClient(sensor, name);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get sensor: " << name << std::endl;
      exit(1);
   }
   ~~~~~~

### 7. Invoke the self test with the required self test type and provide the callback ###

   ~~~~~~{.cpp}
   status = sensor->selfTest(selfTestType, [](telux::common::ErrorCode result) {
      PRINT_CB << "Received self test response: " << static_cast<int>(result) << std::endl;
   });
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Self test request failed";
   } else {
      std::cout << "Self test request successful, waiting for callback" << std::endl;
   }
   ~~~~~~

### 8. Release the instance of ISensorClient if no longer required ###

   ~~~~~~{.cpp}
   sensor = nullptr;
   ~~~~~~

### 9. Release the instance of ISensorManager to cleanup resources ###

   ~~~~~~{.cpp}
   sensorManager = nullptr;
   ~~~~~~
