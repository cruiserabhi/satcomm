.. _sensor-data-acquisition:

Configure and acquire sensor data 
=============================================================

This sample application demonstrates how to configure and acquire sensor data. The sample
application also shows how to acquire data from the same sensor with different configurations

1. Get sensor factory instance

.. code-block::

   auto &sensorFactory = telux::sensor::SensorFactory::getInstance();


2. Prepare a callback that is invoked when the sensor sub-system initialization is complete

.. code-block::

   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
      std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
      p.set_value(status);
   };


3. Get the sensor manager. If initialization fails, perform necessary error handling

.. code-block::

   std::shared_ptr<telux::sensor::ISensorManager> sensorManager
      = sensorFactory.getSensorManager(initCb);
   if (sensorManager == nullptr) {
      std::cout << "sensor manager is nullptr" << std::endl;
      exit(1);
   }
   std::cout << "obtained sensor manager" << std::endl;


4. Wait until initialization is complete

.. code-block::

   p.get_future().get();
   if (sensorManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Sensor service not available" << std::endl;
      exit(1);
   }


5. Get information regarding available sensors in the system

.. code-block::

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


6. Request the ISensorManager for the desired sensor

.. code-block::

   std::shared_ptr<telux::sensor::ISensorClient> lowRateSensorClient;
   std::cout << "Getting sensor: " << name << std::endl;
   status = sensorManager->getSensorClient(lowRateSensorClient, name);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get sensor: " << name << std::endl;
      exit(1);
   }


7. Create and register a sensor event listener for configuration updates and sensor events

  The event listener extends the ISensorEventListener to receive notification about configuration and sensor events.

  .. code-block::

     class SensorEventListener : public telux::sensor::ISensorEventListener {
      public:
        SensorEventListener(std::string name, std::shared_ptr<telux::sensor::ISensorClient> sensor)
           : name_(name)
           , sensorClient_(sensor)
           , totalBatches_(0) {
        }

        // [11] Receive sensor events. This notification is received every time the configured batch
        // count is available with the sensor framework
        virtual void onEvent(std::shared_ptr<std::vector<telux::sensor::SensorEvent>> events) override {

           PRINT_NOTIFICATION << "(" << name_ << "): Received " << events->size()
                                << " events from sensor: " << sensorClient_->getSensorInfo().name
                                << std::endl;

           // I/O intense operations such as below should be avoided since this thread should avoid
           // any time consuming operations
           for (telux::sensor::SensorEvent s : *(events.get())) {
                 printSensorEvent(s);
           }
           ++totalBatches_;
           // [11.1] If we have received expected number of batches and want to reconfigure the sensor
           // we will spawn the request to deactivate, configure and activate on a different thread
           // since we are not allowed to invoke the sensor APIs from this thread context
           if (totalBatches_ > TOTAL_BATCHES_REQUIRED) {
                 totalBatches_ = 0;
                 std::thread t([&] {
                    sensorClient_->deactivate();
                    sensorClient_->configure(sensorClient_->getConfiguration());
                    sensorClient_->activate();
                 });
                 // Be sure to detach the thread
                 t.detach();
           }
        }

        // [9] Receive configuration updates
        virtual void onConfigurationUpdate(telux::sensor::SensorConfiguration configuration) override {
           PRINT_NOTIFICATION << "(" << name_ << "): Received configuration update from sensor: "
                                << sensorClient_->getSensorInfo().name << ": ["
                                << configuration.samplingRate << ", " << configuration.batchCount << " ]"
                                << std::endl;
        }

     private:
        bool isUncalibratedSensor(telux::sensor::SensorType type) {
           return ((type == telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED)
                    || (type == telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED));
        }

        void printSensorEvent(telux::sensor::SensorEvent &s) {
           telux::sensor::SensorInfo info = sensorClient_->getSensorInfo();
           if (isUncalibratedSensor(sensorClient_->getSensorInfo().type)) {
                 PRINT_NOTIFICATION << ": " << sensorClient_->getSensorInfo().name << ": " << s.timestamp
                                   << ", " << s.uncalibrated.data.x << ", " << s.uncalibrated.data.y
                                   << ", " << s.uncalibrated.data.z << ", " << s.uncalibrated.bias.x
                                   << ", " << s.uncalibrated.bias.y << ", " <<   s.uncalibrated.bias.z
                                   << std::endl;
           } else {
                 PRINT_NOTIFICATION << ": " << sensorClient_->getSensorInfo().name << ": " << s.timestamp
                                   << ", " << s.calibrated.x << ", " << s.calibrated.y << ", "
                                   << s.calibrated.z << std::endl;
           }
        }

        std::string name_;
        std::shared_ptr<telux::sensor::ISensorClient> sensorClient_;
        uint32_t totalBatches_;
     };


  Create a event listener and register it with the sensor.

  .. code-block::
   
     std::shared_ptr<SensorEventListener> lowRateSensorClientEventListener
        = std::make_shared<SensorEventListener>("Low-rate", lowRateSensorClient);
     lowRateSensorClient->registerListener(lowRateSensorClientEventListener);


