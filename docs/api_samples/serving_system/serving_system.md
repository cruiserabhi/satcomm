Request service domain preference {#serving_system}
===================================================

This sample application demonstrates how to request current service domain preference.

### 1. Get phone factory and serving system manager instances

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto servingSystemMgr
      = phoneFactory.getServingSystemManager(DEFAULT_SLOT_ID);
   ~~~~~~

### 2. Wait for the serving subsystem initialization

   ~~~~~~{.cpp}
   bool subSystemStatus = servingSystemMgr->isSubsystemReady();
   ~~~~~~

### 2.1 If serving subsystem is not ready, wait for it to be ready

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::cout << "Serving subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = servingSystemMgr->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Exit the application, if serving subsystem can not be initialized

   ~~~~~~{.cpp}
   if(subSystemsStatus) {
      std::cout << " *** Serving subsystem ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize serving subsystem"
                << std::endl;
      return 1;
   }
   ~~~~~~

### 6. Implement response callback to receive response for request service domain preference

   ~~~~~~{.cpp}
   class ServiceDomainResponseCallback {
   public:
      void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
                                 telux::common::ErrorCode errorCode) {
         if(errorCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Service domain preference: "
                      << static_cast<int>(preference)
                      << std::endl;
         } else {
            std::cout << "\n setServiceDomainPreference failed, ErrorCode: "
                      << static_cast<int>(errorCode)
                      << std::endl;
         }
      }
   };
   ~~~~~~

### 7. Send request service domain preference request along with required function object

   ~~~~~~{.cpp}
   if(servingSystemMgr) {
      servingSystemMgr->requestServiceDomainPreference(
         ServiceDomainResponseCallback::serviceDomainResponse);
      std::cout << static_cast<int>(status) <<std::endl;
   }
   ~~~~~~
   
Now serviceDomainResponse() method will be invoked with current service domain preference.
