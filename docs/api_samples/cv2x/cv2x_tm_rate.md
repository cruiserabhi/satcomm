Obtaining adjust filter rate notification {#cv2x_tm_rate}
=========================================================

This sample app demonstrates how client can notified to adjust the incoming message filtering rate.

### 1. Implement ICv2xThrottleManagerListener interface

   ~~~~~~{.cpp}
   class Cv2xTmListener : public ICv2xThrottleManagerListener {
       public:
           void onFilterRateAdjustment(int rate) override;
    };
   ~~~~~~

### 2. Create a initialization status callback method

   ~~~~~~{.cpp}
   bool cv2xTmStatusUpdated = false;
   telux::common::ServiceStatus cv2xTmStatus =
       telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
   std::condition_variable cv;
   std::mutex mtx;

   std::cout << "Running TM app" << std::endl;

   auto statusCb = [&](telux::common::ServiceStatus status) {
       std::lock_guard<std::mutex> lock(mtx);
       cv2xTmStatusUpdated = true;
       cv2xTmStatus = status;
       cv.notify_all();
   };
   ~~~~~~

### 3. Get a handle to the ICv2xThrottleManager object

   ~~~~~~{.cpp}
   // Get handle to Cv2xThrottleManager
   auto & cv2xFactory = Cv2xFactory::getInstance();
   auto cv2xThrottleManager = cv2xFactory.getCv2xThrottleManager(statusCb);
   ~~~~~~

### 4. Wait for throttle manager to complete initialization

   ~~~~~~{.cpp}
   std::unique_lock<std::mutex> lck(mtx);
   cv.wait(lck, [&] { return cv2xTmStatusUpdated; });

   if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
       cv2xTmStatus) {
       std::cout << "Error: failed to initialize Cv2xThrottleManager." << std::endl;
       return EXIT_FAILURE;
   }
   ~~~~~~

### 5. Instantiate Cv2xTmListener

   ~~~~~~{.cpp}
   auto listener = std::make_shared<Cv2xTmListener>();
   ~~~~~~

### 6. Register listener

   ~~~~~~{.cpp}
   if (cv2xThrottleManager->registerListener(listener) !=
       telux::common::Status::SUCCESS) {
       std::cout << "Failed to register listener" << std::endl;
       return EXIT_FAILURE;
   }
   ~~~~~~

### 7. Wait for filter rate adjustment notification

   ~~~~~~{.cpp}
   void Cv2xTmListener::onFilterRateAdjustment(int rate) {
        std::cout << "Updated rate: " << rate << std::endl;
   }
   ~~~~~~
