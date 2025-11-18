Get TCU power state updates {#get_tcu_activity_state_notifications}
==========================================================================

This sample application demonstrates how to register for TCU-activity state notifications on the local machine, for performing any tasks before the state transition.

### 1. Implement ITcuActivityListener and IServiceStatusListener interface

   ~~~~~~{.cpp}
   class MyTcuActivityStateListener : public ITcuActivityListener,
                                      public IServiceStatusListener {
   public:
       void onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) override;
       void onServiceStatusChange(ServiceStatus status) override;
   };
   ~~~~~~

### 2. Get an instance of PowerFactory, and then obtain a TCU-activity manager instance with clientType as SLAVE and machine name as LOCAL_MACHINE

   ~~~~~~{.cpp}
   auto &powerFactory = PowerFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
   ClientInstanceConfig config;
   config.clientType = ClientType::SLAVE;
   config.clientName = "client_name_" + std::to_string(getpid());
   config.machineName =  LOCAL_MACHINE;
   auto tcuActivityManager = powerFactory.getTcuActivityManager(config,
                                 [&](telux::common::ServiceStatus status) {
                                    std::cout << " Init Callback called " << std::endl;
                                    prom.set_value(status);
                                 });
   ~~~~~~

### 3. Wait for the TCU-activity management services to be initialized and ready

   ~~~~~~{.cpp}
   if (tcuActivityStateMgr_ == nullptr) {
      std::cout << " ERROR - Failed to get manager instance" << std::endl;
   }
   std::cout << "  Waiting for TCU Activity Manager to be ready" << std::endl;
   telux::common::ServiceStatus serviceStatus = prom.get_future().get();
   ~~~~~~

### 4. Exit the application, if SDK is unable to initialize TCU-activity management service

   ~~~~~~{.cpp}
   if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << " *** TCU-activity management service is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize TCU-activity management service" << std::endl;
      return 1;
   }
   ~~~~~~

### 5. Instantiate MyTcuActivityStateListener and register for updates on TCU-activity state and its management service status

   ~~~~~~{.cpp}
   auto myTcuStateListener = std::make_shared<MyTcuActivityStateListener>();
   tcuActivityManager->registerListener(myTcuStateListener);
   tcuActivityManager->registerServiceStateListener(myTcuStateListener);
   ~~~~~~

### 6. When the TCU activity state of the local machine is about to change, the below listener callback is invoked with the new activity state

   ~~~~~~{.cpp}
   void MyTcuActivityStateListener::onTcuActivityStateUpdate(TcuActivityState tcuState, std::string machineName) {
        std::cout << std::endl << "********* TCU-activity state update *********" << std::endl;
        // Avoid long blocking calls when handling notifications
        // Perform necessary tasks and prepare for the state transition
   }
   ~~~~~~

### 7. After successfully processing the SUSPEND/SHUTDOWN notification, send an acknowledgement to convey the readiness of the application for the state transition

   ~~~~~~{.cpp}
   tcuActivityManager->sendActivityStateAck(StateChangeResponse::ACK, tcuState);
   ~~~~~~

### 8. Implement onServiceStatusChange callback to know when TCU-activity management service goes down

   ~~~~~~{.cpp}
   // When the TCU-activity management service goes down, this API is invoked
   // with status UNAVAILABLE. All TCU-activity state notifications will be
   // stopped until the status becomes AVAILABLE again.
   void MyTcuActivityStateListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "****** TCU-activity management service status update ******" << std::endl;
        // Avoid long blocking calls when handling notifications
   }
   ~~~~~~