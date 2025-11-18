.. _location-services-configurator:

Using location configurator APIs 
==================================================================

This sample app gives basic idea about using location configurator APIs.

1. Implement a command response method

.. code-block::

   void CmdResponse(ErrorCode error) {
       if (error == ErrorCode::SUCCESS) {
           std::cout << " Command executed successfully" << std::endl;
       }
       else {
           std::cout << " Command failed\n errorCode: " << static_cast<int>(error) << std::endl;
       }
   }


2. Get the LocationFactory instance

.. code-block::

   auto &locationFactory = LocationFactory::getInstance();


3. Get LocationConfigurator instance

.. code-block::

   std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
   auto locConfigurator_ = locationFactory.getLocationConfigurator([&](ServiceStatus status) {
        prom.set_value(status);
   });


4. Wait for the Location Config. initialization

.. code-block::

   ServiceStatus managerStatus = locConfigurator_->getServiceStatus();
    if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Location Config. is not ready, Please wait!!!... "<< std::endl;
        managerStatus = prom.get_future().get();
    }


5. Exit the application, if SDK is unable to initialize location config

.. code-block::

   if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
       std::cout<< "Location Config. is ready" << std::endl;
   } else {
       std::cout << " *** ERROR - Unable to initialize Location Config."<< std::endl;
       return -1;
   }


6. Enable/Disable Constraint Tunc API

.. code-block::

   // CmdResponse callback is invoked with error code indicating
   // SUCCESS/FAILURE of the operation
   locConfigurator_->configureCTunc(enable, CmdResponse, optThreshold, optPower);

