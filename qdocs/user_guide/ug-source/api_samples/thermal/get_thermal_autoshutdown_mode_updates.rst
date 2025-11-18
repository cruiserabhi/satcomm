.. _get-thermal-autoshutdown-mode-updates:

Get thermal autoshutdown mode updates 
==============================================================================

This sample app demonstrates how to get thermal autoshutdown mode updates.

1. Implement IThermalShutdownListener interface

.. code-block::

   class MyThermalShutdownModeListener : public IThermalShutdownListener {
   public:
       void onShutdownEnabled() override;
       void onShutdownDisabled() override;
       void onImminentShutdownEnablement(uint32_t imminentDuration) override;
       void onServiceStatusChange(ServiceStatus status) override;
   };


2. Get thermal factory instance

.. code-block::

   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();


3. Prepare initialization callback

.. code-block::

   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
   };


4. Get thermal shutdown manager instance

.. code-block::

   thermShutdownMgr_ = thermalFactory.getThermalShutdownManager(initCb);
   if(thermShutdownMgr_ == NULL)
   {
       std::cout << APP_NAME << " *** ERROR - Failed to get manager instance" << std::endl;
       return -1;
   }


5. Wait for the initialization callback and check the service status

.. code-block::

   telux::common::ServiceStatus serviceStatus = p.get_future().get();
   if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << APP_NAME << " Thermal-Shutdown management services are ready !" << std::endl;
   } else {
      std::cout << APP_NAME << " *** ERROR - Unable to initialize Thermal-Shutdown management "
                               "services" << std::endl;
      return -1;
   }


6. Instantiate MyThermalStateListener

.. code-block::

   auto myThermalModeListener = std::make_shared<MyThermalShutdownModeListener>();


7. Register for updates on thermal autoshutdown mode and its management service status

.. code-block::

   thermShutdownMgr_->registerListener(myThermalModeListener);


8. Wait for the Thermal auto shutdown mode updates

.. code-block::

   // Avoid long blocking calls when handling notifications
   void MyThermalShutdownModeListener::onShutdownEnabled() {
        std::cout << std::endl << "**** Thermal auto shutdown mode enabled ****" << std::endl;
   }
   void MyThermalShutdownModeListener::onShutdownDisabled() {
        std::cout << std::endl << "**** Thermal auto shutdown mode disabled ****" << std::endl;
   }
   void MyThermalShutdownModeListener::onImminentShutdownEnablement(uint32_t imminentDuration) {
        std::cout << std::endl << "**** Thermal auto shutdown mode will be enabled in "
            << imminentDuration << " seconds ****" << std::endl;
   }


9. Implement onServiceStatusChange callback to know when thermal shutdown management service goes down

.. code-block::

   // When the thermal management service goes down, this API is invoked
   // with status UNAVAILABLE. All thermal auto shutdown mode notifications
   // stopped until the status becomes AVAILABLE again.
   void MyThermalShutdownModeListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "**** Thermal-Shutdown management service status update ****" << std::endl;
        // Avoid long blocking calls when handling notifications
   }

