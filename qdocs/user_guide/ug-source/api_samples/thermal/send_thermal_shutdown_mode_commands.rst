.. _send-thermal-shutdown-mode-commands:

Get/Set autoshutdown modes 
=================================================================

This sample app demonstrates how to get/set thermal autoshutdown mode.

1. Get thermal factory instance

.. code-block::

   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();


2. Prepare initialization callback

.. code-block::

   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
   };


3. Get thermal shutdown manager instance

.. code-block::

   thermShutdownMgr_ = thermalFactory.getThermalShutdownManager(initCb);
   if(thermShutdownMgr_ == NULL)
   {
      std::cout << APP_NAME << " *** ERROR - Failed to get manager instance" << std::endl;
      return -1;
   }


4. Wait for the initialization callback and check the service status

.. code-block::

   telux::common::ServiceStatus serviceStatus = p.get_future().get();
   if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << APP_NAME << " Thermal-Shutdown management services are ready !" << std::endl;
   } else {
      std::cout << APP_NAME << " *** ERROR - Unable to initialize Thermal-Shutdown management "
                                 "services" << std::endl;
      return -1;
   }


5. Query the current thermal auto shutdown mode

.. code-block::

   // Callback which provides response to query operation
   void getStatusCallback(AutoShutdownMode mode)
   {
       if(mode == AutoShutdownMode::ENABLE) {
          std::cout << " Current auto shutdown mode is: Enable" << std::endl;
       } else if(mode == AutoShutdownMode::DISABLE) {
          std::cout << " Current auto shutdown mode is: Disable" << std::endl;
       } else {
          std::cout << " *** ERROR - Failed to send get auto-shutdown mode " << std::endl;
       }
   }

   // Send get themal auto shutdown mode command
   auto status = thermShutdownMgr_->getAutoShutdownMode(getStatusCallback);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "getShutdownMode command failed with error" << static_cast<int>(status) << std::endl;
   } else {
      std::cout << "Request to query thermal shutdown status sent" << std::endl;
   }


6. Set thermal auto shutdown mode

.. code-block::

   // Callback which provides response to set thermal auto shutdown mode command
   void commandResponse(ErrorCode error)
   {
       if(error == ErrorCode::SUCCESS) {
       std::cout << " sent successfully" << std::endl;
       } else {
          std::cout << " failed\n errorCode: " << static_cast<int>(error) << std::endl;
       }
   }

   // Send set themal auto shutdown mode command
   auto status = thermShutdownMgr_->setAutoShutdownMode(state, commandResponse);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "setShutdownMode command failed with error" << static_cast<int>(status) << std::endl;
   } else {
      std::cout << "Request to set thermal shutdown status sent" << std::endl;
   }

