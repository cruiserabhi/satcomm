Request network selection mode {#network_selection}
===================================================

This sample application demonstrates how to request current network selection mode.

### 1. Get phone factory and network selection manager instances

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom;
   auto networkMgr = phoneFactory.getNetworkSelectionManager(DEFAULT_SLOT_ID,
       [&](telux::common::ServiceStatus status) {
           prom.set_value(status);
   });
   ~~~~~~

### 2. Wait for the network selection subsystem initialization

   ~~~~~~{.cpp}
   telux::common::ServiceStatus networkSelMgrStatus = prom.get_future().get();
   ~~~~~~

### 3. Exit the application, if network selection subsystem can not be initialzed

   ~~~~~~{.cpp}
   if (networkSelMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Network Selection Manager is ready " << "\n";
   } else {
       std::cout << "ERROR - Unable to initialize,"
           << " network selection manager subsystem " << std::endl;
       return 1;
   }
   ~~~~~~

### 4. Implement response callback to receive response for request network selection mode

   ~~~~~~{.cpp}
   class SelectionModeResponseCallback {
   public:
      void selectionModeResponse(
         telux::tel::NetworkModeInfo info,
         telux::common::ErrorCode errorCode) {
         if(errorCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Network selection mode: "
                      << static_cast<int>(info.mode)
                      << std::endl;
            if (networkSelectionMode == NetworkSelectionMode::MANUAL) {
                std::cout << "MCC is: " << info.mcc << ", MNC is: " << info.mnc << std::endl;
            }
         } else {
            std::cout << "\n requestNetworkSelectionMode failed, ErrorCode: "
                      << static_cast<int>(errorCode)
                      << std::endl;
         }
      }
   };
   ~~~~~~

### 5. Send requestNetworkSelectionMode along with response callback

   ~~~~~~{.cpp}
   if(networkMgr) {
      auto status = networkMgr->requestNetworkSelectionMode(
         SelectionModeResponseCallback::selectionModeResponse);
      std::cout << static_cast<int>(status) <<std::endl;
      }
   }
   ~~~~~~

Now, selectionModeResponse() callback gets invoked with current network selection mode information.
