/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "PowerManagerServiceImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"
#include <thread>
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/EventService.hpp"

#define POWER_API_JSON "api/power/ITcuActivityManager.json"
#define DEFAULT_DELIMITER " "

PowerManagerServiceImpl::PowerManagerServiceImpl() {
    LOG(DEBUG, __FUNCTION__);
}

PowerManagerServiceImpl::~PowerManagerServiceImpl() {
    LOG(DEBUG, __FUNCTION__, " Destructing");
}

grpc::Status PowerManagerServiceImpl::InitService(ServerContext* context,
    const powerStub::PowerClientConnect* request, powerStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;

    telux::power::ClientType clientType =
        static_cast<telux::power::ClientType>(request->clienttype());
    std::string clientName = request->clientname();
    ::powerStub::MachineName machineName;
    std::string mach_name = request->machinename();
    if((mach_name == "ALL_MACHINES")) {
        machineName = ::powerStub::MachineName::MACH_ALL;
    } else if ((mach_name == "LOCAL_MACHINE") || (mach_name == "PVM")) {
        machineName = ::powerStub::MachineName::MACH_LOCAL;
    } else {
        LOG(ERROR, __FUNCTION__, " Unsupported Machine");
        response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
        return grpc::Status::OK;
    }

    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, POWER_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ITcuActivityManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ITcuActivityManager"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
        //Cache incoming Master/Slave client.
        if(clientType == telux::power::ClientType::MASTER) {
            if(master_.clientName_.empty()) {
                master_.clientType_ = clientType;
                master_.clientName_ = clientName;
                master_.machineName_ = telux::power::LOCAL_MACHINE;
                LOG(ERROR, __FUNCTION__, " Adding Master client- ", master_.clientName_);
            } else {
                LOG(ERROR, __FUNCTION__, " Master already present- ", master_.clientName_);
                serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
            }
        } else {
            ClientInfo slaveInfo {};
            slaveInfo.clientType_ = clientType;
            slaveInfo.clientName_ = clientName;
            if(machineName == ::powerStub::MachineName::MACH_LOCAL) {
                slaveInfo.machineName_ = telux::power::LOCAL_MACHINE;
            } else {
                slaveInfo.machineName_ = telux::power::ALL_MACHINES;
            }

            slaves_.push_back(slaveInfo);
            LOG(ERROR, __FUNCTION__, " Adding Slave client- ", slaveInfo.clientName_);
        }
    } else {
        LOG(ERROR, "Unable to read PowerManager JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters =
            {"PWR_ALL_SLAVE_UPDATE", "PWR_LOC_SLAVE_UPDATE", "PWR_MASTER_UPDATE", "power_mgr"};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
        taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    }
    response->set_delay(cbDelay);
    return grpc::Status::OK;
}

grpc::Status PowerManagerServiceImpl::DeregisterFromServer(ServerContext* context,
    const powerStub::PowerClientConnect* request, google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::power::ClientType clientType =
        static_cast<telux::power::ClientType>(request->clienttype());
    std::string clientName = request->clientname();
    if(clientType == telux::power::ClientType::MASTER) {
        //Resetting if master exits.
        LOG(DEBUG, __FUNCTION__, " Deregistering Master");
        master_.clientType_ = clientType;
        master_.clientName_ = "";
        master_.machineName_ = "";
    } else {
        for(size_t slave = 0; slave < slaves_.size(); slave++) {
            if(slaves_[slave].clientName_ == clientName) {
                LOG(DEBUG, __FUNCTION__, " Deregistering slave");
                slaves_.erase(slaves_.begin() + slave);
                break;
            }
        }
    }
    return grpc::Status::OK;
}

void PowerManagerServiceImpl::convertToGrpcState(telux::power::TcuActivityState currState,
    ::powerStub::TcuState &state) {
    if(currState == telux::power::TcuActivityState::RESUME) {
        state = ::powerStub::TcuState::STATE_RESUME;
    } else if(currState == telux::power::TcuActivityState::SUSPEND) {
        state = ::powerStub::TcuState::STATE_SUSPEND;
    } else if(currState == telux::power::TcuActivityState::SHUTDOWN) {
        state = ::powerStub::TcuState::STATE_SHUTDOWN;
    } else if(currState == telux::power::TcuActivityState::UNKNOWN) {
        state = ::powerStub::TcuState::STATE_UNKNOWN;
    }
}

