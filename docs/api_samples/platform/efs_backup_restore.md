EFS backup and restore {#efs_backup_restore}
============================================

This sample app demonstrates how to use APIs to request EFS backup and listen to EFS restore and backup indications.

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
   std::shared_ptr<EfsEventListener> efsEventListener = std::make_shared<EfsEventListener>();
   fsManager->registerListener(efsEventListener);
   ~~~~~~

### 6. Receive service status notifications

   ~~~~~~{.cpp}
   virtual void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override {
      PRINT_NOTIFICATION << "Filesystem service status: ";
      std::string status;
      switch (serviceStatus) {
         case telux::common::ServiceStatus::SERVICE_AVAILABLE: {
               status = "Available";
               break;
         }
         case telux::common::ServiceStatus::SERVICE_UNAVAILABLE: {
               status = "Unavailable";
               break;
         }
         case telux::common::ServiceStatus::SERVICE_FAILED: {
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

### 7. Start EFS backup whenever necessary

   ~~~~~~{.cpp}
   telux::common::Status status = fsManager->startEfsBackup();
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "Unable to start EFS backup: ";
      Utils::printStatus(status);
   }
   ~~~~~~

### 8. Receive EFS restore and backup notifications

   ~~~~~~{.cpp}
   virtual void OnEfsRestoreEvent(telux::platform::EfsEventInfo event) override {
      PRINT_NOTIFICATION
         << ": Received efs event: Restore"
         << ((event.event == telux::platform::EfsEvent::START) ? " started" : "ended");
      if (event.event == telux::platform::EfsEvent::END) {
         std::cout << " with result: " << Utils::getErrorCodeAsString(event.error) << std::endl;
      }
   }

   virtual void OnEfsBackupEvent(telux::platform::EfsEventInfo event) override {
      PRINT_NOTIFICATION
         << ": Received efs event: Backup"
         << ((event.event == telux::platform::EfsEvent::START) ? " started" : "ended");
      if (event.event == telux::platform::EfsEvent::END) {
         std::cout << " with result: " << Utils::getErrorCodeAsString(event.error) << std::endl;
      }
   }
   ~~~~~~

### 9. Clean-up when we do not need to listen anything

   ~~~~~~{.cpp}
   fsManager->deregisterListener(efsEventListener);
   efsEventListener = nullptr;
   fsManager = nullptr;
   ~~~~~~
