/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "TcuActivityManagerWrapper.hpp"

namespace telux {
namespace power {

using namespace telux::common;

TcuActivityManagerWrapper::TcuActivityManagerWrapper() {
    LOG(INFO, __FUNCTION__);
}

TcuActivityManagerWrapper::~TcuActivityManagerWrapper() {
    LOG(INFO, __FUNCTION__);
}

std::shared_ptr<TcuActivityManagerImpl> TcuActivityManagerWrapper::init(
    ClientInstanceConfig config) {
    LOG(INFO, __FUNCTION__);
    try {
        tcuActivityMgrImpl_ = std::make_shared<TcuActivityManagerImpl>(config);
    } catch (std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, e.what());
        return nullptr;
    }
    return tcuActivityMgrImpl_;
}

void TcuActivityManagerWrapper::cleanup() {
    LOG(INFO, __FUNCTION__);
    tcuActivityMgrImpl_->cleanup();
    tcuActivityMgrImpl_ = nullptr;
}

bool TcuActivityManagerWrapper::isReady() {
    return tcuActivityMgrImpl_->isReady();
}

telux::common::ServiceStatus TcuActivityManagerWrapper::getServiceStatus() {
    return tcuActivityMgrImpl_->getServiceStatus();
}

std::future<bool> TcuActivityManagerWrapper::onReady() {
    return tcuActivityMgrImpl_->onReady();
}

Status TcuActivityManagerWrapper::registerListener(std::weak_ptr<ITcuActivityListener> listener) {
    return tcuActivityMgrImpl_->registerListener(listener);
}

Status TcuActivityManagerWrapper::deregisterListener(std::weak_ptr<ITcuActivityListener> listener) {
    return tcuActivityMgrImpl_->deregisterListener(listener);
}

Status TcuActivityManagerWrapper::registerServiceStateListener(
    std::weak_ptr<IServiceStatusListener> listener) {
    return tcuActivityMgrImpl_->registerServiceStateListener(listener);
}

Status TcuActivityManagerWrapper::deregisterServiceStateListener(
    std::weak_ptr<IServiceStatusListener> listener) {
    return tcuActivityMgrImpl_->deregisterServiceStateListener(listener);
}

telux::common::Status TcuActivityManagerWrapper::setActivityState(
    TcuActivityState state, std::string machineName, telux::common::ResponseCallback callback) {
    return tcuActivityMgrImpl_->setActivityState(state, machineName, callback);
}

telux::common::Status TcuActivityManagerWrapper::setActivityState(
    TcuActivityState state, telux::common::ResponseCallback callback) {
    return tcuActivityMgrImpl_->setActivityState(state, callback);
}

TcuActivityState TcuActivityManagerWrapper::getActivityState() {
    return tcuActivityMgrImpl_->getActivityState();
}

telux::common::Status TcuActivityManagerWrapper::sendActivityStateAck(
    StateChangeResponse ack, TcuActivityState state) {
    return tcuActivityMgrImpl_->sendActivityStateAck(ack, state);
}

telux::common::Status TcuActivityManagerWrapper::sendActivityStateAck(TcuActivityStateAck ack) {
    return tcuActivityMgrImpl_->sendActivityStateAck(ack);
}

telux::common::Status TcuActivityManagerWrapper::setModemActivityState(TcuActivityState state) {
    return tcuActivityMgrImpl_->setModemActivityState(state);
}

telux::common::Status TcuActivityManagerWrapper::getMachineName(std::string &machineName) {
    return tcuActivityMgrImpl_->getMachineName(machineName);
}

telux::common::Status TcuActivityManagerWrapper::getAllMachineNames(
    std::vector<std::string> &machineNames) {
    return tcuActivityMgrImpl_->getAllMachineNames(machineNames);
}

}  // end of namespace power
}  // end of namespace telux
