.. _get-and-set-data-filter-mode:

Get/Set data filter mode
========================================================

This sample application demonstrates how to get and set data filter mode.

1. Get the DataFactory, DataConnectionManager and DataFilterManager instances

.. code-block::

   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataConnMgr_ = dataFactory.getDataConnectionManager();
   auto dataFilterMgr_ = dataFactory.getDataFilterManager();


2. Wait for the Data Connection Manager and Data Filter Manager sub system initialization

.. code-block::

   bool dataConnectionSubSystemStatus = dataConnMgr_->isSubsystemReady();
   if (!dataConnectionSubSystemStatus) {
       std::cout << "Data Connection Manager subsystem is not ready, Please wait" << std::endl;
       std::future<bool> f = dataConnMgr_->onSubsystemReady();
       // Wait unconditionally for data manager subsystem to be ready
       dataConnectionSubSystemStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize data manager subsystems
   if (!dataConnectionSubSystemStatus) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return EXIT_FAILURE;
   }

   bool dataFilterSubSystemStatus = dataFilterMgr_->isReady();
   if (!dataFilterSubSystemStatus) {
       std::cout << "Data Filter Manager subsystem is not ready, Please wait" << std::endl;
       std::future<bool> f = dataFilterMgr_->onReady();
       // Wait unconditionally for data filter subsystem to be ready
       dataFilterSubSystemStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize data manager subsystems
   if (!dataFilterSubSystemStatus) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return EXIT_FAILURE;
   }


3. Set data filter mode to enable

.. code-block::

   std::promise<bool> p;
   int profileId = 2;
   telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;

   telux::data::DataRestrictMode enableMode;
   enableMode.filterAutoExit = telux::data::DataRestrictModeType::DISABLE;
   enableMode.filterMode = telux::data::DataRestrictModeType::ENABLE;

   auto status = dataFilterMgr_->setDataRestrictMode(enableMode,
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to set data filter mode" << std::endl;
                p.set_value(false);
            }
        }, profileId, ipFamilyType);
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Set data filter mode Request sent" << std::endl;
   } else {
      std::cout << "Set data filter mode Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "Set data filter mode succeeded." << std::endl;
   }


4. Get data filter mode

.. code-block::

   std::promise<bool> p;
   std::string interfaceName = "rmnet_data0";

   auto status = dataFilterMgr_->requestDataRestrictMode(interfaceName, configType_,
             [&p](telux::data::DataRestrictMode mode, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
                if (mode.filterMode == DataRestrictModeType::DISABLE) {
                    std::cout << " DataRestrictMode Disabled" << std::endl;
                } else if (mode.filterMode == DataRestrictModeType::ENABLE) {
                    std::cout << " DataRestrictMode Enabled" << std::endl;
                } else {
                    std::cout << " Invalid DataRestrictMode" << std::endl;
                }
            } else {
                std::cout << "Failed to get data filter mode" << std::endl;
                p.set_value(false);
            }
   });
   
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Get data filter mode Request sent" << std::endl;
   } else {
      std::cout << "Get data filter mode Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "Get data filter mode succeeded." << std::endl;
   }

