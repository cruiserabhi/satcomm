Deactivate/Delete modem configuration file {#deactivate_and_delete_modem_config_file}
========================================================================================

This sample app demonstrates how to deactivate and delete modem configuration file.

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

   //  Check if modem config subsystem is ready
   //  If  modem config subsystem is not ready, wait for it to be ready
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

### 3. Deactivate modem configuration

   ~~~~~~{.cpp}
   std::promise<bool> p;
   telux::common::Status status = modemConfigManager_->deactivateConfig(configType_,
           slotId_,[&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to deactivate config" << std::endl;
                p.set_value(false);
            }
   });

   if (status == telux::common::Status::SUCCESS) {
       std::cout << "Deactivate Request sent" << std::endl;
   } else {
       std::cout << "Deactivate Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "deactivate config succeeded." << std::endl;
   }
   ~~~~~~

### 4. Request configuration list from modem's storage

   ~~~~~~{.cpp}
   // configList_ is member variable to store config list.
   std::promise<bool> p;
   telux::common::Status status = modemConfigManager_->requestConfigList(
        [&p, this](std::vector<telux::config::ConfigInfo> configList,
                                        telux::common::ErrorCode errCode) {
        if (errCode == telux::common::ErrorCode::SUCCESS) {
            configList_ = configList;
            p.set_value(true);
        } else {
            std::cout << "Failed to get config list" << std::endl;
            p.set_value(false);
        }
   });

   if (status == telux::common::Status::SUCCESS) {
       std::cout << "Get Config List Request sent" << std::endl;
   } else {
       std::cout << "Get Config List Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "Config List Updated !!" << std::endl;
   }
   ~~~~~~

### 5. Delete modem configuration file from modem's storage

   ~~~~~~{.cpp}
   std::promise<bool> p;
   // configNo denotes the index of config file in config list
   configId = configList_[configNo].id;

   telux::common::Status status = modemConfigManager_->deleteConfig(configType_,
          configId, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to delete config" << std::endl;
                p.set_value(false);
            }
   });

   if (status == telux::common::Status::SUCCESS) {
       std::cout << "Delete Request sent successfully" << std::endl;
   } else {
       std::cout << "Delete Request failed" << std::endl;
   }
   if (p.get_future().get()) {
       std::cout << "Delete config succeeded." << std::endl;
   }
   ~~~~~~