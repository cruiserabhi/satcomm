/*
 * Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This application demonstrates how to configure the WLAN. The steps are as follows:
 *
 * 1. Get a WlanFactory instance.
 * 2. Get a IWlanDeviceManager instance from the WlanFactory.
 * 3. Wait for the WLAN service to become available.
 * 4. Register a listener that will receive WLAN state change updates.
 * 5. Disable WLAN, if it is enabled currently.
 * 6. Update WLAN configuration by specifying number of the
 *    access points and number of the stations.
 * 7. Enable the WLAN for the configuration to take effect.
 * 8. Deregister the listener.
 *
 * Usage:
 * # ./wlan_config_app <number of APs> <number of STAs>
 *
 * Example - ./wlan_config_app 1 1
 *
 * File hostapd.conf and wpa_supplicant.conf contains settings for
 * the AP and STA respectively. Please refer Readme file for the
 * details about these files.
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>

class WLANConfigurator : public telux::wlan::IWlanListener,
                         public std::enable_shared_from_this<WLANConfigurator> {
 public:
    int init() {
        telux::common::ErrorCode ec;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();

        /* Step - 2 */
        wlanDevMgr_ = wlanFactory.getWlanDeviceManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!wlanDevMgr_) {
            std::cout << "Can't get IWlanDeviceManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "WLAN service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        ec = wlanDevMgr_->registerListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't register listener" << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::ErrorCode ec;

        /* Step - 8 */
        ec = wlanDevMgr_->deregisterListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int disableWLAN() {
        bool isEnabled = false;
        telux::common::ErrorCode ec;
        std::vector<telux::wlan::InterfaceStatus> ifaceStatus;

        ec = wlanDevMgr_->getStatus(isEnabled, ifaceStatus);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't get current state, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        if (isEnabled) {
            promise_ = std::promise<bool>();

            /* Step - 5 */
            ec = wlanDevMgr_->enable(false);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                std::cout << "Can't disable WLAN, err " <<
                    static_cast<int>(ec) << std::endl;
                return -EIO;
            }

            isEnabled = promise_.get_future().get();
            if (isEnabled) {
                std::cout << "Failed to disable WLAN" << std::endl;
                return -EIO;
            }
        }

        std::cout << "WLAN disabled" << std::endl;
        return 0;
    }

    int updateConfiguration(int APCount, int STACount) {
        telux::common::ErrorCode ec;

        /* Step - 6 */
        ec = wlanDevMgr_->setMode(APCount, STACount);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set config, err " << static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "\nMode set successfully" << std::endl;
        return 0;
    }

    int enableWLAN() {
        bool isEnabled = false;
        telux::common::ErrorCode ec;

        promise_ = std::promise<bool>();

        /* Step - 7 */
        ec = wlanDevMgr_->enable(true);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't enable WLAN, err " << static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        isEnabled = promise_.get_future().get();
        if (!isEnabled) {
            std::cout << "Failed to enable WLAN" << std::endl;
            return -EIO;
        }

        std::cout << "WLAN enabled" << std::endl;
        return 0;
    }

    void onEnableChanged(bool enable) {
        std::cout << "\nonEnableChanged()" << std::endl;
        std::cout << "New value: " << enable << std::endl;
        promise_.set_value(enable);
    }

 private:
    std::promise<bool> promise_;
    std::shared_ptr<telux::wlan::IWlanDeviceManager> wlanDevMgr_;
};

int main(int argc, char *argv[]) {

    int ret, numberOfAP, numberOfSTA;
    std::shared_ptr<WLANConfigurator> app;

    if (argc != 3) {
        std::cout <<
            "Usage: ./wlan_config_app <number of APs> <number of STAs>" << std::endl;
        return -EINVAL;
    }

    numberOfAP = std::atoi(argv[1]);
    numberOfSTA = std::atoi(argv[2]);

    try {
        app = std::make_shared<WLANConfigurator>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate WLANConfigurator" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->disableWLAN();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->updateConfiguration(numberOfAP, numberOfSTA);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->enableWLAN();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nWlan app exiting" << std::endl;
    return 0;
}
