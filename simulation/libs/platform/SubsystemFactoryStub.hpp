/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SUBSYSTEM_FACTORY_STUB_HPP
#define SUBSYSTEM_FACTORY_STUB_HPP

#include <telux/platform/SubsystemFactory.hpp>

#include "common/FactoryHelper.hpp"

#include "SubsystemManagerStub.hpp"

namespace telux {
namespace platform {

class SubsystemFactoryStub : public SubsystemFactory, public telux::common::FactoryHelper {

 public:
    static SubsystemFactory &getInstance();

    std::shared_ptr<ISubsystemManager> getSubsystemManager(
        telux::common::InitResponseCb initCallback = nullptr) override;

 private:
    std::vector<telux::common::InitResponseCb> initCompleteCallbacks_;
    std::weak_ptr<ISubsystemManager> subsysMgr_;

    SubsystemFactoryStub();
    ~SubsystemFactoryStub();
};

}  // End of namespace platform
}  // End of namespace telux

#endif  // SUBSYSTEM_FACTORY_STUB_HPP
