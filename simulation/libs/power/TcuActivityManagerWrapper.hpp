/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TCUACTIVITYMANAGERWRPR_HPP
#define TCUACTIVITYMANAGERWRPR_HPP

#include "telux/power/TcuActivityDefines.hpp"
#include "telux/power/TcuActivityManager.hpp"
#include "telux/power/TcuActivityListener.hpp"

#include "TcuActivityManagerImpl.hpp"

namespace telux {
namespace power {

using namespace telux::common;

class TcuActivityManagerWrapper : public ITcuActivityManager {
 public:
    TcuActivityManagerWrapper();
    ~TcuActivityManagerWrapper();
    std::shared_ptr<TcuActivityManagerImpl> init(ClientInstanceConfig config);
    void cleanup();

    telux::common::Status getMachineName(std::string &machineName) override;
    telux::common::Status getAllMachineNames(std::vector<std::string> &machineNames) override;
    telux::common::Status setActivityState(TcuActivityState state, std::string machineName,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status registerListener(std::weak_ptr<ITcuActivityListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<ITcuActivityListener> listener) override;
    telux::common::Status registerServiceStateListener(
        std::weak_ptr<IServiceStatusListener> listener) override;
    telux::common::Status deregisterServiceStateListener(
        std::weak_ptr<IServiceStatusListener> listener) override;
    bool isReady() override;
    std::future<bool> onReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status setActivityState(
        TcuActivityState state, telux::common::ResponseCallback callback = nullptr) override;
    TcuActivityState getActivityState() override;
    telux::common::Status sendActivityStateAck(TcuActivityStateAck ack) override;
    telux::common::Status sendActivityStateAck(
        StateChangeResponse ack, TcuActivityState state) override;
    telux::common::Status setModemActivityState(TcuActivityState state) override;

 private:
    TcuActivityManagerWrapper(TcuActivityManagerWrapper const &)            = delete;
    TcuActivityManagerWrapper &operator=(TcuActivityManagerWrapper const &) = delete;

    std::shared_ptr<TcuActivityManagerImpl> tcuActivityMgrImpl_ = nullptr;
};

}  // end of namespace power
}  // end of namespace telux

#endif  // TCUACTIVITYMANAGERWRPR_HPP
