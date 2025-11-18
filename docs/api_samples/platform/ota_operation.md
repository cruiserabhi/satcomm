OTA operation {#ota_operation}
============================================

This sample app demonstrates how to prepare for an ota operation, when to initiate an ota update and handle post-ota operations.

### 1. Get platform factory instance

   ~~~~~~{.cpp}
   auto &platformFactory = telux::platform::PlatformFactory::getInstance();
   ~~~~~~

### 2. Prepare a callback that is invoked when the filesystem sub-system initialization is complete

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
      std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
      p.set_value(status);
    };
   ~~~~~~

### 3. Get the filesystem manager

   ~~~~~~{.cpp}
   std::shared_ptr<telux::platform::IFsManager> fsManager = platformFactory.getFsManager(initCb);
   if (fsManager == nullptr) {
      std::cout << "filesystem manager is nullptr" << std::endl;
      exit(1);
   }
   std::cout << "Obtained filesystem manager" << std::endl;
   ~~~~~~

### 4. Wait until initialization is complete

   ~~~~~~{.cpp}
   p.get_future().get();
   if (fsManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Filesystem service not available" << std::endl;
      exit(1);
   }
   std::cout << "Filesystem service is now available" << std::endl;
   ~~~~~~

### 5. Create the listener object and register as a listener

   ~~~~~~{.cpp}
   std::shared_ptr<OtaOperationsListener> otaOperationsListener
       = std::make_shared<OtaOperationsListener>();
   fsManager->registerListener(otaOperationsListener);
   ~~~~~~

### 6. Receive service status notifications

   ~~~~~~{.cpp}
   virtual void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override {
      PRINT_NOTIFICATION << "Ota operation service status: ";
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
   ~~~~~~

### 7. Download the package

### 8. Prepare for OTA start

   ~~~~~~{.cpp}
   telux::common::Status otaStartStatus = telux::common::Status::FAILED;
   OtaOperation otaOperation = OtaOperation::START;
   std::promise<ErrorCode> p;

   otaStartStatus = fsManager->prepareForOta(
       otaOperation, [&p, fsManager](ErrorCode error) { p.set_value(error); });

   if (otaStartStatus != telux::common::Status::SUCCESS) {
      std::cout << "Request to prepare for ota start : ";
      Utils::printStatus(otaStartStatus);
      exit(1);
   } else {
      std::cout << "Request to prepare for ota start successful";
      ErrorCode error = p.get_future().get();
      std::cout << "Prepare for ota start with result: " << Utils::getErrorCodeAsString(error)
                << std::endl;
   }
   ~~~~~~

### 9. The client can start the OTA update

### 10. Once the OTA is complete, indicate the update status to filesystem manager

   ~~~~~~{.cpp}
   telux::common::Status otaEndStatus = telux::common::Status::FAILED;
   OperationStatus operationStatus = OperationStatus::SUCCESS;
   std::promise<ErrorCode> p;

   otaEndStatus = fsManager->otaCompleted(
       operationStatus, [&p, fsManager](ErrorCode error) { p.set_value(error); });

   if (otaEndStatus != telux::common::Status::SUCCESS) {
      std::cout << "Ota completion for update succeed request : ";
      Utils::printStatus(otaEndStatus);
      // Note: If OTA completion result in a failure, the client needs to start
      // with prepareForOta
   } else {
      std::cout << "Ota completion for update succeed request successful";
      ErrorCode error = p.get_future().get();
      std::cout << " ota completed for update succeed with result: " << Utils::getErrorCodeAsString(error)
                << std::endl;
   }
   ~~~~~~

### 11. If user decides to mirror the filesystem, start AB sync

   ~~~~~~{.cpp}
   telux::common::Status abSyncStatus = telux::common::Status::FAILED;
   std::promise<ErrorCode> p;

   abSyncStatus = fsManager->startAbSync([&p, fsManager](ErrorCode error) { p.set_value(error); });

   if (abSyncStatus != telux::common::Status::SUCCESS) {
      std::cout << "Request to start absync : ";
      Utils::printStatus(abSyncStatus);
   } else {
      std::cout << "Request to start absync successful";
      ErrorCode error = p.get_future().get();
      std::cout << "Start absync with result: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
   ~~~~~~

### 12. Clean-up when we do not need to listen anything

   ~~~~~~{.cpp}
   fsManager->deregisterListener(otaOperationsListener);
   otaOperationsListener = nullptr;
   fsManager = nullptr;
   ~~~~~~
