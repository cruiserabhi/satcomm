/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerStub.cpp
 *
 *
 */

#include "SensorFeatureManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "telux/sensor/SensorDefines.hpp"
#include "SensorDefinesStub.hpp"

#include <telux/common/CommonDefines.hpp>
#include <chrono>
#include <time.h>

//Default cb delay.
#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1

#define RPC_FAIL_SUFFIX " RPC Request failed - "

namespace telux {
namespace sensor {

SensorFeatureManagerStub::SensorFeatureManagerStub(){
    LOG(DEBUG, __FUNCTION__, " Creating");
    serviceStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    stub_ =  CommonUtils::getGrpcStub<SensorFeatureManagerService>();
}

SensorFeatureManagerStub::~SensorFeatureManagerStub(){
    LOG(DEBUG, __FUNCTION__);
    cleanup();
}

telux::common::ServiceStatus SensorFeatureManagerStub::getServiceStatus(){
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void SensorFeatureManagerStub::cleanup(){
    LOG(DEBUG, __FUNCTION__);
    taskQ_.shutdown();
    tcuActivityMgr_ = nullptr;
}

telux::common::Status SensorFeatureManagerStub::init(telux::common::InitResponseCb initCb){
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, initCb]() { this->initSync(initCb); }).share();
    taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

bool SensorFeatureManagerStub::getSystemState() {
    std::lock_guard<std::mutex> lock(mutex_);
    return isSystemSuspended_;
}

void SensorFeatureManagerStub::onTcuActivityStateUpdate(TcuActivityState state,
    std::string machineName) {
    LOG(DEBUG, __FUNCTION__);

    if (state == TcuActivityState::SUSPEND) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            isSystemSuspended_ = true;
            LOG(DEBUG, "isSystemSuspended_: ", isSystemSuspended_);
        }
        telux::common::Status ackStatus = tcuActivityMgr_->sendActivityStateAck(StateChangeResponse::ACK,
            state);
        if (ackStatus == telux::common::Status::SUCCESS) {
            std::cout << " Sent SUSPEND acknowledgement" << std::endl;
        } else {
            std::cout << " Failed to send SUSPEND acknowledgement !" << std::endl;
        }
    } else if (state == TcuActivityState::RESUME) {
        std::lock_guard<std::mutex> lock(mutex_);
        isSystemSuspended_ = false;
    }
}

void SensorFeatureManagerStub::initTcuPowerManager() {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status;
    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};
    telux::power::ClientInstanceConfig config{};

    std::cout << " Initializing the client as a SLAVE " << std::endl;

    config.clientType = telux::power::ClientType::SLAVE;
    config.clientName = "slaveClientSensorFeatureMgrStub";
    config.machineName = telux::power::LOCAL_MACHINE;

    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();

    // Get TCU-activity manager object
    tcuActivityMgr_ = powerFactory.getTcuActivityManager(
        config, [&p](telux::common::ServiceStatus srvStatus) {
        p.set_value(srvStatus);
    });

    if (!tcuActivityMgr_) {
        std::cout << "Can't get ITcuActivityManager" << std::endl;
        return;
    }

    // Wait for TCU-activity manager to be ready
    std::cout << " Waiting for TCU Activity Manager to be ready " << std::endl;
    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Power service unavailable, status " <<
            static_cast<int>(serviceStatus) << std::endl;
        return;
    }

    // Registering a listener for TCU-activity state updates
    status = tcuActivityMgr_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't register listener, err " <<
            static_cast<int>(status) << std::endl;
        return;
    }

    std::cout << " Registered Listener for TCU-activity state updates" << std::endl;
}

void SensorFeatureManagerStub::initSync(telux::common::InitResponseCb callback){
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    if(reqstatus.ok()) {
        std::lock_guard<std::mutex> lock(mutex_);
        serviceStatus_ = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        std::lock_guard<std::mutex> lock(mutex_);
        serviceStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
    }
    if(serviceStatus_ == ServiceStatus::SERVICE_AVAILABLE ){
        auto myself = shared_from_this();
        myself_ = myself;
        initTcuPowerManager();
    }
    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        callback(serviceStatus_);
    }
    return;
}

void SensorFeatureManagerStub::onEventUpdate(google::protobuf::Any event){
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::sensorStub::FeatureEvent>()) {
        ::sensorStub::FeatureEvent featureEvent;
        event.UnpackTo(&featureEvent);
        handleFeatureEvent(featureEvent);
    }
}

