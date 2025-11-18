Provisioning remote SIM {#remote_sim_provisioning}
==================================================

This sample app demonstrates how to use the Remote SIM Provisioning API for performing
SIM profile management operations on the eUICC such as add profile, enable/disable
profile, delete profile, query profile list, configure server address and perform
memory reset.

### 1. Implement ResponseCallback interface to receive subsystem initialization status
### 1.1 Implement ResponseCallback interface to receive SimProfile Manager subsystem initialization status

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> cbSimProfileProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if (subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << SIM Profile subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << SIM Profile subsystem initialization failed << std::endl;
      }
      cbSimProfileProm.set_value(status);
   }
   ~~~~~~

### 1.2 Implement ResponseCallback interface to receive Card Manager subsystem initialization status

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> cbCardProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if (subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << Card Manager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << Card Manager subsystem initialization failed << std::endl;
      }
      cbCardProm.set_value(status);
   }
   ~~~~~~

### 2. Get phone factory, SIM profile manager and Card manager instance

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto simProfileManager = phoneFactory.getSimProfileManager(cbSimProfileProm);
   if (simProfileManager == NULL) {
      std::cout << " Failed to get SIMProfile Manager instance" << std::endl;
      return -1;
   }
   auto cardManager = phoneFactory.getCardManager(cbCardProm);
   if (cardManager == NULL) {
      std::cout << " Failed to get Card Manager instance" << std::endl;
      return -1;
   }
   ~~~~~~

### 3. Check if SIM profile subsystem is ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = simProfileManager.getServiceStatus();
   ~~~~~~

