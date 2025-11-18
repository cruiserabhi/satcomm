Start/Stop cellular data call {#start_or_stop_data_call}
========================================================

This sample application demonstrates how to start or stop a cellular data call.

### 1. Implement initialization callback and get the DataFactory instance

Optionally initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

   ~~~~~~{.cpp}
   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };
   auto &dataFactory = DataFactory::getInstance();
   ~~~~~~

### 2. Get the DataConnectionManager instances

   ~~~~~~{.cpp}
   std::unique_lock<std::mutex> lck(mtx);
   dataConnMgr = dataFactory.getDataConnectionManager(slotId, initCb);
   ~~~~~~

### 3. Wait for DataConnectionManager initialization to be complete

   ~~~~~~{.cpp}
   initCv.wait(lck);
   ~~~~~~

### 3.1 Check data connection manager initialization state

If DataConnectionManager initialization failed, new initialization attempt can be accomplished
by calling step 2. If DataConnectionManager initialization succeed, proceed to step 4

   ~~~~~~{.cpp}
   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }
   ~~~~~~

### 4. Implement DataCallResponseCb callback for startDatacall

   ~~~~~~{.cpp}
   void startDataCallResponseCallBack(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                                 telux::common::ErrorCode error) {
      std::cout<< "Received callback for startDataCall" << std::endl;
      if(error == telux::common::ErrorCode::SUCCESS) {
         std::cout<< "Request sent successfully" << std::endl;
      } else {
         std::cout<< "Request failed with errorCode: " << static_cast<int>(error) << std::endl;
      }
   }
   ~~~~~~

### 5. Send a start data call request with profile ID, IpFamily type along with required callback method

   ~~~~~~{.cpp}
   dataConnectionManager->startDataCall(profileId, telux::data::IpFamilyType::IPV4V6,
                                        startDataCallResponseCallBack);
   ~~~~~~

### 6. Response callback will be called for the startDataCall response

### 7. Implement DataCallResponseCb callback for stopDatacall

   ~~~~~~{.cpp}
   void stopDataCallResponseCallBack(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                                telux::common::ErrorCode error) {
   std::cout << "Received callback for stopDataCall" << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << "Request sent successfully" << std::endl;
   } else {
      std::cout << "Request failed with errorCode: " << static_cast<int>(error) << std::endl;
   }
   ~~~~~~

### 8. Send a stop data call request with profile ID, IpFamily type along with required callback method

   ~~~~~~{.cpp}
   dataConnectionManager->stopDataCall(profileId, telux::data::IpFamilyType::IPV4V6,
                                       stopDataCallResponseCallBack);
   ~~~~~~

Now, response callback will be called for the stopDataCall response.