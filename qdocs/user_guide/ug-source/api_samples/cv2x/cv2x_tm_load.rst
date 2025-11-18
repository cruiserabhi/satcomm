.. _cv2x-tm-load:

Setting verification load 
=========================================

This sample app demonstrates how to use the C-V2X Radio Manager API to set the verificaton load.

1. Create verification load callback method

.. code-block::

   static std::promise<telux::common::ErrorCode> gCallbackPromise;

   // Callback method for Cv2xThrottleManager->setVerificationLoad()
   static void cv2xsetVerificationLoadCallback(telux::common::ErrorCode error) {
       std::cout << "error=" << static_cast<int>(error) << std::endl;
       gCallbackPromise.set_value(error);
   }


2. Create a initialization status callback method

.. code-block::

   bool cv2xTmStatusUpdated = false;
   telux::common::ServiceStatus cv2xTmStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
   std::condition_variable cv;
   std::mutex mtx;

   auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xTmStatusUpdated = true;
        cv2xTmStatus = status;
        cv.notify_all();
   };


3. Get a handle to the ICv2xThrottleManager instance

.. code-block::

   // Get handle to Cv2xThrottleManager
   auto & cv2xFactory = Cv2xFactory::getInstance();
   auto cv2xThrottleManager = cv2xFactory.getCv2xThrottleManager(statusCb);


4. Wait for throttle manager to complete initialization

.. code-block::

   std::unique_lock<std::mutex> lck(mtx);
   cv.wait(lck, [&] { return cv2xTmStatusUpdated; });

   if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
       cv2xTmStatus) {
       std::cout << "Error: failed to initialize Cv2xThrottleManager." << std::endl;
       return EXIT_FAILURE;
   }


5. Set the verification load

.. code-block::

   cv2xThrottleManager->setVerificationLoad(load, cv2xsetVerificationLoadCallback);

   if (telux::common::ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        std::cout << "Error : failed to set verification load" << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "set verification load success" << std::endl;
    }

    gCallbackPromise = std::promise<telux::common::ErrorCode>();

