/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to create a master client and initiate system's
 * power state change. The steps are as follows:
 *
 * 1. Get a PowerFactory instance.
 * 2. Get a ITcuActivityManager instance from the PowerFactory.
 * 3. Wait for the power service to become available.
 * 4. Register for acknowledgements.
 * 5. Initiate suspend as per system requirement.
 * 6. Wait for consolidated acknowledgement.
 * 7. Initiate resume as per system requirement.
 * 8. Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./tcuactivity_master
 *
 * This will create master client, trigger suspend, wait for 10 seconds and then
 * trigger resume.
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>

class PowerStateChanger : public telux::power::ITcuActivityListener,
                          public std::enable_shared_from_this<PowerStateChanger> {
 public:
    int init() {
        telux::common::Status status;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};
        telux::power::ClientInstanceConfig config{};

        /* Step - 1 */
        auto &powerFactory = telux::power::PowerFactory::getInstance();

        /* Step - 2 */
        config.clientType = telux::power::ClientType::MASTER;
        config.clientName = "masterClientFoo";

        tcuActivityMgr_ = powerFactory.getTcuActivityManager(
            config, [&p](telux::common::ServiceStatus srvStatus) {
            p.set_value(srvStatus);
        });

        if (!tcuActivityMgr_) {
            std::cout << "Can't get ITcuActivityManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Power service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        status = tcuActivityMgr_->registerListener(shared_from_this());
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

        /* Step - 8 */
        status = tcuActivityMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int initiateSuspend() {
        telux::common::Status status;

        /* Step - 5 */
        suspendRefused_ = false;
        responseReceived_ = false;
        acknowledgementReceived_ = false;

        status = tcuActivityMgr_->setActivityState(telux::power::TcuActivityState::SUSPEND,
            telux::power::ALL_MACHINES, [this](telux::common::ErrorCode ec) {
                std::cout << "Received response " << static_cast<int>(ec) << std::endl;
                std::lock_guard<std::mutex> lock(mutex_);
                ec_ = ec;
                responseReceived_ = true;
                cv_.notify_all();
            }
        );

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't initiate suspend, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]{ return responseReceived_; });

        if (ec_ != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't suspend, err " << static_cast<int>(ec_) << std::endl;
            return -EIO;
        }

        std::cout << "Suspend initiated" << std::endl;

        /* Step - 6 */
        cv_.wait(lock, [this]{ return acknowledgementReceived_; });

        if (status_ != telux::common::Status::SUCCESS) {
            std::cout << "Acknowledgement error, err " << static_cast<int>(status_) << std::endl;
            return -EIO;
        }
        if (suspendRefused_) {
            std::cout << "Slave client(s) refused to suspend" << std::endl;
            return -EIO;
        }

        std::cout << "Acknowledgement success" << std::endl;
        return 0;
    }

    int initiateResume() {
        telux::common::Status status;

        /* Step - 7 */
        responseReceived_ = false;
        status = tcuActivityMgr_->setActivityState(telux::power::TcuActivityState::RESUME,
            telux::power::ALL_MACHINES, [this](telux::common::ErrorCode ec) {
                std::cout << "Received response " << static_cast<int>(ec) << std::endl;
                std::lock_guard<std::mutex> lock(mutex_);
                ec_ = ec;
                responseReceived_ = true;
                cv_.notify_all();
            }
        );

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't initiate resume, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]{ return responseReceived_; });

        if (ec_ != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't resume, err " << static_cast<int>(ec_) << std::endl;
            return -EIO;
        }

        /* For resume event onSlaveAckStatusUpdate() is not called, so no need to wait */

        std::cout << "Resume initiated" << std::endl;
        return 0;
    }

    void onSlaveAckStatusUpdate(
        const telux::common::Status status,
        const std::string machineName,
        const std::vector<telux::power::ClientInfo> unresponsiveClients,
        const std::vector<telux::power::ClientInfo> nackResponseClients) {

        std::cout << "onSlaveAckStatusUpdate()" << std::endl;
        std::cout << "status " << static_cast<int>(status) << ", machine name " <<
            machineName << std::endl;

        if (unresponsiveClients.size()) {
            suspendRefused_ = true;
            std::cout << "Unresponsive client's count " <<
                unresponsiveClients.size() << std::endl;
            for (auto &client : unresponsiveClients) {
                std::cout << "client name "<< client.first <<
                    ", machine name " << client.second << std::endl;
            }
        }

        if (nackResponseClients.size()) {
            suspendRefused_ = true;
            std::cout << "NACK response client's count " <<
                nackResponseClients.size() << std::endl;
            for (auto &client : nackResponseClients) {
                std::cout << "client name "<< client.first <<
                    ", machine name " << client.second << std::endl;
            }
        }

        std::lock_guard<std::mutex> lock(mutex_);
        status_ = status;
        acknowledgementReceived_ = true;
        cv_.notify_all();
    }

 private:
    bool suspendRefused_;
    bool responseReceived_;
    bool acknowledgementReceived_;
    std::mutex mutex_;
    std::condition_variable cv_;
    telux::common::ErrorCode ec_;
    telux::common::Status status_;
    std::shared_ptr<telux::power::ITcuActivityManager> tcuActivityMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<PowerStateChanger> app;

    try {
        app = std::make_shared<PowerStateChanger>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate PowerStateChanger" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->initiateSuspend();
    if (ret < 0) {
        return ret;
    }

    /* Step - 5 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(10));

    ret = app->initiateResume();
    if (ret < 0) {
        return ret;
    }

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
