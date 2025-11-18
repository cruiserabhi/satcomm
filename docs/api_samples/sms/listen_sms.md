Listening for incoming SMS {#listen_sms}
=======================================

This sample application demonstrates how to listen for an incoming SMS.

### 1. Implement ResponseCallback interface to receive subsystem initialization status

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << SmsManager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << SmsManager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }
   ~~~~~~

### 2. Implement ISmsListener interface to receive incoming SMS

   ~~~~~~{.cpp}
   class MySmsListener : public ISmsListener {
   public:
      void onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> message) override;
   };

   void MySmsListener::onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> smsMsg) {
      std::cout << "MySmsListener::onIncomingSms from PhoneId : " << phoneId << std::endl;
      std::cout << "smsReceived: From : " << smsMsg->toString() << std::endl;
   }
   ~~~~~~

### 3. Get the PhoneFactory and default SmsManager instance

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   std::shared_ptr<ISmsManager> smsMgr = phoneFactory.getSmsManager(initResponseCb);
   if(smsMgr == NULL) {
      std::cout << " Failed to get Sms Manager  instance" << std::endl;
      return -1;
   }
   ~~~~~~

### 4. Wait for SmsManager subsystem to be ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Sms Manager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 5. Instantiate global ISmsListener and register for incoming SMS

   ~~~~~~{.cpp}
   auto mySmsListener = std::make_shared<MySmsListener>();
   if(smsMgr) {
      smsMgr->registerListener(mySmsListener);
   }
   ~~~~~~

### 6. Wait for incoming SMS

   ~~~~~~{.cpp}
   std::cout << " *** wait for MySmsListener::onIncomingSms() to be triggered*** " << std::endl;
   std::cout << " *** Press enter to exit the application *** " << std::endl;
   std::string input;
   std::getline(std::cin, input);
   return 0;
   ~~~~~~
