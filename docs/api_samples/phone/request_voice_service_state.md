Request voice service state updates {#request_voice_service_state}
==========================================================

This sample application demonstrates how to request voice service state corresponding to a SIM
in the device.

### 1. Implement ResponseCallback interface to receive subsystem initialization status

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << Phone Manager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << Phone Manager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }
   ~~~~~~

### 2. Get the PhoneFactory and PhoneManager instance

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager(initResponseCb);
   if(phoneManager == NULL) {
      std::cout << " Failed to get Phone Manager instance" << std::endl;
      return -1;
   }
   ~~~~~~

### 3. Wait for Phone Manager subsystem to be ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Phone Manager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 4. Get the phone object corresponding to a phone ID

   ~~~~~~{.cpp}
   auto phone = phoneManager->getPhone(DEFAULT_PHONE_ID);
   ~~~~~~

### 5. Implement IOperatingModeCallback interface and instantiate MyOperatingModeCallback

   ~~~~~~{.cpp}
   std::promise<bool> callbackPromise;
   telux::tel::OperatingMode operatingMode;
   class MyOperatingModeCallback : public telux::tel::IOperatingModeCallback {
   public:
      void operatingModeResponse(telux::tel::OperatingMode mode, telux::common::ErrorCode error) {
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Operating Mode is : " << static_cast<int>(mode)
                << std::endl;
            operatingMode = static_cast<telux::tel::OperatingMode>(mode);
        } else {
            std::cout << "Operating Mode is : Unknown, errorCode: " << static_cast<int>(error)
            << std::endl;
        }
        callbackPromise.set_value(true);
      }
   };

   auto myOperatingModeCallback = std::make_shared<MyOperatingModeCallback>();

   void setOperatingModeResponse(telux::common::ErrorCode error) {
        std::cout << "Set Operating Mode is :, errorCode: " << static_cast<int>(error)
            << std::endl;
   }
   ~~~~~~


### 6. Request the operating mode of device and set the operating mode to ONLINE, if the operating mode is OFFLINE to perform any operations on the phone

   ~~~~~~{.cpp}
   phoneManager->requestOperatingMode(myOperatingModeCallback);
   if((callbackPromise.get_future().get()) &&
      (telux::tel::operatingMode == telux::tel::OperatingMode::OFFLINE )) {
      phoneManager->setOperatingMode(telux::tel::OperatingMode::ONLINE, &setOperatingModeResponse);
   }
   ~~~~~~

### 7. Implement IVoiceServiceStateCallback interface

   ~~~~~~{.cpp}
   class MyVoiceServiceStateCallback : public telux::tel::IVoiceServiceStateCallback {
   public:
      void voiceServiceStateResponse(const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo, telux::common::ErrorCode error) override;
   };
   ~~~~~~

### 8. Instantiate MyVoiceServiceStateCallback

   ~~~~~~{.cpp}
   auto myVoiceServiceStateCallback = std::make_shared<MyVoiceServiceStateCallback>();
   ~~~~~~

### 9. Send voice service state request

   ~~~~~~{.cpp}
   phone->requestVoiceServiceState(myVoiceServiceStateCallback);
   ~~~~~~

After receiving voiceServiceStateResponse in MyVoiceServiceStateCallback, the status of voice registration can be accessed by using the VoiceServiceInfo.
