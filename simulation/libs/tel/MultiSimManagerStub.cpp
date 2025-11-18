/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "MultiSimManagerStub.hpp"

#define FIRST_SIM_SLOT_ID 1
#define INIT_DELAY 100

namespace telux {

namespace tel {

MultiSimManagerStub::MultiSimManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status MultiSimManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if(!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    auto status = taskQ_->add(f);
    return status;
}

MultiSimManagerStub::~MultiSimManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void MultiSimManagerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
}

void MultiSimManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(callback) {
        auto f = std::async(std::launch::async, [this, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(INIT_DELAY));
            if (callback) {
                callback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
        }).share();
        taskQ_->add(f);
    }
}

std::future<bool> MultiSimManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
        [this]() {
            while (!isSubsystemReady()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(INIT_DELAY));
            }
            return(isSubsystemReady());});
    return((ready_future));
}

telux::common::ServiceStatus MultiSimManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::Status MultiSimManagerStub::registerListener(
    std::weak_ptr<IMultiSimListener> listener) {
    return telux::common::Status::SUCCESS;
}

telux::common::Status MultiSimManagerStub::deregisterListener(
    std::weak_ptr<IMultiSimListener> listener) {
    return telux::common::Status::SUCCESS;
}

bool MultiSimManagerStub::isSubsystemReady() {
    return true;
}

telux::common::Status MultiSimManagerStub::getSlotCount(int &count) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::requestHighCapability(HighCapabilityCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::setHighCapability(int slotId,
    common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::switchActiveSlot(SlotId slotId,
    common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::requestSlotStatus(SlotStatusCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

void MultiSimManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
}

} // end of namespace tel

} // end of namespace telux