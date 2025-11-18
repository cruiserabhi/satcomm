.. _cv2x-rx-app:

Receiving data 
==================================

This sample app demonstrates how to use the C-V2X Radio Manager API to receive C-V2X data.

1. Create Callback methods for ICv2xRadio and ICv2xRadioManager methods

.. code-block::

   static Cv2xStatus gCv2xStatus;
   static promise<ErrorCode> gCallbackPromise;
   static shared_ptr<ICv2xRxSubscription> gRxSub;
   static uint32_t gPacketsReceived = 0u;
   static array<char, G_BUF_LEN> gBuf;

   // Resets the global callback promise
   static inline void resetCallbackPromise(void) {
       gCallbackPromise = promise<ErrorCode>();
   }

   // Callback method for Cv2xRadioManager->requestCv2xStatus()
   static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gCv2xStatus = status;
       }
       gCallbackPromise.set_value(error);
   }

   // Callback method for Cv2xRadio->createRxSubscription() and Cv2xRadio->closeRxSubscription()
   static void rxSubCallback(shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gRxSub = rxSub;
       }
       gCallbackPromise.set_value(error);
   }

Note: We can also use Lambda functions instead of defining global scope callbacks.

2. Implement initialization callback and get the Cv2xRadioManager instance

Optionally initialization callback can be provided with get manager instance. CV2X factory will call callback when manager initialization is complete.

2.1 Create a InitResponseCb lambda function

.. code-block::

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


2.2 Get a handle to the ICv2xRadioManager instance

.. code-block::

   auto & cv2xFactory = Cv2xFactory::getInstance();
   auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager(statusCb);


2.3 Wait for C-V2X Radio Manager readiness

.. code-block::

   std::unique_lock<std::mutex> lck(mtx);
   cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });


2.4 Check C-V2X Radio Manager initialization state

If cv2xRadioManager initialization failed, new initialization attempt can be accomplished by calling step 2.2. If initialization succeed, proceed to step 3.

.. code-block::

   if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 3
   }
   else {
      // Go to step 2.2 for another initialization attempt
   }


3. Request the C-V2X status

We want to verify that the C-V2X RX status is ACTIVE before we trying to receive any data.

.. code-block::

   // Get C-V2X status and make sure Rx is enabled
   assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
   assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

   if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus) {
       cout << "C-V2X RX status is active" << endl;
   }
   else {
       cerr << "C-V2X RX is inactive" << endl;
       return EXIT_FAILURE;
   }


4. Implement initialization callback and get the ICv2xRadio instance

Optionally initialization callback can be provided with get C-V2X Radio instance. CV2X Radio Manager will call callback when C-V2X Radio initialization is complete.

4.1 Create a InitResponseCb lambda function

.. code-block::

   bool cv2x_radio_status_updated = false;
   telux::common::ServiceStatus cv2xRadioStatus =
       telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

   auto cb = [&](ServiceStatus status) {
       std::lock_guard<std::mutex> lock(mtx);
       cv2x_radio_status_updated = true;
       cv2xRadioStatus = status;
       cv.notify_all();
   };


4.2 Get a handle to the ICv2xRadio instance

.. code-block::

   auto cv2xRadio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);


4.3 Wait for C-V2X Radio readiness

.. code-block::

   std::unique_lock<std::mutex> lck(mtx);
   cv.wait(lck, [&] { return cv2x_radio_status_updated; });


4.4 Check C-V2X Radio initialization state

If cv2xRadio initialization failed, new initialization attempt can be accomplished by calling step 4.2. If initialization succeed, proceed to step 5.

.. code-block::

   if (cv2xRadioStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 5
   }
   else {
      // Go to step 4.2 for another initialization attempt
   }


5. Create RX subscription and receive data using RX socket

.. code-block::

   resetCallbackPromise();
   assert(Status::SUCCESS == cv2xRadio->createRxSubscription(TrafficIpType::TRAFFIC_NON_IP,
                                                             RX_PORT_NUM,
                                                             rxSubCallback));
   assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

   // Read from the RX socket in a loop
   for (uint32_t i = 0; i < NUM_TEST_ITERATIONS; ++i) {
       // Receive from RX socket
       sampleRx();
   }


6. Close RX subscription

We supply the callback in this sample and check its status, but note that it is optional.

.. code-block::

   resetCallbackPromise();
   assert(Status::SUCCESS == cv2xRadio->closeRxSubscription(gRxSub,
                                                            rxSubCallback));
   assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

