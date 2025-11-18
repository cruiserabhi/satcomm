/*
 *  Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This application demonstrates how to execute pre and post OTA operations.
 * The steps are as follows:
 *
 * 1. Get a PlatformFactory instance.
 * 2. Get a IFsManager instance from the PlatformFactory.
 * 3. Wait for the file system service to become available.
 * 4. Register a listener that will receive OTA state updates.
 * 5. Prepare the device before OTA.
 * 6. Perform the OTA update.
 * 7. Do post OTA operations.
 * 8. If required, sync A and B partitions.
 * 9. Finally, deregister the listener.
 *
 * Usage:
 * # ./ota_operations_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/FsManager.hpp>
#include <telux/platform/FsListener.hpp>

class OTAOperation : public telux::platform::IFsListener,
                     public std::enable_shared_from_this<OTAOperation> {
 public:
    int init() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &platformFactory = telux::platform::PlatformFactory::getInstance();

        /* Step - 2 */
        fsManager_ = platformFactory.getFsManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!fsManager_) {
            std::cout << "Can't get IFsManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "File system service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        status = fsManager_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        /* Step - 9 */
        status = fsManager_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int prepareForOTA() {
        telux::common::ErrorCode ec;
        telux::common::Status status;
        std::promise<telux::common::ErrorCode> p{};

        /* Step - 5 */
        status = fsManager_->prepareForOta(telux::platform::OtaOperation::START,
                [&p](telux::common::ErrorCode errorCode) {
            p.set_value(errorCode);
        });

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't prepare for OTA, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        ec = p.get_future().get();
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to prepare for OTA, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int startPostOTAOperations() {
        telux::common::ErrorCode ec;
        telux::common::Status status;
        std::promise<telux::common::ErrorCode> p{};

        /* Step - 7 */
        status = fsManager_->otaCompleted(telux::platform::OperationStatus::SUCCESS,
                [&p](telux::common::ErrorCode errorCode) {
            p.set_value(errorCode);
        });

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't start post OTA operation, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        ec = p.get_future().get();
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to start post OTA operation, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int syncABPartitions() {
        telux::common::ErrorCode ec;
        telux::common::Status status;
        std::promise<telux::common::ErrorCode> p{};

        /* Step - 8 */
        status = fsManager_->startAbSync([&p](telux::common::ErrorCode errorCode) {
            p.set_value(errorCode);
        });

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't sync partition, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        ec = p.get_future().get();
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to sync partition, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        return 0;
    }

 private:
    std::shared_ptr<telux::platform::IFsManager> fsManager_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<OTAOperation> app;

    try {
        app = std::make_shared<OTAOperation>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate OTAOperation" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->prepareForOTA();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Step - 6 */
    /* Application specific logic for OTA goes here */

    ret = app->startPostOTAOperations();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->syncABPartitions();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nOTA operation app exiting" << std::endl;
    return 0;
}
