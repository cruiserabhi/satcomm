.. _sensor-feature-control:

Control sensor features 
=================================================

This sample application demonstrates how to control sensor features.

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


3. Get the sensor feature manager. If initialization fails, perform necessary error handling

.. code-block::

   std::shared_ptr<telux::sensor::ISensorFeatureManager> sensorFeatureManager
       = sensorFactory.getSensorFeatureManager(initCb);
   if (sensorFeatureManager == nullptr) {
      std::cout << "sensor feature manager is nullptr" << std::endl;
      exit(1);
   }
   std::cout << "obtained sensor feature manager" << std::endl;


4. Wait until initialization is complete

.. code-block::

   p.get_future().get();
   if (sensorManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Sensor service not available" << std::endl;
      exit(1);
   }


5. Get information regarding available sensor features

.. code-block::

   std::cout << "Sensor feature service is now available" << std::endl;
   std::vector<telux::sensor::SensorFeature> sensorFeatures;
   telux::common::Status status = sensorFeatureManager->getAvailableFeatures(sensorFeatures);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get information on available features" << static_cast<int>(status)
                << std::endl;
      exit(1);
   }
   std::cout << "Received sensor features" << std::endl;
   for (auto feature : sensorFeatures) {
      printSensorFeatureInfo(feature);
   }


6. Create and register a sensor feature event listener

.. code-block::

   std::shared_ptr<SensorFeatureEventListener> sensorFeatureEventListener
       = std::make_shared<SensorFeatureEventListener>();
   sensorFeatureManager->registerListener(sensorFeatureEventListener);


7. Enable the required features

.. code-block::

   status = sensorFeatureManager->enableFeature(name);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to enable feature: " << name << std::endl;
      exit(1);
   }

Note: Enabling a sensor feature when the system is active would additionally require enabling the corresponding sensor which is used by the sensor feature.
If the sensor feature only needs to be enabled during suspend mode, just enabling the sensor feature using this method would be sufficient. The underlying framework would take care to enable the required sensor when the system is about to enter suspend state.

8. Receive sensor feature events with the registered listener when device is not suspended

.. code-block::

   virtual void onEvent(telux::sensor::SensorFeatureEvent event) override {
      printSensorFeatureEvent(event);
   }


9. Receive sensor feature events with the registered listener when device in suspended state

.. code-block::

   virtual void onBufferedEvent(std::string sensorName,
                    std::shared_ptr<std::vector<SensorEvent>> events, bool isLast) override {
      printSensorFeatureEvent(event);
   }


10. When the sensor feature(s) are no longer necessary, disable them

.. code-block::

   status = sensorFeatureManager->disableFeature(name);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to disable feature: " << name << std::endl;
      exit(1);
   }


11. Release the instance of ISensorFeatureManager to cleanup resources

.. code-block::

   sensorFeatureManager = nullptr;

