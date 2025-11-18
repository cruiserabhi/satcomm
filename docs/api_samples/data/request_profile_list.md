Get available modem profiles {#request_profile_list}
====================================================

This sample application demonstrates how to request list of available modem profiles.

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

### 2. Get DataProfileManager instances

   ~~~~~~{.cpp}
   std::unique_lock<std::mutex> lck(mtx);
   auto dataProfileMgr = dataFactory.getDataProfileManager(slotId, initCb);
   ~~~~~~

### 3. Wait for DataProfileManager initialization to be complete

   ~~~~~~{.cpp}
   initCv.wait(lck);
   ~~~~~~

### 3.1 Check DataProfileManager initialization state

If DataProfileManager initialization failed, new initialization attempt can be accomplished by
calling step 2. If initialization succeed, proceed to step 4

   ~~~~~~{.cpp}
   if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      // Go to step 2 for another initialization attempt
   }
   ~~~~~~

### 4. Instantiate requestProfileList callback

   ~~~~~~{.cpp}
   auto dataProfileListCb_ = std::make_shared<DataProfileListCallback>();
   ~~~~~~

### 4.1 Implement IDataProfileListCallback interface to know status of requestProfileList

   ~~~~~~{.cpp}
   class DataProfileListCallback : public telux::common::IDataProfileListCallback {

     virtual void onProfileListResponse(const std::vector<std::shared_ptr<DataProfile>> &profiles,
                                        telux::common::ErrorCode error) override {

      std::cout<<"Length of available profiles are "<<profiles.size()<<std::endl;
     }
   };
   ~~~~~~

### 5. Send a requestProfileList along with required callback method

   ~~~~~~{.cpp}
   telux::common::Status status = dataProfileMgr->requestProfileList(dataProfileListCb_);
   ~~~~~~

Now, receive DataProfileListCallback responses for requestProfileList request.
