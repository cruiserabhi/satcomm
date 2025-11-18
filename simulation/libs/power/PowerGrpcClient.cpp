/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "PowerGrpcClient.hpp"

#include "common/CommonUtils.hpp"

#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1
#define RPC_FAIL_SUFFIX " RPC Request failed - "

using namespace telux::common;

namespace telux {
namespace power {

PowerGrpcClient::PowerGrpcClient(int clientType, std::string clientName, std::string machineName) {
    LOG(DEBUG, __FUNCTION__);
    clientConfig_ = std::make_tuple(clientType, clientName, machineName);
    serviceReady_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    stub_ = CommonUtils::getGrpcStub<PowerManagerService>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<PowerGrpcTcuActivityListener>>();
}

PowerGrpcClient::~PowerGrpcClient() {
    LOG(DEBUG, __FUNCTION__);
    //Deregister streams.
    if(static_cast<telux::power::ClientType>(std::get<0>(clientConfig_)) ==
        telux::power::ClientType::SLAVE) {
        std::string machName = std::get<2>(clientConfig_);
        if(machName == telux::power::ALL_MACHINES) {
            std::vector<std::string> filters = {"PWR_ALL_SLAVE_UPDATE", "power_mgr"};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(myself_, filters);
        } else {
            std::vector<std::string> filters = {"PWR_LOC_SLAVE_UPDATE", "power_mgr"};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(myself_, filters);
        }
    } else {
        std::vector<std::string> filters = {"PWR_MASTER_UPDATE", "power_mgr"};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.deregisterListener(myself_, filters);
    }

    //Deregister from server.
    ::powerStub::PowerClientConnect request {};
    ::google::protobuf::Empty response {};
    ClientContext context{};
    request.set_clienttype(std::get<0>(clientConfig_));
    request.set_clientname(std::get<1>(clientConfig_));
    grpc::Status reqStatus = stub_->DeregisterFromServer(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Deregister From Server failed");
    }
}

bool PowerGrpcClient::isReady() {
    LOG(DEBUG, __FUNCTION__);
    return (serviceReady_ == common::ServiceStatus::SERVICE_AVAILABLE);
}

bool PowerGrpcClient::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> cvLock(grpcClientMutex_);
    //Send init to GRPC server with clientConfig_
    ::powerStub::PowerClientConnect request {};
    ::powerStub::GetServiceStatusReply response {};
    ClientContext context{};
    int cbDelay;
    grpc::Status reqStatus;
    request.set_clienttype(std::get<0>(clientConfig_));
    request.set_clientname(std::get<1>(clientConfig_));
    std::string machName = std::get<2>(clientConfig_);
    request.set_machinename(machName);

    reqStatus = stub_->InitService(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " InitService request failed");
        return false;
    }

    serviceReady_ = static_cast<telux::common::ServiceStatus>(response.service_status());
    cbDelay = static_cast<int>(response.delay());
    LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(serviceReady_));
    if(serviceReady_ == ServiceStatus::SERVICE_AVAILABLE ){
        auto myself = shared_from_this();
        myself_ = myself;
        if(static_cast<telux::power::ClientType>(std::get<0>(clientConfig_)) ==
            telux::power::ClientType::SLAVE) {
            /**
             * A slave client depending upon the machine it is interested in
             * registers with the Power manager service with the specific stream.
             * power_mgr stream is a filter for general notifications like onMachineUpdate event.
             */
            if(machName == telux::power::ALL_MACHINES) {
                std::vector<std::string> filters = {"PWR_ALL_SLAVE_UPDATE", "power_mgr"};
                auto &clientEventManager = telux::common::ClientEventManager::getInstance();
                clientEventManager.registerListener(myself_, filters);
            } else {
                std::vector<std::string> filters = {"PWR_LOC_SLAVE_UPDATE", "power_mgr"};
                auto &clientEventManager = telux::common::ClientEventManager::getInstance();
                clientEventManager.registerListener(myself_, filters);
            }
        } else {
            //A master client registers with the power manager service with a Master stream.
            std::vector<std::string> filters = {"PWR_MASTER_UPDATE", "power_mgr"};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(myself_, filters);
        }
    }
    if (cbDelay != SKIP_CALLBACK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::",
            static_cast<int>(serviceReady_));
    }

    return (serviceReady_ == common::ServiceStatus::SERVICE_AVAILABLE);
}

