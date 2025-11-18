/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This application demonstrates how to get modem configuration, auto selection
 * mode and the active config. The steps are as follows:
 *
 * 1. Get a ConfigFactory instance.
 * 2. Get a IModemConfigManager instance from the ConfigFactory.
 * 3. Wait for the config service to become available.
 * 4. Retrieve all configs present in the modem's storage.
 * 5. Retrieve selection mode of the configs.
 * 6. Retrieve the active config.
 *
 * Usage:
 * # ./modem_config_app
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/config/ConfigFactory.hpp>
#include <telux/config/ModemConfigManager.hpp>

class ModemConfigListener : public std::enable_shared_from_this<ModemConfigListener> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &configFactory = telux::config:: ConfigFactory::getInstance();

        /* Step - 2 */
        modemConfigMgr_ = configFactory.getModemConfigManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!modemConfigMgr_) {
            std::cout << "Can't get IModemConfigManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Config service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int getConfigurationFilesInfo() {
        int count = 0;
        std::string type;
        telux::common::Status status;

        auto responseCb = std::bind(
            &ModemConfigListener::onConfigListAvailable, this,
            std::placeholders::_1, std::placeholders::_2);

        /* Step - 4 */
        status = modemConfigMgr_->requestConfigList(responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request config list, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForResponse()) {
            std::cout << "Failed to request config list, err " <<
                static_cast<int>(errorCode_) << std::endl;
            return -EIO;
        }

        std::cout << "\nCurrent configs are:" << std::endl;
        for (auto &config : configList_) {
            std::cout << "Config No  : " << count << std::endl;
            if (config.type == telux::config::ConfigType::HARDWARE) {
                type = "HARDWARE";
            } else if(config.type == telux::config::ConfigType::SOFTWARE) {
                type = "SOFTWARE";
            } else {
                type = "";
            }
            std::cout << "Type        : " << type << std::endl;
            std::cout << "Size        : " << static_cast<uint32_t>(config.size) << std::endl;
            std::cout << "Version     : " << static_cast<uint32_t>(config.version) << std::endl;
            std::cout << "Description : " << config.desc << std::endl;
            count++;
        }

        return 0;
    }

    int retrieveAutoSelectionMode() {
        telux::common::Status status;

        auto responseCb = std::bind(
            &ModemConfigListener::onAutoSelectionAvailable, this,
            std::placeholders::_1, std::placeholders::_2);

        /* Step - 5 */
        status = modemConfigMgr_->getAutoSelectionMode(responseCb, DEFAULT_SLOT_ID);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't get selection mode, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForResponse()) {
            std::cout << "Failed to get selection mode, err " <<
                static_cast<int>(errorCode_) << std::endl;
            return -EIO;
        }

        if (selectionMode_ == telux::config::AutoSelectionMode::DISABLED) {
            std::cout << "Auto selection is disabled" << std::endl;
        } else {
            std::cout << "Auto selection is enabled" << std::endl;
        }

        return 0;
    }

    int getActiveConfiguration() {
        std::string type;
        telux::common::Status status;

        auto responseCb = std::bind(
            &ModemConfigListener::onActiveConfigAvailable, this,
            std::placeholders::_1, std::placeholders::_2);

        /* Step - 6 */
        /* Get active config, this will error out if only default configs are active */
        status = modemConfigMgr_->getActiveConfig(
            telux::config::ConfigType::SOFTWARE, responseCb, DEFAULT_SLOT_ID);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't get active config, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        if (!waitForResponse()) {
            std::cout << "Failed to get active config, err " <<
                static_cast<int>(errorCode_) << std::endl;
            return -EIO;
        }

        std::cout << "Current active configuration:" << std::endl;
        if (configInfo_.type == telux::config::ConfigType::HARDWARE) {
            type = "HARDWARE";
        } else if(configInfo_.type == telux::config::ConfigType::SOFTWARE) {
                type = "SOFTWARE";
        } else {
                type = "";
        }

        std::cout << "Type        : " << type << std::endl;
        std::cout << "Size        : " << static_cast<uint32_t>(configInfo_.size) << std::endl;
        std::cout << "Version     : " << static_cast<uint32_t>(configInfo_.version) << std::endl;
        std::cout << "Description : " << configInfo_.desc << std::endl;

        return 0;
    }

    /* Receives response of the requestConfigList() request */
    void onConfigListAvailable(std::vector<telux::config::ConfigInfo> configList,
            telux::common::ErrorCode error) {

        std::lock_guard<std::mutex> lock(updateMutex_);
        std::cout << "\nonConfigListAvailable()" << std::endl;
        errorCode_ = error;
        configList_ = configList;
        updateCV_.notify_one();
    }

    /* Receives response of the getAutoSelectionMode() request */
    void onAutoSelectionAvailable(telux::config::AutoSelectionMode selectionMode,
            telux::common::ErrorCode error) {

        std::lock_guard<std::mutex> lock(updateMutex_);
        std::cout << "\nonAutoSelectionAvailable()" << std::endl;
        errorCode_ = error;
        selectionMode_ = selectionMode;
        updateCV_.notify_one();
    }

    /* Receives response of the getActiveConfig() request */
    void onActiveConfigAvailable(telux::config::ConfigInfo configInfo,
            telux::common::ErrorCode error) {

        std::lock_guard<std::mutex> lock(updateMutex_);
        std::cout << "\nonActiveConfigAvailable()" << std::endl;
        errorCode_ = error;
        configInfo_ = configInfo;
        updateCV_.notify_one();
    }

    bool waitForResponse() {
        int const DEFAULT_TIMEOUT_SECONDS = 5;
        std::unique_lock<std::mutex> lock(updateMutex_);

        auto cvStatus = updateCV_.wait_for(lock,
            std::chrono::seconds(DEFAULT_TIMEOUT_SECONDS));

        if (cvStatus == std::cv_status::timeout) {
            std::cout << "Timedout" << std::endl;
            errorCode_ = telux::common::ErrorCode::TIMEOUT_ERROR;
            return false;
        }

        return true;
    }

 private:
    std::mutex updateMutex_;
    std::condition_variable updateCV_;
    telux::common::ErrorCode errorCode_;
    telux::config::ConfigInfo configInfo_;
    telux::config::AutoSelectionMode selectionMode_;
    std::vector<telux::config::ConfigInfo> configList_;
    std::shared_ptr<telux::config::IModemConfigManager> modemConfigMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ModemConfigListener> app;

    try {
        app = std::make_shared<ModemConfigListener>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate ModemConfigListener" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->getConfigurationFilesInfo();
    if (ret < 0) {
        return ret;
    }

    ret = app->retrieveAutoSelectionMode();
    if (ret < 0) {
        return ret;
    }

    ret = app->getActiveConfiguration();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nModem config app exiting" << std::endl;
    return 0;
}
