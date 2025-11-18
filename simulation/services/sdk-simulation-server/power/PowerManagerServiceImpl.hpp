/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef POWER_MANAGER_SERVICE_HPP
#define POWER_MANAGER_SERVICE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <mutex>
#include <condition_variable>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <telux/common/CommonDefines.hpp>
#include <telux/power/TcuActivityDefines.hpp>
#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#include "protos/proto-src/power_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using powerStub::PowerManagerService;

struct ClientInfo {
    telux::power::ClientType clientType_;
    std::string clientName_;
    std::string machineName_;
};

class PowerManagerServiceImpl final : public powerStub::PowerManagerService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<PowerManagerServiceImpl> {
 public:
    PowerManagerServiceImpl();
    ~PowerManagerServiceImpl();
    grpc::Status InitService(ServerContext* context, const powerStub::PowerClientConnect* request,
        powerStub::GetServiceStatusReply* response);
    grpc::Status RegisterTcuStateEvent(ServerContext* context,
        const powerStub::MachineTcuState* request, powerStub::TcuStateEventReply* response);
    grpc::Status SendActivityState(ServerContext* context,
        const powerStub::SetActivityState* request, powerStub::PowerManagerCommandReply* response);
    grpc::Status SendActivityStateAck(ServerContext* context, const powerStub::SlaveAck* request,
        google::protobuf::Empty* response);
    grpc::Status SendModemActivityState(ServerContext* context,
        const powerStub::SetActivityState* request, powerStub::PowerManagerCommandReply* response);
    grpc::Status DeregisterFromServer(ServerContext* context,
        const powerStub::PowerClientConnect* request, google::protobuf::Empty* response);

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    void apiJsonReader(std::string apiName, powerStub::PowerManagerCommandReply* response);
    void convertToGrpcState(telux::power::TcuActivityState currState, ::powerStub::TcuState &state);
    void onEventUpdate(std::string event);
    void handleEvent(std::string token , std::string event);
    void initiateSuspend(telux::power::TcuActivityState state,
        ::powerStub::MachineName machineName);
    void notifySlavesOnStateUpdate(::powerStub::TcuState powerState,
        ::powerStub::MachineName machineName);
    void notifyMasterOnSlaveAck(::powerStub::MachineName machineName);
    void doResume(::powerStub::MachineName machineName);
    void handleMachineUpdateEvent(std::string event);
    void triggerMachineUpdateEvent(::powerStub::MachineState machineState);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    //Maintain master info and slaves list.
    ClientInfo master_;
    std::vector<ClientInfo> slaves_;
    //List of clients with the corresponding acknowledgements.
    std::vector<std::string> ackClients_;
    std::vector<std::string> nackClients_;
    std::vector<std::string> noackClients_;
    /**
     * ackMutex is to protect the critical section between
     * the suspend thread and incoming slave ack threads.
     */
    std::mutex ackMutex_;
    bool considerAck_ = true;
    /**
     * susMutex_ or suspend mutex is used to protect the critical section between
     * the suspend thread and the resume thread during the suspend timeout.
    */
    std::mutex susMutex_;
    std::condition_variable cv_;
    bool withinSuspendTimeout_ = false;
    bool resumeReceivedWithinTimeout_ = false;
    //Current states of local and all machines.
    telux::power::TcuActivityState localMachState_ = telux::power::TcuActivityState::RESUME;
    telux::power::TcuActivityState allMachState_ = telux::power::TcuActivityState::RESUME;
};

#endif // POWER_MANAGER_SERVICE_HPP