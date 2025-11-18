.. _get-roaming-status-and-indication:

Get roaming status and indication 
======================================================================

This sample application demonstrates how to get roaming status and indications.

1. Implement IServingSystemListener listener class

.. code-block::

   class ServingSystemListener : public telux::data::IServingSystemListener {
    public:
      ServingSystemListener(SlotId slotId) : slotId_(slotId) {}
      void onRoamingStatusChanged(telux::data::RoamingStatus status) {
        std::cout << "\n onRoamingStatusChanged on SlotId: "
                    << static_cast<int>(slotId_) << std::endl;
        bool isRoaming = status.isRoaming;
        if(isRoaming) {
            std::cout << "System is in Roaming State" << std::endl;
        } else {
            std::cout << "System is not in Roaming State" << std::endl;
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


6. Get current service status

.. code-block::

   // Callback
   auto respCb = [&slotId](
         telux::data::RoamingStatus roamingStatus, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "requestRoamingStatus Response on slotid " << static_cast<int>(slotId);
         if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            logRoamingStatusDetails(roamingStatus);
         }
         else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         }
   };
   
   dataServingSystemMgr->requestRoamingStatus(respCb);


Now, wait for request response and notifications.
