/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to enable/disable NTN, send non IP data and update
 * SystemSelectionSpecifier.
 * The steps are as follows:
 *
 * 1. Get a SatcomFactory instance.
 * 2. Get a INtnManager instance from the SatcomFactory.
 * 3. Wait for the service to become available.
 * 4. update System Selection Specifiers
 * 5. Enable NTN
 * 6. Perform operations such as sending non IP data over NTN network.
 * 7. Finally, when the use case is over, disable NTN.
 *
 * Usage:
 * # ./ntn_sample_app
 *
 * This will allow updating the system selection specifier, enabling NTN, sending non-IP data,
 * and disabling NTN.
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
#include <vector>
#include <cstring>
#include <cstdint>

#include <telux/common/CommonDefines.hpp>
#include <telux/satcom/SatcomFactory.hpp>
#include <telux/satcom/NtnManager.hpp>

using namespace telux::satcom;

class NtnApp : public INtnListener,
    public std::enable_shared_from_this<NtnApp> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &satcomFactory = SatcomFactory::getInstance();

        /* Step - 2 */
        ntnMgr_ = satcomFactory.getNtnManager(
            [&p](telux::common::ServiceStatus srvStatus) {
            p.set_value(srvStatus);
        });

        if (!ntnMgr_) {
            std::cout << "Can't get INtnManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "NTN service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        auto status = ntnMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int enableNtn(std::string iccid)
    {
        telux::common::ErrorCode err;

        err = ntnMgr_->enableNtn(true, true, iccid);
        if (err != telux::common::ErrorCode::SUCCESS)
        {
            std::cout << __FUNCTION__ << " Failed with error = " << static_cast<int>(err) <<
                std::endl;
            return -EIO;
        }

        std::cout << "Ntn enable complete\n";
        return 0;
    }

    void printSignalStrength(SignalStrength ss)
    {
        switch (ss)
        {
            case SignalStrength::NONE:
                std::cout << "No signal\n";
                break;
            case SignalStrength::POOR:
                std::cout << "Signal Strength : POOR\n";
                break;
            case SignalStrength::MODERATE:
                std::cout << "Signal Strength : MODERATE\n";
                break;
            case SignalStrength::GOOD:
                std::cout << "Signal Strength : GOOD\n";
                break;
            case SignalStrength::GREAT:
                std::cout << "Signal Strength : GREAT\n";
                break;
        }
    }

    void printNtnCapabilities(NtnCapabilities cap)
    {
        std::cout << __FUNCTION__ << "maxDataSize= " << cap.maxDataSize << "\n";
    }

    void printNtnState(NtnState state)
    {
        switch (state)
        {
            case NtnState::DISABLED:
                break;
                std::cout << "NtnState: DISABLED\n";
            case NtnState::OUT_OF_SERVICE:
                std::cout << "NtnState: OUT_OF_SERVICE\n";
                break;
            case NtnState::IN_SERVICE:
                std::cout << "NtnState: IN_SERVICE\n";
                break;
        }
    }

    int getSignalStrength()
    {
        telux::common::ErrorCode err;
        SignalStrength ss;

        err = ntnMgr_->getSignalStrength(ss);
        if (err != telux::common::ErrorCode::SUCCESS)
        {
            std::cout << __FUNCTION__ << " failed with error = " << static_cast<int>(err)
                << std::endl;
            return -EIO;
        }
        printSignalStrength(ss);
        return 0;
    }

    int getNtnCapabilities()
    {
        telux::common::ErrorCode err;
        NtnCapabilities cap;

        err = ntnMgr_->getNtnCapabilities(cap);

        if (err != telux::common::ErrorCode::SUCCESS)
        {
            std::cout << __FUNCTION__ << " failed with error = " << static_cast<int>(err)
                << std::endl;
            return -EIO;
        }
        printNtnCapabilities(cap);
        return 0;
    }

    void getNtnState()
    {
        NtnState state;

        state = ntnMgr_->getNtnState();
        printNtnState(state);
    }

    int updateSystemSelectionSpecifiers()
    {
        telux::common::ErrorCode err;

        // Fill params as per vendor specification.
        // These params will only work for the device camped on NTN.
        // Below are example params only for demonstration purposes
        std::vector<SystemSelectionSpecifier> params;
        params.push_back({"310", "260", {253, 255, 256}, {229011}});
        params.push_back({"310", "260", {253, 255, 256}, {228786}});

        err = ntnMgr_->updateSystemSelectionSpecifiers(params);
        if (err != telux::common::ErrorCode::SUCCESS)
        {
            std::cout << __FUNCTION__ << " failed with error = " << static_cast<int>(err)
                << std::endl;
            return -EIO;
        }
        return 0;
    }

    int sendData()
    {
        telux::common::Status status;
        const char* str = "TEST DATA";
        uint8_t *data = reinterpret_cast<uint8_t*>(const_cast<char*>(str));
        uint32_t size = strlen(str);
        TransactionId tid;
        bool isEmergency = true;

        for (int i = 0; i < 10; ++i)
        {
            status = ntnMgr_->sendData(data, size, isEmergency, tid);
            std::unique_lock<std::mutex> lock{ mtx };
            cv.wait_for(lock, std::chrono::seconds(30));
            std::cout << __FUNCTION__ << " iteration " << i + 1 << " status = "
                << static_cast<int>(status) << std::endl;
        }
        return 0;
    }

    int abortData()
    {
        telux::common::ErrorCode err;
        err = ntnMgr_->abortData();

        if (err != telux::common::ErrorCode::SUCCESS)
        {
            std::cout << "abortData Failed with error = " << static_cast<int>(err) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int disableNtn()
    {
        telux::common::ErrorCode err;

        err = ntnMgr_->enableNtn(false, true, "");
        if (err != telux::common::ErrorCode::SUCCESS)
        {
            std::cout << "disableNtn Failed with error = " << static_cast<int>(err) << std::endl;
            return -EIO;
        }
        return 0;
    }

    void onNtnStateChange(NtnState newState)
    {
        printNtnState(newState);
    }

    void onCapabilitiesChange(NtnCapabilities capabilities) {
        printNtnCapabilities(capabilities);
    }

    void onSignalStrengthChange(SignalStrength signalStrength) {
        printSignalStrength(signalStrength);
    }

    void onServiceStatusChange(telux::common::ServiceStatus status) {
        std::cout << "Service status update: " << static_cast<int>(status) << "\n";
    }

    void onDataAck(telux::common::ErrorCode err, TransactionId id) {
        std::cout << "Data ack for Transaction ID: " << id << "\n";
        std::unique_lock<std::mutex> lock{ mtx };
        cv.notify_all();
    }

    void onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) {
        std::cout << "Downlink data available:\n";
        // Process downlink data
    }

 private:
    std::shared_ptr<INtnManager> ntnMgr_;
    std::mutex mtx;
    std::condition_variable cv;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<NtnApp> app;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " [iccid of NTN profile]\n";
        return 0;
    }

    std::string iccid(argv[1]);

    try {
        app = std::make_shared<NtnApp>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate NtnApp" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    /* Step - 4 */
    ret = app->updateSystemSelectionSpecifiers();
    if (ret < 0) {
        return ret;
    }

    /* Step - 5 */
    ret = app->enableNtn(iccid);
    if (ret < 0) {
        return ret;
    }

    /* Step - 6 */
    /* Send/Receive data as per application logic  */
    ret = app->sendData();

    /* Step - 7 */
    ret = app->disableNtn();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