std::future<bool> PowerGrpcClient::onReady() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return f;
}

telux::common::Status PowerGrpcClient::registerListener(
    std::weak_ptr<PowerGrpcTcuActivityListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
    } else {
        LOG(ERROR, __FUNCTION__, " ListenerManager is null");
    }
    return status;
}

telux::common::Status PowerGrpcClient::deregisterListener(
    std::weak_ptr<PowerGrpcTcuActivityListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->deRegisterListener(listener);
    } else {
        LOG(ERROR, __FUNCTION__, " ListenerManager is null");
    }
    return status;
}

void PowerGrpcClient::getAvailableListeners(
    std::vector<std::shared_ptr<PowerGrpcTcuActivityListener>> &listeners) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<PowerGrpcTcuActivityListener>> regListeners;
        listenerMgr_->getAvailableListeners(regListeners);
        for (auto &wp : regListeners) {
            listeners.emplace_back(wp);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " ListenerManager is null");
    }
}

telux::common::Status PowerGrpcClient::registerTcuStateEvents(TcuActivityState &initialState) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::MachineTcuState request {};
    ::powerStub::TcuStateEventReply response {};
    ClientContext context{};

    std::string machName = std::get<2>(clientConfig_);
    if(machName == telux::power::ALL_MACHINES) {
        request.set_mach_name(::powerStub::MachineName::MACH_ALL);
    } else {
        request.set_mach_name(::powerStub::MachineName::MACH_LOCAL);
    }

    grpc::Status reqStatus;
    reqStatus = stub_->RegisterTcuStateEvent(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
        return telux::common::Status::FAILED;
    }

    ::powerStub::TcuState state = response.initialstate();
    if(state == ::powerStub::TcuState::STATE_RESUME) {
        initialState = telux::power::TcuActivityState::RESUME;
    } else if(state == ::powerStub::TcuState::STATE_SUSPEND) {
        initialState = telux::power::TcuActivityState::SUSPEND;
    } else if(state == ::powerStub::TcuState::STATE_SHUTDOWN) {
        initialState = telux::power::TcuActivityState::SHUTDOWN;
    } else if(state == ::powerStub::TcuState::STATE_UNKNOWN) {
        initialState = telux::power::TcuActivityState::UNKNOWN;
    }
    //state_ determines the current state of the client.
    state_ = initialState;
    return telux::common::Status::SUCCESS;
}

telux::common::Status PowerGrpcClient::sendActivityStateCommand(TcuActivityState state,
    std::string machineName, telux::common::ResponseCallback &callback) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::SetActivityState request {};
    ::powerStub::PowerManagerCommandReply response {};
    ClientContext context{};
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::powerStub::TcuState powerState;
    if(state == telux::power::TcuActivityState::RESUME) {
        powerState = ::powerStub::TcuState::STATE_RESUME;
    } else if(state == telux::power::TcuActivityState::SUSPEND) {
        powerState = ::powerStub::TcuState::STATE_SUSPEND;
    } else if(state == telux::power::TcuActivityState::SHUTDOWN) {
        powerState = ::powerStub::TcuState::STATE_SHUTDOWN;
    } else if(state == telux::power::TcuActivityState::UNKNOWN) {
        powerState = ::powerStub::TcuState::STATE_UNKNOWN;
    }
    request.set_powerstate(powerState);
    ::powerStub::MachineName machine_Name;
    if(machineName == telux::power::ALL_MACHINES) {
        machine_Name = ::powerStub::MachineName::MACH_ALL;
    } else {
        machine_Name = ::powerStub::MachineName::MACH_LOCAL;
    }
    request.set_mach_name(machine_Name);
    grpc::Status reqStatus;
    reqStatus = stub_->SendActivityState(&context, request, &response);
    if(reqStatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        if (callback && (cbDelay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }).share();
            taskQ_.add(f);
        }
    }
    return status;
}

