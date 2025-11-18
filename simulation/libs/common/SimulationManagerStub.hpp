/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SIMULATION_MANAGER_STUB_HPP
#define SIMULATION_MANAGER_STUB_HPP

#include <string>
#include <thread>
#include "AsyncTaskQueue.hpp"
#include "CommonUtils.hpp"
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/common_simulation.grpc.pb.h"
#include <google/protobuf/empty.pb.h>

#define DELAY 100

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

template<typename T>
class SimulationManagerStub {
 public:
    SimulationManagerStub(std::string manager)
        : stub_(CommonUtils::getGrpcStub<T>()) {
            LOG(DEBUG, __FUNCTION__, ":: ", manager);
            exiting_ = false;
    }

    virtual ~SimulationManagerStub() {
        LOG(DEBUG, __FUNCTION__);
        exiting_ = true;
        cv_.notify_all();
    }

    telux::common::Status init(InitResponseCb callback) {
        LOG(DEBUG, __FUNCTION__);
        telux::common::Status status = telux::common::Status::SUCCESS;
        initCb_ = callback;
        status = init();
        if ((status != telux::common::Status::SUCCESS) &&
            (status != telux::common::Status::ALREADY)) {
            cleanup();
            return status;
        }

        auto f = std::async(std::launch::async, [this]() {
                this->initSync();
        }).share();
        taskQ_.add(f);

        return status;
    }

 protected:
    virtual telux::common::Status init() = 0;
    virtual void cleanup() = 0;
    virtual uint32_t getInitCbDelay() = 0;
    virtual telux::common::Status initSyncComplete(
            telux::common::ServiceStatus srvcStatus) = 0;
    virtual void notifyServiceStatus(telux::common::ServiceStatus srvcStatus) = 0;
    virtual void setInitCbDelay(uint32_t cbDelay) = 0;

    telux::common::ServiceStatus getServiceStatus() {
        LOG(DEBUG, __FUNCTION__);
        std::lock_guard<std::mutex> lock(srvcStatusMtx_);
        return serviceStatus_;
    }

    void setServiceReady(telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(srvcReadyMtx_);
        if (serviceReady_ != status) {
            serviceReady_ = status;
            cv_.notify_all();
        }
    }

    void setServiceStatus(telux::common::ServiceStatus status) {
        {
            std::lock_guard<std::mutex> lock(srvcStatusMtx_);
            if (serviceStatus_ == status) {
                return;
            }
            serviceStatus_ = status;
            if (status != ServiceStatus::SERVICE_AVAILABLE) {
                isInitsyncTriggered_ = false;
            }
        }

        if ((initCb_) && (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(getInitCbDelay()));
            initCb_(status);
        }
        // Notify service status to TelSDK clients
        notifyServiceStatus(status);
    }

    bool isReady() {
        return (serviceReady_ == telux::common::ServiceStatus::SERVICE_AVAILABLE);
    }

    std::future<bool> onReady() {
        auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
        return f;
    }

    void initSync() {
        LOG(DEBUG, __FUNCTION__);
        uint32_t cbDelay = 0;
        telux::common::ServiceStatus serviceStatus = ServiceStatus::SERVICE_FAILED;
        {
            std::lock_guard<std::mutex> lock(srvcStatusMtx_);
            if (isInitsyncTriggered_) {
                LOG(DEBUG, __FUNCTION__, ": Initialization is already triggered");
                return;
            }
            isInitsyncTriggered_ = true;
        }
        waitForServiceReady(serviceStatus, cbDelay);
        setInitCbDelay(cbDelay);
        setServiceReady(serviceStatus);
        if (serviceStatus != ServiceStatus::SERVICE_FAILED) {
            auto status = initSyncComplete(serviceStatus);
            if (status != telux::common::Status::SUCCESS) {
                LOG(ERROR, __FUNCTION__, ":: Failed to register for thermal indications.");
                serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
            }
        }
        setServiceStatus(serviceStatus);
    }

    std::unique_ptr<typename T::Stub> stub_;

 private:
    bool waitForInitialization() {
        std::unique_lock<std::mutex> cvLock(srvcReadyMtx_);
        //TODO:  shall NOT block if notified by setServiceStatus(SERVICE_UNAVAILABLE)
        while(serviceReady_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE && !exiting_) {
            cv_.wait(cvLock);
        }

        return (serviceReady_ == telux::common::ServiceStatus::SERVICE_AVAILABLE);
    }

    void waitForServiceReady(telux::common::ServiceStatus &srvcStatus,
            uint32_t &cbDelay) {
        LOG(DEBUG, __FUNCTION__);

        ::commonStub::GetServiceStatusReply response;
        const ::google::protobuf::Empty request;
        ClientContext context{};

        srvcStatus = getRemoteServiceStatus(cbDelay);
        if (srvcStatus == telux::common::ServiceStatus::SERVICE_FAILED) {
            return;
        } else if (srvcStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            grpc::Status reqstatus = stub_->InitService(&context, request, &response);
            if (reqstatus.ok()) {
                srvcStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
                if (srvcStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
                    if (!isReady() && !onReady().get()) {
                        LOG(ERROR, __FUNCTION__, ":: failed to initialize");
                        srvcStatus = telux::common::ServiceStatus::SERVICE_FAILED;
                        cbDelay = DELAY;
                        return;
                    }
                    srvcStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
                }
            } else {
                srvcStatus = telux::common::ServiceStatus::SERVICE_FAILED;
                cbDelay = DELAY;
                LOG(ERROR, __FUNCTION__, ":: failed to initialize");
                return;
            }
        }
    }

    telux::common::ServiceStatus getRemoteServiceStatus(uint32_t &cbDelay) {
        LOG(DEBUG, __FUNCTION__);
        ::commonStub::GetServiceStatusReply response;
        const ::google::protobuf::Empty request;
        ClientContext context;

        grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
        telux::common::ServiceStatus serviceStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<uint32_t>(response.delay());
        LOG(INFO, __FUNCTION__, ", serviceStatus: ", static_cast<int>(serviceStatus),
                ", Init cbDelay:: ", cbDelay);

        return serviceStatus;
    }

    telux::common::ServiceStatus serviceStatus_ =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex srvcStatusMtx_;
    telux::common::ServiceStatus serviceReady_ =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex srvcReadyMtx_;
    std::condition_variable cv_;
    bool isInitsyncTriggered_ = false;
    std::atomic<bool> exiting_;
    InitResponseCb initCb_;
    telux::common::AsyncTaskQueue<void> taskQ_;
};

#endif  // SIMULATION_MANAGER_STUB_HPP
