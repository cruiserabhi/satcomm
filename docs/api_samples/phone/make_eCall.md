Make eCall {#make_eCall}
========================

This sample application demonstrates how to make an emergency (E112) voice call.

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

### 5. Initialize the data required for eCall such as eCallMsdData,emergencyCategory and eCallVariant

   ~~~~~~{.cpp}
   ECallCategory emergencyCategory = ECallCategory::VOICE_EMER_CAT_AUTO_ECALL;
   ECallVariant eCallVariant = ECallVariant::ECALL_TEST;

   // Instantiate ECallMsdData structure and populate it with valid information
   // such as Latitude, Longitude etc.
   // Parameter values mentioned here are for illustrative purposes only.
   ECallMsdData eCallMsdData;
   eCallMsdData.msdData.msdVersion = 2;
   eCallMsdData.msdData.messageIdentifier = 1; // Each MSD message should bear a unique id
   eCallMsdData.optionals.recentVehicleLocationN1Present = true;
   eCallMsdData.optionals.recentVehicleLocationN2Present = true;
   eCallMsdData.optionals.numberOfPassengersPresent = 2;
   eCallMsdData.msdData.control.automaticAvtivation = true;
   eCallMsdData.control.testCall = true;
   eCallMsdData.control.positionCanBeTrusted = true;
   eCallMsdData.control.vehicleType = ECallVehicleType::PASSENGER_VEHICLE_CLASS_M1;
   eCallMsdData.msdData.vehicleIdentificationNumber.isowmi = "ECA";
   eCallMsdData.msdData.vehicleIdentificationNumber.isovds = "LLEXAM";
   eCallMsdData.msdData.vehicleIdentificationNumber.isovisModelyear = "P";
   eCallMsdData.msdData.vehicleIdentificationNumber.isovisSeqPlant = "LE02013";
   eCallMsdData.msdData.vehiclePropulsionStorage.gasolineTankPresent = true;
   eCallMsdData.msdData.vehiclePropulsionStorage.dieselTankPresent = false;
   eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas = false;
   eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas = false;
   eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage = false;
   eCallMsdData.vehiclePropulsionStorage.hydrogenStorage = false;
   eCallMsdData.vehiclePropulsionStorage.otherStorage = false;
   eCallMsdData.timestamp = 1367878452;
   eCallMsdData.vehicleLocation.positionLatitude = 123;
   eCallMsdData.vehicleLocation.positionLongitude = 1234;
   eCallMsdData.msdData.vehicleDirection = 4;
   eCallMsdData.recentVehicleLocationN1.latitudeDelta = false;
   eCallMsdData.recentVehicleLocationN1.longitudeDelta = 0;
   eCallMsdData.recentVehicleLocationN2.latitudeDelta = true;
   eCallMsdData.recentVehicleLocationN2.longitudeDelta = 0;
   ~~~~~~

### 8. Send a eCall request

   ~~~~~~{.cpp}
   int phoneId = 1;
   if(callManager) {
      auto makeCallStatus = callManager->makeECall(phoneId, eCallMsdData, emergencyCategory,
                                                   eCallVariant, dialCb);
      std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;
   }
   ~~~~~~
