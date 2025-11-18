Create static NAT entry {#create_snat_entry}
============================================

This sample application demonstrates how to create static NAT entry.

### 1. Implement initialization callback and get the DataFactory instance

Optionally, initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

   ~~~~~~{.cpp}
   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };
   
   auto &dataFactory = telux::data::DataFactory::getInstance();
   ~~~~~~

### 2. Get the NatManager instances

   ~~~~~~{.cpp}
   std::unique_lock<std::mutex> lck(mtx);
   auto dataSnatMgr  = dataFactory.getNatManager(opType);
   ~~~~~~

### 3. Wait for NatManager initialization to be complete

   ~~~~~~{.cpp}
   initCv.wait(lck);
   ~~~~~~

### 3.1 Check NatManager initialization state

If NatManager initialization failed, new initialization attempt can be accomplished
by calling step 2. If NatManager initialization succeed, proceed to step 4.

   ~~~~~~{.cpp}
   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }
   ~~~~~~

### 4. Implement callback for create SNAT entry

   ~~~~~~{.cpp}
   auto respCb = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "addStaticNatEntry"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
   };
   ~~~~~~

### 5. Create Snat entry based on profile id, local ip, local port, global port, and protocol

   ~~~~~~{.cpp}
   natConfig.addr = ipAddr;
   natConfig.port = (uint16_t)localIpPort;
   natConfig.globalPort = (uint16_t)globalIpPort;
   natConfig.proto = (uint8_t)proto;
   dataSnatMgr->addStaticNatEntry(profileId, natConfig, respCb);
   ~~~~~~

Now, response callback will be called for the addStaticNatEntry response.
