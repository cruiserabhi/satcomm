Enable/Disable socks {#enable_disable_socks}
============================================

This sample application demonstrates how to enable/disable socks.

### 1.Implement initialization callback and get the DataFactory instance

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

### 2. Get the SocksManager instances

   ~~~~~~{.cpp}
   std::unique_lock<std::mutex> lck(mtx);
   auto dataSocksMgr  = dataFactory.getSocksManager(opType, initCb);
   ~~~~~~

### 3. Wait for DataConnectionManager initialization to be complete

   ~~~~~~{.cpp}
   initCv.wait(lck);
   ~~~~~~

### 3.1 Check SocksManager initialization state

If SocksManager initialization failed, new initialization attempt can be accomplished
by calling step 2. If SocksManager initialization succeed, proceed to step 4.

   ~~~~~~{.cpp}
   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }
   ~~~~~~

### 4. Optionally, instantiate enable Socks callback instance

   ~~~~~~{.cpp}
   auto respCb = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "enableSocks Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error) << "\n";
      promise.set_value(1);
   };
   ~~~~~~

### 5. Enable/Disable socks using enableSocks()

   ~~~~~~{.cpp}
    dataSocksMgr->enableSocks(enable, respCb);
   ~~~~~~

Now, response callback will be called for the setFirewall response.
