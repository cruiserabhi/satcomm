/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SatcomFactoryStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"

#include "NtnManagerStub.hpp"

namespace telux {
namespace satcom {

SatcomFactoryStub::SatcomFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
}

SatcomFactoryStub::~SatcomFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
    auto manager = ntnManager_.lock();
    ntnCallbacks_.clear();
}

SatcomFactory::SatcomFactory() {
    LOG(DEBUG, __FUNCTION__);
}

SatcomFactory::~SatcomFactory() {
    LOG(DEBUG, __FUNCTION__);
}

SatcomFactory &SatcomFactoryStub::getInstance() {
    static SatcomFactoryStub instance;
    return instance;
}

SatcomFactory &SatcomFactory::getInstance() {
    return SatcomFactoryStub::getInstance();
}

std::shared_ptr<INtnManager> SatcomFactoryStub::getNtnManager(
    telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::satcom::INtnManager>(telux::common::InitResponseCb)>
        createAndInit = [this](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::satcom::INtnManager> {
        std::shared_ptr<telux::satcom::NtnManagerStub> manager
            = std::make_shared<telux::satcom::NtnManagerStub>(
              );
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            LOG(ERROR, __FUNCTION__, " DataFactory unable to initialize Ntn Manager");
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Ntn manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str());
    auto manager = getManager<telux::satcom::INtnManager>(
        type, ntnManager_, ntnCallbacks_, clientCallback, createAndInit);
    return manager;
}

}  // namespace satcom
}  // namespace telux
