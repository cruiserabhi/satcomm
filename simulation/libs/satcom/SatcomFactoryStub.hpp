/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SatcomFactoryStub.hpp
 *
 * @brief      Implementation of SatcomFactory
 *
 */

#ifndef SATCOMFACTORYIMPL_HPP
#define SATCOMFACTORYIMPL_HPP

#include <memory>
#include <mutex>
#include <vector>
#include "common/FactoryHelper.hpp"
#include <telux/satcom/SatcomFactory.hpp>
#include <common/AsyncTaskQueue.hpp>

namespace telux {
namespace satcom {

class SatcomFactoryStub : public SatcomFactory, public telux::common::FactoryHelper {
 public:
    static SatcomFactory &getInstance();

    virtual std::shared_ptr<telux::satcom::INtnManager> getNtnManager(
        telux::common::InitResponseCb clientCallback) override;

 private:
    SatcomFactoryStub();
    ~SatcomFactoryStub();

    std::weak_ptr<telux::satcom::INtnManager> ntnManager_;
    std::vector<telux::common::InitResponseCb> ntnCallbacks_;
};

}  // namespace satcom
}  // namespace telux

#endif  // SATCOMFACTORYIMPL_HPP