void SensorFeatureManagerStub::handleFeatureEvent(::sensorStub::FeatureEvent event){
    LOG(DEBUG, __FUNCTION__);
    std::string sensorName = " ";
    SensorFeatureEvent featureEvent;
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    featureEvent.timestamp =  (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
    featureEvent.name = event.featurename();
    featureEvent.id = event.id();
    auto bufferedEvents = std::make_shared<std::vector<SensorEvent>>();
    parseBufferedEvent(event.events(),bufferedEvents,sensorName);
    invokeEventListener(featureEvent);
    if(getSystemState()) {
        invokeBufferedEventListener(sensorName, bufferedEvents, true);
    }
}

void SensorFeatureManagerStub::parseBufferedEvent(std::string eventString,
    std::shared_ptr<std::vector<SensorEvent>> &events, std::string &sensorName) {
    LOG(DEBUG, __FUNCTION__,eventString.length());
    std::stringstream ss(eventString);
    std::vector<std::string> eventValues;
    while( ss.good() ) {
        std::string substr;
        std::getline( ss, substr, ',' );
        eventValues.push_back(substr);
    }
    int dataLength = eventValues.size();
    sensorName = eventValues[dataLength-1];
    for(int i=1;i<dataLength-1;i+=6){
        SensorEvent event;
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        event.timestamp =  (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
        event.uncalibrated.data.x = std::stof(eventValues[i]);
        event.uncalibrated.data.y = std::stof(eventValues[i+1]);
        event.uncalibrated.data.z = std::stof(eventValues[i+2]);
        event.uncalibrated.bias.x = std::stof(eventValues[i+3]);
        event.uncalibrated.bias.y = std::stof(eventValues[i+4]);
        event.uncalibrated.bias.z = std::stof(eventValues[i+5]);
        events->push_back(event);
    }
    return;
}

telux::common::Status SensorFeatureManagerStub::getAvailableFeatures(
    std::vector<SensorFeature> &features){
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::GetFeatureListReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    telux::common::Status status = telux::common::Status::FAILED;
    ::grpc::Status reqstatus = stub_->GetFeatureList(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        cbDelay = static_cast<int>(response.delay());
        std::string sensorFeatureString = response.list();
        std::stringstream ss(sensorFeatureString);
        while( ss.good() ) {
            SensorFeature sensorFeature;
            std::string substr;
            std::getline( ss, substr, ',' );
            sensorFeature.name = substr;
            features.emplace_back(sensorFeature);
        }
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    auto f = std::async(std::launch::async, [=]() {
        if (cbDelay != SKIP_CALLBACK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status SensorFeatureManagerStub::enableFeature(std::string name){
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::SensorFeatureManagerCommandReply response;
    ::sensorStub::SensorEnableFeature request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    telux::common::Status status = telux::common::Status::FAILED;
    request.set_feature(name);
    ::grpc::Status reqstatus = stub_->EnableFeature(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        cbDelay = static_cast<int>(response.delay());
        LOG(DEBUG , __FUNCTION__ , " Request Sent Successfully ");
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    auto f = std::async(std::launch::async, [=]() {
        if (cbDelay != SKIP_CALLBACK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status SensorFeatureManagerStub::disableFeature(std::string name) {
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::SensorFeatureManagerCommandReply response;
    ::sensorStub::SensorEnableFeature request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    telux::common::Status status = telux::common::Status::FAILED;
    request.set_feature(name);
    ::grpc::Status reqstatus = stub_->DisableFeature(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        cbDelay = static_cast<int>(response.delay());
        LOG(DEBUG , __FUNCTION__ , " Request Sent Successfully ");
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    auto f = std::async(std::launch::async, [=]() {
        if (cbDelay != SKIP_CALLBACK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status SensorFeatureManagerStub::registerListener(
    std::weak_ptr<ISensorFeatureEventListener> listener){
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(mutex_);
    telux::common::Status status = telux::common::Status::SUCCESS;
    auto spt = listener.lock();
    if (spt) {
        if (listeners_.size() == 0) {
            std::vector<std::string> filters = {"sensor_feature"};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(myself_, filters);
        }
        bool existing = 0;
        for (auto iter = listeners_.begin(); iter < listeners_.end(); ++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, " Register Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    } else {
        status = telux::common::Status::INVALIDPARAM;
    }
    return status;
}

telux::common::Status SensorFeatureManagerStub::deregisterListener(
    std::weak_ptr<ISensorFeatureEventListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt) {
        for (auto iter=listeners_.begin(); iter<listeners_.end(); ++iter) {
            if (spt == (*iter).lock()) {
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                iter = listeners_.erase(iter);
                status = telux::common::Status::SUCCESS;
                break;
            }
        }
    } else {
        status = telux::common::Status::INVALIDPARAM;
    }
    return status;
}

void SensorFeatureManagerStub::invokeEventListener(SensorFeatureEvent event){
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onEvent(event);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}
void SensorFeatureManagerStub::invokeBufferedEventListener(std::string sensorName,
    std::shared_ptr<std::vector<SensorEvent>> events, bool isLast){
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onBufferedEvent(sensorName,events,isLast);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }

}

}
}