telux::common::Status PowerGrpcClient::sendActivityStateAck(
    StateChangeResponse ack, TcuActivityState state) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::SlaveAck request {};
    ::google::protobuf::Empty response {};
    ClientContext context{};
    switch (ack) {
        case StateChangeResponse::ACK:
            if (state == TcuActivityState::SUSPEND) {
                request.set_ack_type(::powerStub::AckType::ACK_SUSPEND);
            } else if (state == TcuActivityState::SHUTDOWN) {
                request.set_ack_type(::powerStub::AckType::ACK_SHUTDOWN);
            } else {
                LOG(ERROR, __FUNCTION__, "Invalid TcuActivityState provided for conversion");
                return telux::common::Status::INVALIDPARAM;
            }
            break;
        case StateChangeResponse::NACK:
            if (state == TcuActivityState::SUSPEND) {
                request.set_ack_type(::powerStub::AckType::NACK_SUSPEND);
            } else if (state == TcuActivityState::SHUTDOWN) {
                request.set_ack_type(::powerStub::AckType::NACK_SHUTDOWN);
            } else {
                LOG(ERROR, __FUNCTION__, "Invalid TcuActivityState provided for conversion");
                return telux::common::Status::INVALIDPARAM;
            }
            break;
    }
    request.set_clientname(std::get<1>(clientConfig_));
    grpc::Status reqStatus;
    reqStatus = stub_->SendActivityStateAck(&context, request, &response);
    if(!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status PowerGrpcClient::setModemActivityState(TcuActivityState state) {
    LOG(DEBUG, __FUNCTION__);

    ::powerStub::SetActivityState request {};
    ::powerStub::PowerManagerCommandReply response {};
    ClientContext context{};
    telux::common::Status status = telux::common::Status::FAILED;
    ::powerStub::TcuState powerState;
    if(state == telux::power::TcuActivityState::RESUME) {
        powerState = ::powerStub::TcuState::STATE_RESUME;
    } else if(state == telux::power::TcuActivityState::SUSPEND) {
        powerState = ::powerStub::TcuState::STATE_SUSPEND;
    } else if(state == telux::power::TcuActivityState::SHUTDOWN) {
        powerState = ::powerStub::TcuState::STATE_SHUTDOWN;
    } else if(state == telux::power::TcuActivityState::UNKNOWN) {
        powerState = ::powerStub::TcuState::STATE_UNKNOWN;
    }
    request.set_powerstate(powerState);
    ::powerStub::MachineName machine_Name = ::powerStub::MachineName::MACH_LOCAL;
    request.set_mach_name(machine_Name);

    grpc::Status reqStatus;
    reqStatus = stub_->SendModemActivityState(&context, request, &response);
    if(reqStatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
    }
    return status;
}

void PowerGrpcClient::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::powerStub::TcuStateUpdateEvent>()) {
        LOG(DEBUG, __FUNCTION__, " TCU State update");
        ::powerStub::TcuStateUpdateEvent tcuStateUpdateEvent;
        event.UnpackTo(&tcuStateUpdateEvent);
        handleTcuStateUpdateEvent(tcuStateUpdateEvent);
    } else if (event.Is<::powerStub::ConsolidatedAcksEvent>()) {
        LOG(DEBUG, __FUNCTION__, " Consolidated Acks Event");
        ::powerStub::ConsolidatedAcksEvent consolidatedAcksEvent;
        event.UnpackTo(&consolidatedAcksEvent);
        handleConsolidatedAcksEvent(consolidatedAcksEvent);
    } else if (event.Is<::powerStub::MachineUpdateEvent>()) {
        LOG(DEBUG, __FUNCTION__, " Machine update Event");
        ::powerStub::MachineUpdateEvent machineUpdateEvent;
        event.UnpackTo(&machineUpdateEvent);
        handleMachineUpdateEvent(machineUpdateEvent);
    }
}