8. Configure the sensor with required configuration setting the necessary validityMask

.. code-block::

   telux::sensor::SensorConfiguration lowRateConfig;
   lowRateConfig.samplingRate = getMinimumSamplingRate(lowRateSensorClient->getSensorInfo());
   lowRateConfig.batchCount = lowRateSensorClient->getSensorInfo().maxBatchCountSupported;
   std::cout << "Configuring sensor with samplingRate, batchCount [" << lowRateConfig.samplingRate
            << ", " << lowRateConfig.batchCount << "]" << std::endl;
   lowRateConfig.validityMask.set(telux::sensor::SensorConfigParams::SAMPLING_RATE);
   lowRateConfig.validityMask.set(telux::sensor::SensorConfigParams::BATCH_COUNT);
   status = lowRateSensorClient->configure(lowRateConfig);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to configure sensor: " << name << std::endl;
      exit(1);
   }


9. Receive updates on sensor configuration

.. code-block::

   virtual void onConfigurationUpdate(telux::sensor::SensorConfiguration configuration) override {
      PRINT_NOTIFICATION << "(" << name_ << "): Received configuration update from sensor: "
                        << sensorClient_->getSensorInfo().name << ": ["
                        << configuration.samplingRate << ", " << configuration.batchCount << " ]"
                        << std::endl;
   }


10. Activate the sensor to receive sensor data

.. code-block::

   status = lowRateSensorClient->activate();
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to activate sensor: " << name << std::endl;
      exit(1);
   }


11. Receive sensor data with the registered listener

Avoid any time consuming operation in this callback. This thread should be released back the SDK
library to avoid latency.

If any sensor APIs need to be called in this method, they should be done on a different thread. One
such method is to spawn a detached thread that invokes the required API.

.. code-block::

   virtual void onEvent(std::shared_ptr<std::vector<telux::sensor::SensorEvent>> events) override {

      PRINT_NOTIFICATION << "(" << name_ << "): Received " << events->size()
                        << " events from sensor: " << sensorClient_->getSensorInfo().name
                        << std::endl;

      // I/O intense operations such as below should be avoided since this thread should avoid
      // any time consuming operations
      for (telux::sensor::SensorEvent s : *(events.get())) {
         printSensorEvent(s);
      }
      ++totalBatches_;
      // [11.1] If we have received expected number of batches and want to reconfigure the sensor
      // we will spawn the request to deactivate, configure and activate on a different thread
      // since we are not allowed to invoke the sensor APIs from this thread context
      if (totalBatches_ > TOTAL_BATCHES_REQUIRED) {
         totalBatches_ = 0;
         std::thread t([&] {
               sensorClient_->deactivate();
               sensorClient_->configure(sensorClient_->getConfiguration());
               sensorClient_->activate();
         });
         // Be sure to detach the thread
         t.detach();
      }
   }


12. Create another sensor client for the same sensor and it's corresponding listener

.. code-block::

   std::shared_ptr<telux::sensor::ISensorClient> highRateSensorClient;
   std::cout << "Getting sensor: " << name << std::endl;
   status = sensorManager->getSensorClient(highRateSensorClient, name);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get sensor: " << name << std::endl;
      exit(1);
   }
   std::shared_ptr<SensorEventListener> highRateSensorEventListener
      = std::make_shared<SensorEventListener>("High-rate", highRateSensorClient);
   highRateSensorClient->registerListener(highRateSensorEventListener);


13. Configure this sensor client with a different configuration, as necessary

.. code-block::

   telux::sensor::SensorConfiguration highRateConfig;
   highRateConfig.samplingRate = getMaximumSamplingRate(highRateSensorClient->getSensorInfo());
   highRateConfig.batchCount = highRateSensorClient->getSensorInfo().maxBatchCountSupported;
   std::cout << "Configuring sensor with samplingRate, batchCount [" << highRateConfig.samplingRate
            << ", " << highRateConfig.batchCount << "]" << std::endl;
   highRateConfig.validityMask.set(telux::sensor::SensorConfigParams::SAMPLING_RATE);
   highRateConfig.validityMask.set(telux::sensor::SensorConfigParams::BATCH_COUNT);
   status = highRateSensorClient->configure(highRateConfig);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to configure sensor: " << name << std::endl;
      exit(1);
   }


14. Activate this sensor as well

.. code-block::

   status = highRateSensorClient->activate();
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to activate sensor: " << name << std::endl;
      exit(1);
   }


15. When data acquisition is no longer necessary, deactivate the sensors

.. code-block::

   status = lowRateSensorClient->deactivate();
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to deactivate sensor: " << name << std::endl;
      exit(1);
   }
   status = highRateSensorClient->deactivate();
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to deactivate sensor: " << name << std::endl;
      exit(1);
   }


16. Release the instances of ISensorClient if no longer required

.. code-block::

   lowRateSensorClient = nullptr;
   highRateSensorClient = nullptr;


17. Release the instance of ISensorManager to cleanup resources

.. code-block::

   sensorManager = nullptr;

