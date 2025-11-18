Enable/Disable firewall {#enable_disable_firewall}
==================================================

This sample application demonstrates how to enable/disable firewall.

### 1. Implement initialization callback and get the DataFactory instances

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

### 2. Get the FirewallManager instances

   ~~~~~~{.cpp}
   std::unique_lock<std::mutex> lck(mtx);
   auto dataFwMgr  = dataFactory.getFirewallManager(opType, initCb);
   ~~~~~~

### 3. Wait for FirewallManager initialization to be complete

   ~~~~~~{.cpp}
   initCv.wait(lck);
   ~~~~~~

### 3.1 Check FirewallManager initialization state

If FirewallManager initialization failed, new initialization attempt can be accomplished
by calling step 2. If FirewallManager initialization succeed, proceed to step 4.

   ~~~~~~{.cpp}
   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }
   ~~~~~~

### 4. Implement callback for setting firewall

   ~~~~~~{.cpp}
   auto respCb = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "setFirewall Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
   };
   ~~~~~~

### 5. set firewall mode based on profileId, enable/disable and allow/drop packets

   ~~~~~~{.cpp}
   dataFwMgr->setFirewall(profileId,fwEnable, allowPackets, respCb);
   ~~~~~~

Now, response callback will be called for the setFirewall response.
