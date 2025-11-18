.. _cv2x-tx-app:

Transmitting data 
==================================

This sample app demonstrates how to use the C-V2X Radio Manager API to send C-V2X data.

1. Create Callback methods for ICv2xRadio and ICv2xRadioManager methods

.. code-block::

   static Cv2xStatus gCv2xStatus;
   static promise<ErrorCode> gCallbackPromise;
   static shared_ptr<ICv2xTxFlow> gSpsFlow;
   static array<char, G_BUF_LEN> gBuf;

   // Resets the global callback promise
   static inline void resetCallbackPromise(void) {
       gCallbackPromise = promise<ErrorCode>();
   }

   // Callback method for ICv2xRadioManager->requestCv2xStatus()
   static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gCv2xStatus = status;
       }
       gCallbackPromise.set_value(error);
   }

   // Callback method for ICv2xRadio->createTxSpsFlow()
   static void createSpsFlowCallback(shared_ptr<ICv2xTxFlow> txSpsFlow,
                                     shared_ptr<ICv2xTxFlow> unusedFlow,
                                     ErrorCode spsError,
                                     ErrorCode unusedError) {
       if (ErrorCode::SUCCESS == spsError) {
           gSpsFlow = txSpsFlow;
       }
       gCallbackPromise.set_value(spsError);
   }

   // Callback for ICv2xRadio->closeTxFlow()
   static void closeFlowCallback(shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
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

We want to verify that the C-V2X TX status is ACTIVE before we try to send data.

.. code-block::

   // Get C-V2X status and make sure Tx is enabled
   assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
   assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

   if (Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
       cout << "C-V2X TX status is active" << endl;
   }
   else {
       cerr << "C-V2X TX is inactive" << endl;
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


5. Create TX SPS flow and send data using TX socket

.. code-block::

   // Set SPS parameters
   SpsFlowInfo spsInfo;
   spsInfo.priority = Priority::PRIORITY_2;
   spsInfo.periodicity = Periodicity::PERIODICITY_100MS;
   spsInfo.nbytesReserved = G_BUF_LEN;
   spsInfo.autoRetransEnabledValid = true;
   spsInfo.autoRetransEnabled = true;

   // Create new SPS flow
   resetCallbackPromise();
   assert(Status::SUCCESS == cv2xRadio->createTxSpsFlow(TrafficIpType::TRAFFIC_NON_IP,
                                                        SPS_SERVICE_ID,
                                                        spsInfo,
                                                        SPS_SRC_PORT_NUM,
                                                        false,
                                                        0,
                                                        createSpsFlowCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    // Send message in a loop
    for (uint16_t i = 0; i < NUM_TEST_ITERATIONS; ++i) {
        fillBuffer();
        sampleSpsTx();
        usleep(100000u);
    }


6. Close TX SPS flow

.. code-block::

   // Deregister SPS flow
   resetCallbackPromise();
   assert(Status::SUCCESS == cv2xRadio->closeTxFlow(gSpsFlow, closeFlowCallback));
   assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

