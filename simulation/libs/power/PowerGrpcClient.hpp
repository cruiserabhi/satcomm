/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       PowerGrpcClient.hpp
 * @brief      Power GRPC Client interacts with power-management GRPC service to send/receive GRPC
 *             requests/ indications and dispatch to respective listeners
 *
 */

#ifndef POWERGRPCCLIENT_HPP
#define POWERGRPCCLIENT_HPP

#include <vector>
#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>
#include <telux/power/TcuActivityDefines.hpp>

#include "common/TaskDispatcher.hpp"
#include "common/CommandCallbackManager.hpp"
#include "common/Logger.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "common/event-manager/ClientEventManager.hpp"

#include <grpcpp/grpcpp.h>
#include "protos/proto-src/power_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using powerStub::PowerManagerService;

namespace telux {
namespace power {

//<clientType, clientName, machineName>
typedef std::tuple<int, std::string, std::string> pwrClientConfig;

class PowerGrpcTcuActivityListener {
  public:
    virtual void onTcuStateUpdate(TcuActivityState state, std::string machName) {}
    virtual void onSlaveAckStatusUpdate(std::vector<std::string> nackList,
        std::vector<std::string> noackList, std::string machName) {}
    virtual void onMachineUpdate(MachineEvent state) {}
    virtual ~PowerGrpcTcuActivityListener() {}
};

class PowerGrpcClient : public IEventListener,
                        public std::enable_shared_from_this<PowerGrpcClient> {
  public:
    PowerGrpcClient(int clientType, std::string clientName, std::string machineName);
    bool isReady();
    std::future<bool> onReady();
    telux::common::Status registerListener(std::weak_ptr<PowerGrpcTcuActivityListener> listener);
    telux::common::Status deregisterListener(std::weak_ptr<PowerGrpcTcuActivityListener> listener);
    telux::common::Status registerTcuStateEvents(TcuActivityState &initialState);
    telux::common::Status sendActivityStateCommand(TcuActivityState state, std::string machineName,
        telux::common::ResponseCallback &callback);
    telux::common::Status sendActivityStateAck(StateChangeResponse ack, TcuActivityState state);
    telux::common::Status setModemActivityState(TcuActivityState state);
    void onEventUpdate(google::protobuf::Any event) override;
    ~PowerGrpcClient();

  private:
    bool waitForInitialization();
    void getAvailableListeners(
        std::vector<std::shared_ptr<PowerGrpcTcuActivityListener>> &listeners);
    void handleTcuStateUpdateEvent(::powerStub::TcuStateUpdateEvent tcuStateUpdateEvent);
    void handleConsolidatedAcksEvent(::powerStub::ConsolidatedAcksEvent consolidatedAcksEvent);
    void handleMachineUpdateEvent(::powerStub::MachineUpdateEvent machineUpdateEvent);

    std::unique_ptr<::powerStub::PowerManagerService::Stub> stub_;
    std::weak_ptr<telux::power::PowerGrpcClient> myself_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    pwrClientConfig clientConfig_;
    telux::power::TcuActivityState state_;
    std::mutex grpcClientMutex_;
    std::shared_ptr<telux::common::ListenerManager<PowerGrpcTcuActivityListener>> listenerMgr_ ;
    telux::common::ServiceStatus serviceReady_;
};

}  // end namespace power
}  // end namespace telux

#endif  // POWERGRPCCLIENT_HPP
