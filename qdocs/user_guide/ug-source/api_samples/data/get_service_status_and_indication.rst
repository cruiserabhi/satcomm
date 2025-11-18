.. _get-service-status-and-indication:

Get service status and indication 
======================================================================

This sample application demonstrates how to get service status and indications.

1. Implement IServingSystemListener listener class

.. code-block::

   class ServingSystemListener : public telux::data::IServingSystemListener {
    public:
      ServingSystemListener(SlotId slotId) : slotId_(slotId) {}
      void onServiceStateChanged(telux::data::ServiceStatus status) {
        std::cout << "\n onServiceStateChanged on SlotId: " << static_cast<int>(slotId_);
        if(status.serviceState == telux::data::DataServiceState::OUT_OF_SERVICE) {
            std::cout << "Current Status is Out Of Service" << std::endl;
        } else {
            std::cout << "Current Status is In Service" << std::endl;
        }
      }
    private:
      SlotId slotId_;
   };


2. Optionally, instantiate an initialization callback

.. code-block::

   auto initCb = [&](telux::common::ServiceStatus status) {
        subSystemStatus = status;
        subSystemStatusUpdated = true;
        cv_.notify_all();
   };


3. Get the data factory and data serving system manager instance and if data serving system manager is ready

.. code-block::

   auto &dataFactory = telux::data::DataFactory::getInstance();
   do {
       subSystemStatusUpdated = false;
       std::unique_lock<std::mutex> lck(mtx_);
       dataServingSystemMgr = dataFactory.getServingSystemManager(slotId, initCb);

       if (dataServingSystemMgr) {
            std::cout << "\n\nInitializing Data Serving System manager subsystem on slot " <<
                slotId << ", Please wait ..." << std::endl;
            cv_.wait(lck, [&]{return subSystemStatusUpdated;});
            subSystemStatus = dataServingSystemMgr->getServiceStatus();
       }
      
       if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** DATA Serving System is Ready *** " << std::endl;
            break;
       }
       else {
            std::cout << " *** Unable to initialize data Serving System *** " << std::endl;
       }
   } while (1);


4. Register for serving system listener

.. code-block::

   dataServingSystemMgr->registerListener(dataListener);


5. Get current service status

.. code-block::

   // Callback
   auto respCb = [&slotId](
        telux::data::ServiceStatus serviceStatus, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestServiceStatus Response on slotid " << static_cast<int>(slotId);
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            logServiceStatusDetails(serviceStatus);
        }
        else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error) << std::endl;
        }
   };
   
   dataServingSystemMgr->requestServiceStatus(respCb);


Now, wait for request response and notifications.
