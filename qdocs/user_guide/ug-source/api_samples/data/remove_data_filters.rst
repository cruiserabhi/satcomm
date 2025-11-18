.. _remove-data-filter:

Remove data filter mode 
=============================================

This sample application demonstrates how to remove all data filter mode.

1. Get the DataFactory, DataConnectionManager and DataFilterManager instances

.. code-block::

   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataConnMgr_ = dataFactory.getDataConnectionManager();
   auto dataFilterMgr_ = dataFactory.getDataFilterManager();


2. Wait for the data connection and data filter manager sub system initialization

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


3. Remove all data filter

.. code-block::

   std::promise<bool> p;
   int profileId = 2;
   telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;

   auto status = dataFilterMgr_->removeAllDataRestrictFilters(
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to remove all filter" << std::endl;
                p.set_value(false);
            }
        }, profileId, ipFamilyType);
   if(status == telux::common::Status::SUCCESS) {
        std::cout << "Remove all data filter Request sent" << std::endl;
   } else {
        std::cout << "Remove all data filter Request failed" << std::endl;
   }

   if (p.get_future().get()) {
             std::cout << "Remove all data filter succeeded." << std::endl;
   }

