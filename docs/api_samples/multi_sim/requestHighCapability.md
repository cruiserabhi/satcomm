Request to find out which SIM/slot supports advance radio Technology {#requestHighCapability}
========================================================

This sample application demonstrates which SIM/slot supports advance radio Technology

### 1. Implement ResponseCallback interface to receive subsystem initialization status and
###    callback interface to fetch requestHighCapability slot details.

   ~~~~~~{.cpp}
   void requestHighCapabilityResponse(int slotId, telux::common::ErrorCode error) {

    std::cout << "Slot with high capability: " << slotId << std::endl;
    std::cout << "Request high capability request errorCode: " << static_cast<int>(error);

   }

   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << MultiSimManager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << MultiSimManager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }
   ~~~~~~

### 2. Get the PhoneFactory and MultiSimManager instance and wait for MultiSimManager subsystem subsystem to be ready

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto multiSimMgr = phoneFactory.getMultiSimManager(initResponseCb);
   if(multiSimMgr == NULL) {
      std::cout << " Failed to get MultiSimManager instance" << std::endl;
      return -1;
   }

   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize MultiSimManager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 4. RequestHighCapability information

   if(multiSimMgr != nullptr) {
       multiSimMgr_->requestHighCapability(requestHighCapabilityResponse);
   }
   ~~~~~~
