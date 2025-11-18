Get CV2X service status {#cv2x_get_status_app}
==================================================

This sample app demonstrates how to use the C-V2X Radio Manager API to get the C-V2X status.

### 1. Create a RequestCv2xStatusCallback method

   ~~~~~~{.cpp}
   static Cv2xStatus gCv2xStatus;
   static promise<ErrorCode> gCallbackPromise;

   static map<Cv2xStatusType, string> gCv2xStatusToString = {
       {Cv2xStatusType::INACTIVE, "Inactive"},
       {Cv2xStatusType::ACTIVE, "Active"},
       {Cv2xStatusType::SUSPENDED, "SUSPENDED"},
       {Cv2xStatusType::UNKNOWN, "UNKNOWN"},
   };

   // Callback method for Cv2xRadioManager->requestCv2xStatus
   static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gCv2xStatus = status;
       }
       gCallbackPromise.set_value(error);
   }
   ~~~~~~
Note: as an alternative, we can use a Lambda function which would eliminate the need for this global scope.

### 2. Implement initialization callback and get the Cv2xRadioManager instance

Optionally initialization callback can be provided with get manager instance.
CV2X factory will call callback when manager initialization is complete.

### 2.1 Create a InitResponseCb lambda function

   ~~~~~~{.cpp}
   bool cv2xRadioManagerStatusUpdated = false;
   telux::common::ServiceStatus cv2xRadioManagerStatus =
       telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
   std::condition_variable cv;
   std::mutex mtx;
   auto statusCb = [&](telux::common::ServiceStatus status) {
       std::lock_guard<std::mutex> lock(mtx);
       cv2xRadioManagerStatusUpdated = true;
       cv2xRadioManagerStatus = status;
       cv.notify_all();
   };
   ~~~~~~

### 2.2 Get a handle to the ICv2xRadioManager instance

   ~~~~~~{.cpp}
   auto & cv2xFactory = Cv2xFactory::getInstance();
   auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager(statusCb);
   ~~~~~~

### 2.3 Wait for C-V2X Radio Manager readiness

   ~~~~~~{.cpp}
   std::unique_lock<std::mutex> lck(mtx);
   cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
   ~~~~~~

### 2.4 Check C-V2X Radio Manager initialization state

If cv2xRadioManager initialization failed, new initialization attempt can be accomplished by
calling step 2.2. If initialization succeed, proceed to step 3.

   ~~~~~~{.cpp}
   if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 3
   }
   else {
      // Go to step 2.2 for another initialization attempt
   }
   ~~~~~~

### 3. Request the C-V2X status

   ~~~~~~{.cpp}
   if (Status::SUCCESS != cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback)) {
       cout << "Error : request for C-V2X status failed." << endl;
       return EXIT_FAILURE;
   }
   if (ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
       cout << "Error : failed to retrieve C-V2X status." << endl;
           return EXIT_FAILURE;
   }

   if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus) {
       cout << "C-V2X Status:" << endl
            << "  RX : " << gCv2xStatusToString[gCv2xStatus.rxStatus] << endl
            << "  TX : " << gCv2xStatusToString[gCv2xStatus.txStatus] << endl;
   }
   ~~~~~~
