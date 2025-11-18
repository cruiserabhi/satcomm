/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to get and set a config. The steps are as follows:
 *
 * 1. Get a ConfigFactory instance.
 * 2. Get a IConfigManager instance from the ConfigFactory.
 * 3. Wait for the config service to become available.
 * 4. Register listener that will be called whenever a config is changed.
 * 5. Retrieve all current configs.
 * 6. Set a particular config.
 * 7. Receive config update in the listener.
 * 8. Get a particular config.
 * 9. Finally, deregister the listener.
 *
 * Usage:
 * # ./param_config_app
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/config/ConfigFactory.hpp>
#include <telux/config/ConfigManager.hpp>

class ParamConfigListener : public telux::config::IConfigListener,
                            public std::enable_shared_from_this<ParamConfigListener> {
 public:
    int init() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &configFactory = telux::config:: ConfigFactory::getInstance();

        /* Step - 2 */
        configMgr_ = configFactory.getConfigManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!configMgr_) {
            std::cout << "Can't get IConfigManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Config service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        status = configMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register, status " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 9 */
        status = configMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int getConfigurations() {
        std::map<std::string, std::string> allConfigs;

        /* Step - 5 */
        allConfigs = configMgr_->getAllConfigs();

        std::cout << "\nCurrent configs are:" << std::endl;
        for(auto &config: allConfigs) {
            std::cout << config.first << " : " << config.second << "\n";
        }

        return 0;
    }

    int setConfiguration() {
        std::string configValue;
        telux::common::Status status;
        std::string key("FILE_LOG_LEVEL");
        std::string value("DEBUG");

        /* Step - 6 */
        status = configMgr_->setConfig(key, value);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't set config, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        /* Step - 8 */
        /* Optional, check if the config got set or not */
        configValue = configMgr_->getConfig(key);
        if (configValue.compare(value) != 0) {
            std::cout << "configValue not set: " << configValue << std::endl;
            return -EIO;
        }

        return 0;
    }

    /* Step - 7 */
    void onConfigUpdate(std::string key, std::string value) {
        std::cout << "\nonConfigUpdate()" << std::endl;
        std::cout << "Updated " << key << " with new value: " << value << std::endl;
    }

 private:
    std::shared_ptr<telux::config::IConfigManager> configMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ParamConfigListener> app;

    try {
        app = std::make_shared<ParamConfigListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate ParamConfigListener" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getConfigurations();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->setConfiguration();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Wait for receiving all asynchronous responses.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(3));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nParam config app exiting" << std::endl;
    return 0;
}
