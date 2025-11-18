/*
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
 * This application demonstrates how to use the filesystem manager to start
 * EFS backup and receive filesystem events like start or end of the EFS backup
 * events. The steps are as follows:
 *
 * 1. Get a PlatformFactory instance.
 * 2. Get a IFsManager instance from the PlatformFactory.
 * 3. Wait for the file system service to become available.
 * 4. Register a listener that will receive EFS backup events.
 * 5. Request to trigger an EFS backup.
 * 6. Receive status of EFS backup in the listener.
 * 7. Finally, deregister the listener.
 *
 * Usage:
 * # ./efs_backup_restore_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/FsManager.hpp>
#include <telux/platform/FsListener.hpp>

class EFSBackupRestore : public telux::platform::IFsListener,
                         public std::enable_shared_from_this<EFSBackupRestore> {
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

        /* Step - 7 */
        status = fsManager_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int triggerEFSBackup() {
        telux::common::Status status;

        /* Step - 5 */
        status = fsManager_->startEfsBackup();
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't prepare for OTA, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Backup triggered" << std::endl;
        return 0;
    }

    /* Step - 6 */
    void OnEfsBackupEvent(telux::platform::EfsEventInfo eventInfo) override {
        std::cout << "OnEfsBackupEvent()" << std::endl;
        std::string result;
        if (eventInfo.error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to backup, err " <<
                static_cast<int>(eventInfo.error) << std::endl;
            return;
        }
        result = (eventInfo.event == telux::platform::EfsEvent::START) ? "started" : "ended";
        std::cout << "Backup " << result << std::endl;
    }

 private:
    std::shared_ptr<telux::platform::IFsManager> fsManager_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<EFSBackupRestore> app;

    try {
        app = std::make_shared<EFSBackupRestore>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate EFSBackupRestore" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->triggerEFSBackup();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Wait for receiving all asynchronous responses.
     * Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::minutes(3));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nEFS backup app exiting" << std::endl;
    return 0;
}
