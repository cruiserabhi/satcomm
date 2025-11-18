Make a voice call {#make_call}
=============================

This sample application demonstrates how to make a voice call.

### 1. Implement ResponseCallback interface to receive subsystem initialization status

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << Call Manager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << Call Manager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }
   ~~~~~~

### 2. Get the PhoneFactory and Call Manager instance

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto callManager = phoneFactory.getCallManager(initResponseCb);
   if(callManager == NULL) {
      std::cout << " Failed to get Call Manager instance" << std::endl;
      return -1;
   }
   ~~~~~~

### 3. Wait for Call Manager subsystem to be ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Call Manager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 4. Optionally, implement IMakeCallCallback interface to receive response for the dial request

   ~~~~~~{.cpp}
   class DialCallback : public IMakeCallCallback {
   public:
      void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
         // will be invoked with response of makeCall operation
      }
   };
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback> ();
   ~~~~~~

### 5. Send a dial request

   ~~~~~~{.cpp}
   if(callManager) {
      std::string phoneNumber("+18989531755");
      int phoneId = 1;
      auto makeCallStatus = callManager->makeCall(phoneId, phoneNumber, dialCb);
      std::cout << "Dial Call Status:" << (int)makeCallStatus << std::endl;
   }
   ~~~~~~