### 3.1 If SIM profile manager subsystem is not ready, wait for it to be ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cbSimProfileProm.get_future().get();
   if (status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize SIMProfile Manager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 4. Check if card subsystem is ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cardManager.getServiceStatus();
   ~~~~~~

### 4.1 If card manager subsystem is not ready, wait for it to be ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cbCardProm.get_future().get();
   if (status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Card Manager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 5. Return/Exit the application, if SIM profile and card manager subsystem can not be initialized

   ~~~~~~{.cpp}
   if((status == SERVICE_AVAILABLE) {
      std::cout << " *** SIM profile manager subsystem and Card manager ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize SIM profile manager/Card manager subsystem"
                << std::endl;
   }
   ~~~~~~

### 6. Instantiate and register RspListener

   ~~~~~~{.cpp}
   std::shared_ptr<ISimProfileListener> listener = std::make_shared<RspListener>();
   simProfileManager.registerListener(listener);
   ~~~~~~

###### 6.1 Implementation of ISimProfileListener interface for receiving Remote SIM provisioning notifications

   ~~~~~~{.cpp}
   class RspListener : public telux::tel::ISimProfileListener {
   public:
       void onDownloadStatus(SlotId slotId, telux::tel::DownloadStatus status,
           telux::tel::DownloadErrorCause cause) override {
            // Profile download and installation status when add profile operation performed.
       }
       void onUserDisplayInfo(SlotId slotId, bool userConsentRequired,
           telux::tel::PolicyRuleMask mask) override  {
            // Based on profile policy rules received client can decide to provide user consent
            // for download and installation of profile by calling
            // ISimProfileManager::provideUserConsent
       }
       void onConfirmationCodeRequired(SlotId slotId, std::string profileName) override {
           // Provide confirmation code for the download and installation of profile by calling
           // ISimProfileManager::provideConfirmationCode
       }
   ~~~~~~

### 7. Request EID of the eUICC

   ~~~~~~{.cpp}
   auto respCb = [&](std::string eid, telux::common::ErrorCode errorCode)
       { eidCallback(eid, errorCode); };

   // Implement a callback method to get response for the EID request
   void eidCallback(std::string eid, telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "requestEid failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "requestEid succeeded." << std::endl;
   }
   // Request EID of the eUICC.
   auto card = cardManager->getCard(SlotId::DEFAULT_SLOT_ID, &status);
   status = card->requestEid(respCb);
   ~~~~~~

### 8. Add profile on the eUICC

   ~~~~~~{.cpp}
    auto respCb = [&](telux::common::ErrorCode errorCode) { addProfileCallback(errorCode); };

    // Implement a callback method to get response for the add profile request
    void addProfileCallback(telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "addProfile failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "addProfile succeeded." << std::endl;
    }
    // Add/Download profile on the eUICC.
    status = simProfileManager->addProfile(SlotId::DEFAULT_SLOT_ID, activationCode,
        confirmationCode, isUserConsentSupported, respCb);
   ~~~~~~

###### 8.1 If user consent is required for downloading the profile, the registered listener of client will be notified by invoking onUserDisplayInfo API

   Client is expected to invoke ISimProfileManager::provideUserConsent API in order to proceed
   further for downloading the profile.

   ~~~~~~{.cpp}
    void onUserDisplayInfo(SlotId slotId, bool userConsentRequired,
        telux::tel::PolicyRuleMask mask) override  {
        // Based on user info received client can decide to provide user consent for download and
        // installation of profile by calling ISimProfileManager::provideUserConsent
    }
   ~~~~~~
###### 8.2 If confirmation code is required for downloading the profile, the registered listener of client will be notified by invoking onConfirmationCodeRequired API

   Client is expected to invoke ISimProfileManager::provideConfirmationCode API in order to proceed
   further for downloading the profile.

   ~~~~~~{.cpp}
    void onConfirmationCodeRequired(SlotId slotId, std::string profileName) override {
        // Provide confirmation code for the download and installation of profile by calling
        // ISimProfileManager::provideConfirmationCode
    }
   ~~~~~~

###### 8.3 When the download of profile completes or fails, the client is notified about download status

   ~~~~~~{.cpp}
    void onDownloadStatus(SlotId slotId, telux::tel::DownloadStatus status,
        telux::tel::DownloadErrorCause cause) override {
        // Profile download and installation status is recieved here whenever add profile operation
        // is performed.
    }
   ~~~~~~

### 9. Delete profile on the eUICC

   ~~~~~~{.cpp}
   auto respCb = [&](telux::common::ErrorCode errorCode) { deleteProfileCallback(errorCode); };

   // Implement a callback method to get response for the delete profile request
   void deleteProfileCallback(telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteProfile failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "deleteProfile succeeded." << std::endl;
   }

   // Delete profile on the eUICC.
   status = simProfileManager->deleteProfile(SlotId::DEFAULT_SLOT_ID, profileId, respCb);
   ~~~~~~

### 10. Request profile list on the eUICC

   ~~~~~~{.cpp}
   auto respCb = [&](const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
        telux::common::ErrorCode errorCode) { profileListCallback(profiles, errorCode); };

   // Implement a callback method to get response for the request profile list
   void profileListCallback(const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
        telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "profileList failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "profileList succeeded." << std::endl;
   }

   // Get profile list on the eUICC.
   status = simProfileManager->requestProfileList(SlotId::DEFAULT_SLOT_ID, respCb);
   ~~~~~~

### 11. Enable/disable profile on the eUICC

   ~~~~~~{.cpp}
   auto respCb = [&](telux::common::ErrorCode errorCode) { setProfileCallback(errorCode); };

   // Implement a callback method to get response for the set profile request
   void setProfileCallback(telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setProfile failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "setProfile succeeded." << std::endl;
   }

   // Enable/disable profile on the eUICC.
   status = simProfileManager->setProfile(SlotId::DEFAULT_SLOT_ID, profileId, enable, respCb);
   ~~~~~~

### 12. Update Nickname of the profile

   ~~~~~~{.cpp}
   auto respCb = [&](telux::common::ErrorCode errorCode) { updateNicknameCallback(errorCode); };

   // Implement a callback method to get response for update nickname request
   void updateNicknameCallback(telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "updateNickname failed with error" << static_cast<int>(error) <<
                std::endl;
            return;
        }
        std::cout << "updateNickname succeeded." << std::endl;
   }

   // Update Nickname of the profile
   status = simProfileManager->updateNickName(SlotId::DEFAULT_SLOT_ID, profileId, nickname, respCb);
   ~~~~~~

### 13. Set SMDP+ server address on the eUICC

   ~~~~~~{.cpp}
   auto respCb = [&](telux::common::ErrorCode errorCode) { setServerAddressCallback(errorCode); };

   // Implement a callback method to get response for set server addresss request
   void setServerAddressCallback(telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setServerAddress failed with error" << static_cast<int>(error) <<
                std::endl;
            return;
        }
        std::cout << "setServerAddress succeeded." << std::endl;
   }

   // Set SMDP server address on the eUICC
   status = simProfileManager->setServerAddress(SlotId::DEFAULT_SLOT_ID, smdpAddress, respCb);
   ~~~~~~

### 14. Get SMDP+ and SMDS server address from the eUICC

   ~~~~~~{.cpp}
   auto respCb = [&](std::string smdpAddress,
        std::string smdsAddress, telux::common::ErrorCode errorCode) {
            requestServerAddressCallback(smdpAddress, smdsAddress, errorCode); };

   // Implement a callback method to get response for the get server addresss request
   void requestServerAddressCallback(std::string smdpAddress,
        std::string smdsAddress, telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "requestServerAddress failed with error" << static_cast<int>(error) <<
                std::endl;
            return;
        }
        std::cout << "requestServerAddress succeeded." << std::endl;
   }

   // Get SMDP+ and SMDS server address on the eUICC
   status = simProfileManager->requestServerAddress(SlotId::DEFAULT_SLOT_ID, respCb);
   ~~~~~~

### 15. Memory reset on the eUICC

   ~~~~~~{.cpp}
   auto respCb = [&](telux::common::ErrorCode errorCode) { memoryResetCallback(errorCode); };

   // Implement a callback method to get response for the memory reset request
   void memoryResetCallback(telux::common::ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "memoryReset failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "memoryReset succeeded." << std::endl;
   }

   // Memory reset on the eUICC
   status = simProfileManager->memoryReset(SlotId::DEFAULT_SLOT_ID, resetmask, respCb);
   ~~~~~~
