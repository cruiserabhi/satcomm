Make Third Party Service (TPS) eCall over IMS {#make_eCall_Over_Ims}
========================

This sample application demonstrates how to make a TPS eCall over IMS.

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


### 4. Optionally, instantiate dial call instance

   ~~~~~~{.cpp}
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback> ();
   ~~~~~~


##### 4.1. Optionally, implement IMakeCallCallback interface to receive response for the dial
#####      request

   ~~~~~~{.cpp}
   class DialCallback : public IMakeCallCallback {
   public:
      void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) override;
   };

   void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
      // will be invoked with response of makeECall operation
   }
   ~~~~~~


### 5. Optionally, instantiate dial call instance

    ~~~~~~{.cpp}
    CustomSipHeader header;
    std::string contentTypeHeader;
    std::string acceptInfoHeader;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter Custom SIP Header for contentType (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        contentTypeHeader = temp;
    } else {
        std::cout << "No input, proceeding with default contentType: " << std::endl;
    }
    temp = "";
    std::cout << "Enter Custom SIP Header for acceptInfo (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        acceptInfoHeader = temp;
    } else {
        std::cout << "No input, proceeding with default acceptInfo: " << std::endl;
    }
    if (contentTypeHeader != "") {
        header.contentType = contentType;
    } else {
        header.contentType = telux::tel::CONTENT_HEADER;
    }
    if (acceptInfoHeader != "") {
        header.acceptInfo = acceptInfo;
    } else  {
        header.acceptInfo = "";
    }
    ~~~~~~


### 6. Initialize the data required for eCall such as dialnumber, msdData

   ~~~~~~{.cpp}
   std::string dialNumber = 77777777;
   std::vector<uint8_t> rawData;
   rawData = { 2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0, 128, 4, 52, 10, 140,
              65, 89, 164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8, 32, 19, 198, 68, 0,
              0, 48, 20 };
   ~~~~~~


### 7. Send a eCall request

   ~~~~~~{.cpp}
   if(callManager) {
      auto makeCallStatus = callManager->makeECall(phoneId, dialNumber, rawData, header, dialCb);
      std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;
   }
   ~~~~~~