grpc::Status PowerManagerServiceImpl::RegisterTcuStateEvent(ServerContext* context,
    const powerStub::MachineTcuState* request, powerStub::TcuStateEventReply* response) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::MachineName machineName = request->mach_name();
    ::powerStub::TcuState state;
    if(machineName == ::powerStub::MachineName::MACH_LOCAL) {
        convertToGrpcState(localMachState_, state);
    } else {
        convertToGrpcState(allMachState_, state);
    }
    response->set_initialstate(state);
    return grpc::Status::OK;
}

grpc::Status PowerManagerServiceImpl::SendActivityState(ServerContext* context,
    const powerStub::SetActivityState* request, powerStub::PowerManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::TcuState tcuState = request->powerstate();
    ::powerStub::MachineName machineName = request->mach_name();
    apiJsonReader("setActivityState", response);
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
    if(state == telux::power::TcuActivityState::UNKNOWN) {
        ::commonStub::ErrorCode errorCode = ::commonStub::ErrorCode::REQUEST_NOT_SUPPORTED;
        response->set_error(errorCode);
    } else if((machineName == ::powerStub::MachineName::MACH_LOCAL && state == localMachState_) ||
        (machineName == ::powerStub::MachineName::MACH_ALL && state == allMachState_)) {
        /**
         * RESUME can come in 2 cases-
         * 1. To resume from the exisiting suspend/shutdown state.
         * 2. To prevent suspend/shutdown when a nack/noack is received.
        */
        if(state == telux::power::TcuActivityState::RESUME) {
            /**
             * Suppose ONLY local machine is suspended. If master sends RESUME on all machines,
             * local machine should be resumed.
            */
            if(localMachState_ != telux::power::TcuActivityState::RESUME) {
                doResume(machineName);
                ::commonStub::ErrorCode errorCode = ::commonStub::ErrorCode::ERROR_CODE_SUCCESS;
                response->set_error(errorCode);
                return grpc::Status::OK;
            }

            //If the resume has come within the suspend timeout, we perform resume operation.
            std::unique_lock<std::mutex> lock(susMutex_);
            //withinSuspendTimeout_ is set to TRUE/FALSE by the suspend thread.
            if(withinSuspendTimeout_) {
                resumeReceivedWithinTimeout_ = true;
                doResume(machineName);
                ::commonStub::ErrorCode errorCode = ::commonStub::ErrorCode::ERROR_CODE_SUCCESS;
                response->set_error(errorCode);
                return grpc::Status::OK;
            }
            cv_.notify_all();
        }
        ::commonStub::ErrorCode errorCode = ::commonStub::ErrorCode::INCOMPATIBLE_STATE;
        response->set_error(errorCode);
    } else {
        if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
            if(state != telux::power::TcuActivityState::RESUME) {
                //Resetting the Ack state for next cycle.
                considerAck_ = true;
                //Clear the stale lists from the previous cycle.
                ackClients_.clear();
                nackClients_.clear();
                noackClients_.clear();
                notifySlavesOnStateUpdate(tcuState, machineName);
                auto f = std::async(std::launch::deferred,
                    [=]() {
                            this->initiateSuspend(state, machineName);
                    }).share();
                taskQ_->add(f);
            } else {
                auto f = std::async(std::launch::deferred,
                    [=]() {
                            this->doResume(machineName);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return grpc::Status::OK;
}
/**
 * This suspend thread waits for a timeout t to receive ACKs/NACKS from all clients.
 * Post receiving all the ACKs, the nack and noack lists are sent to the master.
 * The thread waits for a second timeout t2 ONLY IF there's a nack or noack from any slave.
 * If within t2 the master sends a resume, suspend/shutdown halts else system goes
 * to suspend/shutdown state.
*/
void PowerManagerServiceImpl::initiateSuspend(telux::power::TcuActivityState state,
    ::powerStub::MachineName machineName) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    {
        //Critical section
        std::unique_lock<std::mutex> lock(ackMutex_);
        /**
         * When this variable is set to true, incoming slave acks within the
         * timeout will be considered. Post the timeout, all incoming acks will be rejected
         * by setting this variable as false.
         */
        considerAck_ = false;
        //Populate No-ack list.
        for(auto slave: slaves_) {
            if(std::find(ackClients_.begin(), ackClients_.end(), slave.clientName_)
                == ackClients_.end()) {
                if(std::find(nackClients_.begin(), nackClients_.end(), slave.clientName_)
                    == nackClients_.end()) {
                    /**
                     * In case trigger comes for LOCAL machines,
                     * we don't expect ack from slaves registered for all machines.
                     */
                    if((machineName == ::powerStub::MachineName::MACH_LOCAL) &&
                        (slave.machineName_ == telux::power::ALL_MACHINES)) {
                        continue;
                    }
                    noackClients_.push_back(slave.clientName_);
                }
            }
        }
        notifyMasterOnSlaveAck(machineName);
        /**
         * ONLY if nack/noack list is non-empty wait for RESUME within
         * timeout t2 and then perform suspend.
         */
        if(!noackClients_.empty() || !nackClients_.empty()) {
            /**
             * If this variable is set to true and resume is received then
             * ongoing suspend will be halted.
             * This thread waits until timeout t2 to check if RESUME is received.
             */
            withinSuspendTimeout_ = true;
            std::unique_lock<std::mutex> lock(susMutex_);
            cv_.wait_until(lock,
                std::chrono::system_clock::now() + std::chrono::milliseconds(10000), [this] {
                    return resumeReceivedWithinTimeout_; // Check if resume is received.
            });
            /**
             * If resume is received within the timeout, suspend/shutdown is halted and
             * system stays in resume. This variable is set by the resume thread.
             */
            if(resumeReceivedWithinTimeout_) {
                LOG(DEBUG, __FUNCTION__, " Resume received, halting suspend/shutdown.");
                //Reset for next suspend/shutdown.
                resumeReceivedWithinTimeout_ = false;
                withinSuspendTimeout_ = false;
                return;
            } else {
                //Timeout t2 completed. Resume is not received. So only resetting suspend timeout.
                withinSuspendTimeout_ = false;
            }
        }
        /**
         * If the local state is set to SUSPEND, the initial state of any new slave
         * registering with ALL_MACHINES should be RESUME and local machine will be SUSPEND.
         * However, if the all machine state is set to SUSPEND,
         * any new slave's initial state will be SUSPEND regardless of the machine type.
        */
        localMachState_ = state;
        if(machineName == ::powerStub::MachineName::MACH_ALL) {
            allMachState_ = state;
        }
        //TBD may be replaced with actual logic to suspend host machine.
    }
}

void PowerManagerServiceImpl::doResume(::powerStub::MachineName machineName) {
    LOG(DEBUG, __FUNCTION__);
    notifySlavesOnStateUpdate(::powerStub::TcuState::STATE_RESUME, machineName);
    localMachState_ = telux::power::TcuActivityState::RESUME;
    if(machineName == ::powerStub::MachineName::MACH_ALL) {
        allMachState_ = telux::power::TcuActivityState::RESUME;
    }
    //TBD may be replaced with actual logic to resume host machine.
}

grpc::Status PowerManagerServiceImpl::SendActivityStateAck(ServerContext* context,
    const powerStub::SlaveAck* request, google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string clientName = request->clientname();
    ::powerStub::AckType ackType = request->ack_type();
    {
        std::unique_lock<std::mutex> lock(ackMutex_);
        //If Ack comes after timeout of 2 seconds, it will not be considered.
        if(considerAck_) {
            if(ackType == ::powerStub::AckType::ACK_SUSPEND) {
                LOG(DEBUG, __FUNCTION__, " Received ACK_SUSPEND from ", clientName);
                ackClients_.push_back(clientName);
            } else if(ackType == ::powerStub::AckType::ACK_SHUTDOWN) {
                LOG(DEBUG, __FUNCTION__, " Received ACK_SHUTDOWN from ", clientName);
                ackClients_.push_back(clientName);
            } else if(ackType == ::powerStub::AckType::NACK_SUSPEND) {
                LOG(DEBUG, __FUNCTION__, " Received NACK_SUSPEND from ", clientName);
                nackClients_.push_back(clientName);
            } else if(ackType == ::powerStub::AckType::NACK_SHUTDOWN) {
                LOG(DEBUG, __FUNCTION__, " Received NACK_SHUTDOWN from ", clientName);
                nackClients_.push_back(clientName);
            }
        }
    }
    return grpc::Status::OK;
}

grpc::Status PowerManagerServiceImpl::SendModemActivityState(ServerContext* context,
    const powerStub::SetActivityState* request, powerStub::PowerManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("setModemActivityState", response);
    return grpc::Status::OK;
}

void PowerManagerServiceImpl::apiJsonReader(
    std::string apiName, powerStub::PowerManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, POWER_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ITcuActivityManager", apiName, status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}

void PowerManagerServiceImpl::onEventUpdate(::eventService::UnsolicitedEvent event){
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == "power_mgr") {
        onEventUpdate(event.event());
    }
}

void PowerManagerServiceImpl::onEventUpdate(std::string event){
    LOG(DEBUG, __FUNCTION__,event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token,event);

}

void PowerManagerServiceImpl::handleEvent(std::string token, std::string event){
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", event);
    if(token == "machine_availability") {
        handleMachineUpdateEvent(event);
    }
}

void PowerManagerServiceImpl::handleMachineUpdateEvent(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::string availability;
    ::powerStub::MachineState machineState;
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " machine availability is not passed");
    } else {
        try {
            availability = token;
        } catch (std::exception& ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    if(availability == "UNAVAILABLE") {
        machineState = ::powerStub::MachineState::MACH_UNAVAILABLE;
    } else {
        machineState = ::powerStub::MachineState::MACH_AVAILABLE;
    }
    auto f = std::async(std::launch::async, [=](){
        this->triggerMachineUpdateEvent(machineState);
    }).share();
    taskQ_->add(f);
}

void PowerManagerServiceImpl::triggerMachineUpdateEvent(::powerStub::MachineState machineState) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::MachineUpdateEvent machineUpdateEvent;
    ::eventService::EventResponse anyResponse;
    machineUpdateEvent.set_mach_state(machineState);
    anyResponse.set_filter("power_mgr");
    anyResponse.mutable_any()->PackFrom(machineUpdateEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void PowerManagerServiceImpl::notifySlavesOnStateUpdate(::powerStub::TcuState powerState,
    ::powerStub::MachineName machineName) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::TcuStateUpdateEvent tcuStateUpdateEvent;
    ::eventService::EventResponse anyResponse;
    tcuStateUpdateEvent.set_power_state(powerState);
    tcuStateUpdateEvent.set_mach_name(machineName);

    //Sending to slaves registered for Local machines
    anyResponse.set_filter("PWR_LOC_SLAVE_UPDATE");
    anyResponse.mutable_any()->PackFrom(tcuStateUpdateEvent);
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);

    /**
     * If there's a state change for all machines, slaves registered for local
     * as well as all machines get the update.
     */
    if(machineName == ::powerStub::MachineName::MACH_ALL) {
        //Sending to slaves registered for ALL machines
        anyResponse.set_filter("PWR_ALL_SLAVE_UPDATE");
        anyResponse.mutable_any()->PackFrom(tcuStateUpdateEvent);
        eventImpl.updateEventQueue(anyResponse);
    }
}

void PowerManagerServiceImpl::notifyMasterOnSlaveAck(::powerStub::MachineName machineName) {
    LOG(DEBUG, __FUNCTION__);
    ::powerStub::ConsolidatedAcksEvent consolidatedAcksEvent;
    ::eventService::EventResponse anyResponse;
    consolidatedAcksEvent.set_mach_name(machineName);
    for(size_t i = 0; i < nackClients_.size(); i++) {
        consolidatedAcksEvent.add_nack_client_list(nackClients_[i]);
    }
    for(size_t i = 0; i < noackClients_.size(); i++) {
        consolidatedAcksEvent.add_noack_client_list(noackClients_[i]);
    }
    anyResponse.set_filter("PWR_MASTER_UPDATE");
    anyResponse.mutable_any()->PackFrom(consolidatedAcksEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}
