Enable/Disable auto selection mode {#get_and_set_auto_selection_mode}
================================================================================

This sample app demonstrates how to enable/disable auto selection mode for modem configuration.

### 1. Get the ConfigFactory instance

   ~~~~~~{.cpp}
   auto &configFactory = telux::config::ConfigFactory::getInstance();
   ~~~~~~

### 2. Get ModemConfigManager instance and Wait for sub system initialization

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> prom{};
   modemConfigManager_ = configFactory.getModemConfigManager(
        [&prom](telux::common::ServiceStatus status) {
            prom.set_value(status);
   });

   if (!modemConfigManager_) {
       std::cout << "Failed to get modem config Manager" << std::endl;
       return;
   }

   // Check if modem config subsystem is ready
   // If  modem config subsystem is not ready, wait for it to be ready
   telux::common::ServiceStatus managerStatus = modemConfigManager_->getServiceStatus();
   if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "\nModem config subsystem is not ready, Please wait ..." << std::endl;
       managerStatus = prom.get_future().get();
   }

   // Exit the application, if SDK is unable to initialize modem config subsystems
   if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return;
   }
   ~~~~~~

### 3. Set auto configuration selection mode

   ~~~~~~{.cpp}
   std::promise<bool> p;
   telux::config::AutoSelectionMode mode = telux::config::AutoSelectionMod::ENABLED;

   telux::common::Status status = modemConfigManager_->setAutoSelectionMode(mode,
           slotId_, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to set selection mode" << std::endl;
                p.set_value(false);
            }
   });

   if (status == telux::common::Status::SUCCESS) {
       std::cout << "set selection mode Request sent" << std::endl;
   } else {
       std::cout << "set selection mode Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "set selection mode succeeded." << std::endl;
   }
   ~~~~~~

### 4. Get current auto configuration selection mode

   ~~~~~~{.cpp}
   std::promise<bool> p;
   telux::config::AutoSelectionMode selMode;

   telux::common::Status status = modemConfigManager_->getAutoSelectionMode(
           [&p, &selMode](AutoSelectionMode selectionMode, telux::common::ErrorCode error) {
           if (error == telux::common::ErrorCode::SUCCESS) {
               selMode = selectionMode;
               p.set_value(true);
           } else {
               std::cout << "Failed to get selection mode" << std::endl;
               p.set_value(false);
           }
   }, slotId_);

   if (status == telux::common::Status::SUCCESS) {
       std::cout << "Get selection mode Request sent" << std::endl;
   } else {
       std::cout << "Get selection mode Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       if (selMode == telux::config::AutoSelectionMode::DISABLED) {
           std::cout << "DISABLED" << std::endl;
       } else {
           std::cout << "ENABLED" << std::endl;
       }
   }
   ~~~~~~