void PowerGrpcClient::handleTcuStateUpdateEvent(
    ::powerStub::TcuStateUpdateEvent tcuStateUpdateEvent) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::TcuState tcuState = tcuStateUpdateEvent.power_state();
    telux::power::TcuActivityState state;
    if(tcuState == ::powerStub::TcuState::STATE_RESUME) {
        state = telux::power::TcuActivityState::RESUME;
    } else if(tcuState == ::powerStub::TcuState::STATE_SUSPEND) {
        state = telux::power::TcuActivityState::SUSPEND;
    } else if(tcuState == ::powerStub::TcuState::STATE_SHUTDOWN) {
        state = telux::power::TcuActivityState::SHUTDOWN;
    } else if(tcuState == ::powerStub::TcuState::STATE_UNKNOWN) {
        state = telux::power::TcuActivityState::UNKNOWN;
    }
    ::powerStub::MachineName machineName = tcuStateUpdateEvent.mach_name();
    std::string machName = "";
    if(machineName == ::powerStub::MachineName::MACH_LOCAL) {
        machName = telux::power::LOCAL_MACHINE;
    } else {
        machName = telux::power::ALL_MACHINES;
    }
    /**
     * If the incoming state is the same as the state of the client, the notification to the
     * SDK library is dropped.
    */
    if(state_ == state) {
        LOG(DEBUG, __FUNCTION__, " Dropping since state is same for ", machName);
        return;
    }
    //Client state gets updated.
    state_ = state;
    std::vector<std::shared_ptr<PowerGrpcTcuActivityListener>> applisteners;
    getAvailableListeners(applisteners);
    for(auto &listener : applisteners) {
        listener->onTcuStateUpdate(state, machName);
    }
}

void PowerGrpcClient::handleConsolidatedAcksEvent(
    ::powerStub::ConsolidatedAcksEvent consolidatedAcksEvent) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::MachineName machineName = consolidatedAcksEvent.mach_name();
    std::string machName = "";
    if(machineName == ::powerStub::MachineName::MACH_LOCAL) {
        machName = telux::power::LOCAL_MACHINE;
    } else {
        machName = telux::power::ALL_MACHINES;
    }
    std::vector<std::string> nackList;
    for (const std::string& str : consolidatedAcksEvent.nack_client_list()) {
        nackList.push_back(str);
    }
    std::vector<std::string> noackList;
    for (const std::string& str : consolidatedAcksEvent.noack_client_list()) {
        noackList.push_back(str);
    }
    LOG(DEBUG, __FUNCTION__, " Nacklist size- ", nackList.size(),
        " Noacklist size- ", noackList.size());
    std::vector<std::shared_ptr<PowerGrpcTcuActivityListener>> applisteners;
    getAvailableListeners(applisteners);
    for(auto &listener : applisteners) {
        listener->onSlaveAckStatusUpdate(nackList, noackList, machName);
    }
}

void PowerGrpcClient::handleMachineUpdateEvent(::powerStub::MachineUpdateEvent machineUpdateEvent) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::MachineState machineState = machineUpdateEvent.mach_state();
    telux::power::MachineEvent state;
    if(machineState == ::powerStub::MachineState::MACH_UNAVAILABLE) {
        state = telux::power::MachineEvent::UNAVAILABLE;
    } else if (machineState == ::powerStub::MachineState::MACH_AVAILABLE) {
        state = telux::power::MachineEvent::AVAILABLE;
    }
    std::vector<std::shared_ptr<PowerGrpcTcuActivityListener>> applisteners;
    getAvailableListeners(applisteners);
    for(auto &listener : applisteners) {
        listener->onMachineUpdate(state);
    }
}

}  // end namespace power
}  // end namespace telux