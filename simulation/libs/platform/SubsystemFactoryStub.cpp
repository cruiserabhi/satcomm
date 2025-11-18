/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include "SubsystemFactoryStub.hpp"

namespace telux {
namespace platform {

SubsystemFactoryStub::SubsystemFactoryStub() {
}

SubsystemFactoryStub::~SubsystemFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
}

SubsystemFactory::SubsystemFactory() {
}

SubsystemFactory::~SubsystemFactory() {
    LOG(DEBUG, __FUNCTION__);
}

SubsystemFactory &SubsystemFactoryStub::getInstance() {
    static SubsystemFactoryStub instance;
    return instance;
}

SubsystemFactory &SubsystemFactory::getInstance() {
    return SubsystemFactoryStub::getInstance();
}

/**
 * Gets a SubsystemFactoryStub instance.
 *
 * @return Shared pointer to the SubsystemFactoryStub object.
 */
std::shared_ptr<ISubsystemManager> SubsystemFactoryStub::getSubsystemManager(
    telux::common::InitResponseCb initCallback) {
    LOG(DEBUG, __FUNCTION__);

    std::function<std::shared_ptr<ISubsystemManager>(InitResponseCb)> createAndInit
        = [this](telux::common::InitResponseCb initCb) ->
        std::shared_ptr<ISubsystemManager> {
        std::shared_ptr<SubsystemManagerStub> manager =
        std::make_shared<SubsystemManagerStub>();
        if (telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };

    auto type = std::string("Subsystem manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
        " , callback = ", &initCompleteCallbacks_);
    auto manager = getManager<ISubsystemManager>(type, subsysMgr_,
        initCompleteCallbacks_, initCallback, createAndInit);
    return manager;
}

}  // End of namespace platform
}  // End of namespace telux
