Set TCU activity state {#set_tcu_activity_state}
===========================================================================

This sample application demonstrates how to change the TCU activity state of all the application processors (machines) in the system

### 1. Implement a command response callback method

   ~~~~~~{.cpp}
   // This is invoked with error code indicating whether the request
   // was SUCCESS or FAILURE
   void commandResponse(ErrorCode error) {
       if (error == ErrorCode::SUCCESS) {
           std::cout << " Command executed successfully" << std::endl;
       } else {
           std::cout << " Command failed, errorCode: " << static_cast<int>(error) << std::endl;
       }
   }
   ~~~~~~

### 2. Implement ITcuActivityListener and IServiceStatusListener interface

   ~~~~~~{.cpp}
   class MyTcuActivityStateListener : public ITcuActivityListener,
                                      public IServiceStatusListener {
   public:
      void onMachineUpdate(const std::string machineName, const MachineEvent machineEvent);
      void onSlaveAckStatusUpdate(const telux::common::Status status,
         const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
         const std::vector<ClientInfo> nackResponseClients) override;
      void onServiceStatusChange(ServiceStatus status) override;
   };
   ~~~~~~

### 3. Get an instance of PowerFactory, and then obtain a TCU-activity manager instance with ClientType as MASTER

   ~~~~~~{.cpp}
   auto &powerFactory = PowerFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
   ClientInstanceConfig config;
   config.clientType = ClientType::MASTER;
   config.clientName = "client_name_" + std::to_string(getpid());
   config.machineName =  ALL_MACHINES;
   auto tcuActivityManager = powerFactory.getTcuActivityManager(config,
                                 [&](telux::common::ServiceStatus status) {
                                    std::cout << " Init Callback called " << std::endl;
                                    prom.set_value(status);
                                 });
   ~~~~~~

### 4. Wait for the TCU-activity management services to be initialized and ready

   ~~~~~~{.cpp}
   if (tcuActivityStateMgr_ == nullptr) {
      std::cout << " ERROR - Failed to get manager instance" << std::endl;
   }
   std::cout << "  Waiting for TCU Activity Manager to be ready" << std::endl;
   telux::common::ServiceStatus serviceStatus = prom.get_future().get();
   ~~~~~~

### 5. Exit the application, if SDK is unable to initialize TCU-activity management service

   ~~~~~~{.cpp}
   if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << " *** TCU-activity management service is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize TCU-activity management service" << std::endl;
      return 1;
   }
   ~~~~~~

### 6. Instantiate MyTcuActivityStateListener and register for updates on the triggered TCU-activity state and its management service status

   ~~~~~~{.cpp}
   auto myTcuStateListener = std::make_shared<MyTcuActivityStateListener>();
   tcuActivityManager->registerListener(myTcuStateListener);
   tcuActivityManager->registerServiceStateListener(myTcuStateListener);
   ~~~~~~

### 7. Set the TCU activity state (SUSPEND here) of all the machines in the system

   ~~~~~~{.cpp}
   telux::common::Status status = tcuActivityManager->setActivityState(TcuActivityState::SUSPEND, ALL_MACHINES, &commandResponse);

   if (status == telux::common::Status::SUCCESS) {
      // successfully passed trigger, wait for the command response callback (commandResponse) to know if trigger is accepted and getting processed
   } else {
      std::cout << " *** ERROR - Failed to trigger TCU activity state change " << std::endl;
      // failed to pass trigger check status for more information
   }
   ~~~~~~
### 8. The below listener API is invoked to notify the consolidated acknowledgement status of all the SLAVE clients, corresponding to the machine that is undergoing the transition

   ~~~~~~{.cpp}
   void void MyTcuActivityStateListener::onSlaveAckStatusUpdate(const telux::common::Status status,
      const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
      const std::vector<ClientInfo> nackResponseClients) {
      std::cout << "********* TCU-activity state update status and consolidate slave acknowledgement information *********" << std::endl;
      // Avoid long blocking calls when handling notifications

      if (status != telux::common::Status::SUCCESS) {
         std::cout << " Analyze which clients have not responded or responded with nack " << std::endl;
         if(unresponsiveClients.size() > 0) {
            std::cout << " Number of unresponsive clients : "<< unresponsiveClients.size() << std::endl;
            for (size_t i = 0; i < unresponsiveClients.size(); i++) {
               std::cout << " client name : "<< unresponsiveClients[i].first
                           << " , machine name : "<< unresponsiveClients[i].second << std::endl;
            }
         }

         if(nackResponseClients.size() > 0) {
            std::cout << " Number of clients responded with nack : "<< nackResponseClients.size()
                  << std::endl;
            for (size_t i = 0; i < nackResponseClients.size(); i++) {
                  std::cout << " client name : "<< nackResponseClients[i].first
                           << " , machine name : "<< nackResponseClients[i].second << std::endl;
            }
         }

         // If master expects to stop state transition considering the above information, then call
         // setActivityState and revert state, or else state transition will proceed after the configured
         // timeout
         tcuActivityManager->setActivityState(TcuActivityState::RESUME, desiredVMName, &commandResponse);
      }
   }
   ~~~~~~

### 9. Implement onServiceStatusChange callback to know when TCU-activity management service goes down

   ~~~~~~{.cpp}
   // When the TCU-activity management service goes down, this API is invoked
   // with status UNAVAILABLE. All TCU-activity state notifications will be
   // stopped until the status becomes AVAILABLE again.
   void MyTcuActivityStateListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "****** TCU-activity management service status update ******" << std::endl;
        // Avoid long blocking calls when handling notifications
   }
   ~~~~~~
