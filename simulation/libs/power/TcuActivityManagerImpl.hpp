/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TCUACTIVITYMANAGERIMPL_HPP
#define TCUACTIVITYMANAGERIMPL_HPP

#include "telux/power/TcuActivityDefines.hpp"
#include "telux/power/TcuActivityManager.hpp"
#include "telux/power/TcuActivityListener.hpp"

#include "PowerGrpcClient.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"

namespace telux {
namespace power {

using namespace telux::common;

struct TcuActivityUserData {
    int cmdCallbackId;
    TcuActivityState prevState;
};

class TcuActivityManagerImpl : public ITcuActivityManager,
                               public telux::power::PowerGrpcTcuActivityListener,
                               public std::enable_shared_from_this<TcuActivityManagerImpl> {
 public:
    TcuActivityManagerImpl(ClientInstanceConfig config);

    bool isReady() override;

    std::future<bool> onReady() override;

    telux::common::Status getMachineName(std::string &machineName) override;

    telux::common::Status getAllMachineNames(std::vector<std::string> &machineNames) override;

    telux::common::Status setActivityState(TcuActivityState state, std::string machineName,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status init(telux::common::InitResponseCb callback);

    void cleanup(bool isExiting = true);

    telux::common::Status registerListener(std::weak_ptr<ITcuActivityListener> listener) override;

    telux::common::Status deregisterListener(std::weak_ptr<ITcuActivityListener> listener) override;

    telux::common::Status registerServiceStateListener(
        std::weak_ptr<IServiceStatusListener> listener) override;

    telux::common::Status deregisterServiceStateListener(
        std::weak_ptr<IServiceStatusListener> listener) override;

    telux::common::Status setActivityState(
        TcuActivityState state, telux::common::ResponseCallback callback = nullptr) override;

    TcuActivityState getActivityState() override;

    telux::common::Status sendActivityStateAck(TcuActivityStateAck ack) override;

    telux::common::Status sendActivityStateAck(
        StateChangeResponse ack, TcuActivityState state) override;

    telux::common::Status setModemActivityState(TcuActivityState state) override;

    void onTcuStateUpdate(TcuActivityState state, std::string tcuMachineName) override;

    void onSlaveAckStatusUpdate(std::vector<std::string> nackList,
        std::vector<std::string> noackList, std::string machName) override;

    void onMachineUpdate(MachineEvent state) override;

    ~TcuActivityManagerImpl();

 private:
    void setServiceStatusAndNotify(telux::common::ServiceStatus status);
    void setCachedTcuState(TcuActivityState state);
    TcuActivityManagerImpl(TcuActivityManagerImpl const &)            = delete;
    TcuActivityManagerImpl &operator=(TcuActivityManagerImpl const &) = delete;

    std::shared_ptr<telux::common::ListenerManager<ITcuActivityListener>> listenerMgr_;
    std::shared_ptr<telux::common::ListenerManager<IServiceStatusListener>> svcStatusListenerMgr_;
    telux::common::CommandCallbackManager cmdCallbackMgr_;
    TcuActivityState currentTcuState_;
    std::mutex mutex_;
    bool isInitsyncTriggered_ = false;
    telux::common::AsyncTaskQueue<void> taskQ_;
    std::shared_ptr<telux::power::PowerGrpcClient> grpcClient_;
    ClientInstanceConfig config_;
    std::condition_variable initCV_;

    bool waitForInitialization();
    void initSync();

    telux::common::ServiceStatus subSystemStatus_
        = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    telux::common::ServiceStatus lastReportedSvcState_
        = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    telux::common::InitResponseCb initCb_;
};

}  // end of namespace power
}  // end of namespace telux

#endif  // TCUACTIVITYMANAGERIMPL_HPP
