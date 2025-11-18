Load and activate modem configuration {#load_and_activate_modem_config_file}
=================================================================================

This sample app demonstrates how to load and activate modem configuration.

### 1. Get the ConfigFactory instance

   ~~~~~~{.cpp}
   auto &configFactory = telux::config::ConfigFactory::getInstance();
   ~~~~~~

### 2. Get ModemConfigManager instance and wait for sub system initialization

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

    // Exit the application, if TelSDK is unable to initialize modem config subsystem
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "ERROR - Unable to initialize subsystem" << std::endl;
        return;
    }
   ~~~~~~

### 3. Load a modem config file from file path

   ~~~~~~{.cpp}
   std::promise<bool> p;
   auto status = modemConfigManager_->loadConfigFile(filePath, configType_,
             [&p](telux::common::ErrorCode error) {
             if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
             } else {
                std::cout << "Failed to load config" << std::endl;
                p.set_value(false);
             }
   });

   if(status == telux::common::Status::SUCCESS) {
       std::cout << "Load config Request sent" << std::endl;
   } else {
       std::cout << "Load config Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "Load config succeeded." << std::endl;
   }
   ~~~~~~

### 4. Request configuration list from modem's storage

   ~~~~~~{.cpp}
   // configList_ is member variable to store config list
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

### 5. Activate modem configuration

   ~~~~~~{.cpp}
   // configNo denotes index of the modem config file in in config List.
   ConfigId configId;
   configId = configList_[configNo].id;

   telux::common::Status status = modemConfigManager_->activateConfig(configType_, configId,
            slotId_, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to activate modem configuration" << std::endl;
                p.set_value(false);
            }
   });

   if (status == telux::common::Status::SUCCESS) {
       std::cout << "Activate Request sent " << std::endl;
   } else {
       std::cout << "Activate Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "config Activated !!" << std::endl;
   }
   ~~~~~~
