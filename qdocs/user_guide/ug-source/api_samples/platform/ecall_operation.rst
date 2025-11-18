.. _ecall-operation:

Ecall operation 
============================================

This sample app demonstrates how to prepare for an eCall operation and indicate eCall completion.

1. Get platform factory instance

.. code-block::

   auto &platformFactory = PlatformFactory::getInstance();


2. Prepare a callback that is invoked when the filesystem sub-system initialization is complete

.. code-block::

   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
      std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
      p.set_value(status);
   };


3. Get the filesystem manager

.. code-block::

   std::shared_ptr<telux::platform::IFsManager> fsManager = platformFactory.getFsManager(initCb);
   if (fsManager == nullptr) {
      std::cout << "filesystem manager is nullptr" << std::endl;
      exit(1);
   }
   std::cout << "Obtained filesystem manager" << std::endl;


4. Wait until initialization is complete

.. code-block::

   p.get_future().get();
   if (fsManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Filesystem service not available" << std::endl;
      exit(1);
   }
   std::cout << "Filesystem service is now available" << std::endl;


5. Create the listener object and register as a listener

.. code-block::

   std::shared_ptr<EcallOperationListener> ecallOperationListener
       = std::make_shared<EcallOperationListener>();
   fsManager->registerListener(ecallOperationListener);


6. Receive service status notifications

.. code-block::

   virtual void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override {
      PRINT_NOTIFICATION << "Ecall operation service status: ";
      std::string status;
      switch (serviceStatus) {
         case ServiceStatus::SERVICE_AVAILABLE: {
            status = "Available";
            break;
         }
         case ServiceStatus::SERVICE_UNAVAILABLE: {
            status = "Unavailable";
            break;
         }
         case ServiceStatus::SERVICE_FAILED: {
            status = "Failed";
            break;
         }
         default: {
            status = "Unknown";
            break;
         }
      }
      std::cout << status << std::endl;
   }


7. Before starting an eCall, prepare the filesystem manager for an eCall
  
  Note: It is recommended to start an eCall even if the request to prepare for an eCall fails.this API can re-invoke while an eCall is ongoing.

  .. code-block::

     telux::common::Status ecallStartStatus = telux::common::Status::FAILED;
     std::cout << "Request to prepare for eCall start invoked " << std::endl;

     ecallStartStatus = fsManager->prepareForEcall();

     if (ecallStartStatus == telux::common::Status::SUCCESS) {
        std::cout << "Request to prepare for eCall start successful";
     } else {
        std::cout << "Request to prepare for eCall start failed : ";
        Utils::printStatus(ecallStartStatus);
     }


8. Initiate an eCall

9. Receive notification when filesystem operation is about to resumes in seconds - timeLetftToStart

  Note: *On this notification, the client can still suspand the filesystem operation and continue the eCall by invoking prepareForEcall API (Step[7])*

  .. code-block::

     virtual void OnFsOperationImminentEvent(uint32_t timeLeftToStart) override {
          PRINT_NOTIFICATION << "Filesystem operation resumes in seconds: ";
          std::cout << timeLeftToStart << std::endl;
     }


10. Once an eCall is completed, indicate to the filesystem manager

.. code-block::

   telux::common::Status ecallEndStatus = telux::common::Status::FAILED;
   std::cout << "eCall completion request being invoked " << std::endl;

   ecallEndStatus = fsManager->eCallCompleted();

   if (ecallEndStatus == telux::common::Status::SUCCESS) {
      std::cout << "eCall completion request successful";
   } else {
      std::cout << "eCall completion request failed : ";
      Utils::printStatus(ecallEndStatus);
   }


11. Clean-up when we do not need to listen anything

.. code-block::

   fsManager->deregisterListener(ecallOperationListener);
   ecallOperationListener = nullptr;
   fsManager = nullptr;

