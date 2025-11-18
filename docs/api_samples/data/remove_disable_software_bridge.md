Remove a software bridge and its management {#remove_disable_software_bridge}
=============================================================================

This sample application demonstrates how to remove a software bridge and its management.

### 1. Implement initialization callback Get the DataFactory instances

Optionally initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

   ~~~~~~{.cpp}
   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };
    
   auto &dataFactory = telux::data::DataFactory::getInstance();
   ~~~~~~

### 2. Get the BridgeManager instances

    ~~~~~~{.cpp}
    std::unique_lock<std::mutex> lck(mtx);
    auto dataBridgeMgr  = dataFactory.getBridgeManager(initCb);
    ~~~~~~

### 3. Wait for BridgeManager initialization to be complete

   ~~~~~~{.cpp}
   initCv.wait(lck);
   ~~~~~~

### 3.1 Check BridgeManager initialization state

BridgeManager needs to be ready to remove a software bridge and disable the software bridge
management in the system. If BridgeManager initialization failed, new initialization attempt can
be accomplished by calling step 2. If BridgeManager initialization succeed, proceed to step 4.

   ~~~~~~{.cpp}
   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }
   ~~~~~~

### 4. Implement callbacks to get, remove software bridge and disable the software bridge management

   ~~~~~~{.cpp}
   auto respCbGet = [](const std::vector<BridgeInfo> &configs, telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Get software bridge info request"
                 << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
       for (auto c : configs) {
           std::cout << "Iface name: " << c.ifaceName << ", ifaceType: " << (int)c.ifaceType
                     << ", bandwidth: " << c.bandwidth << std::endl;
       }
   };
   
   auto respCbRemove = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Remove software bridge request"
                 << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };
   
   auto respCbDisable = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Disable software bridge management request is"
                 << (error == telux::common::ErrorCode::SUCCESS ? " successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };
   ~~~~~~

### 5. Get the list of software bridges configured in the system

   ~~~~~~{.cpp}
   dataBridgeMgr->requestBridgeInfo(respCbGet);
   ~~~~~~

Now, response callback will be invoked with the list of software bridges.

### 6. Remove the software bridge from the configured bridges, based on the interface name

   ~~~~~~{.cpp}
   dataBridgeMgr->removeBridge(ifaceName, respCbRemove);
   ~~~~~~

Now, response callback will be invoked with the removeBridge response.

### 7. Disable the software bridge management (if required)

   ~~~~~~{.cpp}
   bool enable = false;
   dataBridgeMgr->enableBridge(enable, respCbDisable);
   ~~~~~~

Now, response callback will be invoked with the enableBridge response.
