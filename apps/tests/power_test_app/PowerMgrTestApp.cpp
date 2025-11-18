/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a sample program to register and receive TCU-activity state updates, send commands to
 * change the TCU-activity state
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <csignal>
#include <mutex>
#include <condition_variable>

extern "C" {
#include "unistd.h"
}

#include "PowerMgrTestApp.hpp"
#include "../../common/utils/Utils.hpp"

static bool listenerEnabled = false;
static std::mutex mutex;
static std::condition_variable cv;

static void printTcuActivityState(TcuActivityState state) {

    if(state == TcuActivityState::SUSPEND) {
        PRINT_NOTIFICATION << " TCU-activity State : SUSPEND" << std::endl;
    } else if(state == TcuActivityState::RESUME) {
        PRINT_NOTIFICATION << " TCU-activity State : RESUME" << std::endl;
    } else if(state == TcuActivityState::SHUTDOWN) {
        PRINT_NOTIFICATION << " TCU-activity State : SHUTDOWN" << std::endl;
    } else if(state == TcuActivityState::UNKNOWN) {
        PRINT_NOTIFICATION << " TCU-activity State : UNKNOWN" << std::endl;
    } else {
        std::cout << APP_NAME << " ERROR: Invalid TCU-activity state notified" << std::endl;
    }
}

static void printHelp() {
std::cout << "-----------------------------------------------" << std::endl;
    std::cout << "./telux_power_test_app <-l> <-s> <-r> <-p> <-c> <-h>" << std::endl;
    std::cout << "Operations: " << std::endl;
    std::cout << "   -l : listen to TCU-activity state updates (as SLAVE)" << std::endl;
    std::cout << "   -s : send SUSPEND command (as MASTER)" << std::endl;
    std::cout << "   -r : send RESUME command (as MASTER)" << std::endl;
    std::cout << "   -p : send SHUT-DOWN command (as MASTER)" << std::endl << std::endl;

    std::cout << "Scope of the operation: " << std::endl;
    std::cout << "   -M : Name of the machine on which the operation is expected to be performed" <<
                 "        Applicable only to MASTER while sending SUSPEND/RESUME/SHUTDOWN \n" <<
                 "        commands. '-M <machine_name>' \n" <<
                 "        e.g. telux_power_test_app -s -M qcom,televm" << std::endl;
    std::cout << "   -L : carry out operation on LOCAL machine" << std::endl <<
                 "        e.g. telux_power_test_app -l -L " << std::endl;
    std::cout << "   -A : carry out operation on ALL machine" << std::endl <<
                 "        e.g. telux_power_test_app -l -A " << std::endl <<
                 "             telux_power_test_app -s -A " << std::endl << std::endl;

    std::cout << "Additional: " << std::endl;
    std::cout << "   -m : get LOCAL machine name" << std::endl;
    std::cout << "   -a : get list of all machine names " << std::endl;
    std::cout << "   -n : set client name (recommended mainly for SLAVE)" << std::endl <<
                 "        e.g. telux_power_test_app -l -A -n testApp_123 " << std::endl;
    std::cout << "   -c : open interactive console (as MASTER)" << std::endl;
    std::cout << "   -h : print the help menu" << std::endl;
}

PowerMgmtTestApp::PowerMgmtTestApp()
    : ConsoleApp("System Power-Management Menu", "power-mgmt> ")
    , tcuActivityMgr_(nullptr) {
}

PowerMgmtTestApp::~PowerMgmtTestApp() {
    tcuActivityMgr_ = nullptr;
}

void PowerMgmtTestApp::onTcuActivityStateUpdate(TcuActivityState tcuState,
    std::string machineName) {
    std::cout << " TCU Activity state changed for machine "
        << machineName << std::endl;
    printTcuActivityState(tcuState);
    if(tcuState == TcuActivityState::SUSPEND) {
        Status ackStatus = tcuActivityMgr_->sendActivityStateAck(StateChangeResponse::ACK,
            tcuState);
        if(ackStatus == Status::SUCCESS) {
            std::cout << APP_NAME << " Sent SUSPEND acknowledgement" << std::endl;
        } else {
            std::cout << APP_NAME << " Failed to send SUSPEND acknowledgement !" << std::endl;
        }
    } else if(tcuState == TcuActivityState::SHUTDOWN) {
        Status ackStatus = tcuActivityMgr_->sendActivityStateAck(StateChangeResponse::ACK,
            tcuState);
        if(ackStatus == Status::SUCCESS) {
            std::cout << APP_NAME << " Sent SHUTDOWN acknowledgement" << std::endl;
        } else {
            std::cout << APP_NAME << " Failed to send SHUTDOWN acknowledgement !" << std::endl;
        }
    }
}

void PowerMgmtTestApp::onMachineUpdate(const std::string machineName,
    const MachineEvent machineEvent) {
    if(machineEvent == MachineEvent::AVAILABLE) {
        std::cout << APP_NAME << " Machine: " << machineName << " Event : AVAILABLE" << std::endl;
    } else if(machineEvent == MachineEvent::UNAVAILABLE){
        std::cout << APP_NAME << " Machine: " << machineName << " Event : UNAVAILABLE" << std::endl;
    }
}

void PowerMgmtTestApp::onSlaveAckStatusUpdate(const telux::common::Status status,
        const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
        const std::vector<ClientInfo> nackResponseClients) {
    std::cout << " Consolidated acknowledgement status for machine: "<< machineName << std::endl;
    if(status == telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " Slave applications successfully acknowledged the state" <<
                                 " transition" << std::endl;
    } else if(status == telux::common::Status::EXPIRED) {
        std::cout << APP_NAME << " Timeout occurred while waiting for acknowledgements from slave"
                              << " applications" << std::endl;
    } else if(status == telux::common::Status::NOTREADY) {
        std::cout << APP_NAME << " Received NACK from slave applications" << std::endl;
    } else {
        std::cout << APP_NAME << " Failed to receive acknowledgements from slave applications"
                              << std::endl;
    }
    if(unresponsiveClients.size() > 0) {
        std::cout << " Number of unresponsive clients : "<< unresponsiveClients.size() << std::endl;
        for (size_t i = 0; i < unresponsiveClients.size(); i++) {
            std::cout << " client name : "<< unresponsiveClients[i].first
                      << " , machine name : "<< unresponsiveClients[i].second << std::endl;
        }
    }

    if(nackResponseClients.size() > 0) {
        std::cout << " Number of clients responded with NACK : "<< nackResponseClients.size()
            << std::endl;
        for (size_t i = 0; i < nackResponseClients.size(); i++) {
            std::cout << " client name : "<< nackResponseClients[i].first
                      << " , machine name : "<< nackResponseClients[i].second << std::endl;
        }
    }
    if(!listenerEnabled) {
        std::unique_lock<std::mutex> lock(mutex);// To make sure cv.notify happens after cv.wait
        cv.notify_all();
    }
}

void PowerMgmtTestApp::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << " Service Status : UNAVAILABLE" << std::endl;
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << " Service Status : AVAILABLE" << std::endl;
    }
}

static void signalHandler( int signum ) {
    std::unique_lock<std::mutex> lock(mutex);
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    cv.notify_all();
}

void PowerMgmtTestApp::sendActivityStateCommand(TcuActivityState state) {
    std::string machineName;
    if(!userInputMachineName(machineName)){
        std::cout << " Unable to get machine name, try again " << std::endl;
        return;
    }
    sendActivityStateCommandEx(machineName, state);
}

void PowerMgmtTestApp::sendActivityStateCommandEx( std::string machineName,
    TcuActivityState state) {
    if(state == TcuActivityState::SUSPEND) {
        std::cout << APP_NAME << " Sending SUSPEND command to " << machineName << std::endl;
    } else if(state == TcuActivityState::SHUTDOWN) {
        std::cout << APP_NAME << " Sending SHUTDOWN command to " << machineName << std::endl;
    } else if((state == TcuActivityState::RESUME)) {
        std::cout << APP_NAME << " Sending RESUME command to " << machineName << std::endl;
    }
    telux::common::Status status =
        tcuActivityMgr_->setActivityState(state, machineName, [&, state](ErrorCode errorCode){
            if(errorCode == telux::common::ErrorCode::SUCCESS) {
                std::cout << APP_NAME << " Command initiated successfully " << std::endl;
            } else {
                std::cout << APP_NAME << " Command failed !!!" <<
                    "  ErrorCode : " << Utils::getErrorCodeAsString(errorCode) << std::endl;
            }
            if(!listenerEnabled) {
                if((errorCode != telux::common::ErrorCode::SUCCESS)
                    || (state == TcuActivityState::RESUME)) {
                    // To make sure cv.notify happens after cv.wait
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.notify_all();
                }
            }
        }
    );
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to send TCU-activity state command" << std::endl;
    }
}

void PowerMgmtTestApp::getMachineName() {
    std::string machineName;
    telux::common::Status status = tcuActivityMgr_->getMachineName(machineName);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to get local machine name " << std::endl;
    } else {
        std::cout << APP_NAME << " Local machine name = " << machineName << std::endl;
    }
}


std::vector<std::string> PowerMgmtTestApp::getAllMachineNames() {
    std::vector<std::string> machineNames;
    telux::common::Status status = tcuActivityMgr_->getAllMachineNames(machineNames);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to get all machine names " << std::endl;
    } else {
        std::cout << APP_NAME << " Number of available machines " << machineNames.size()
            << std::endl;
        for (size_t i = 0; i < machineNames.size(); i++){
            std::cout << machineNames[i] << std::endl;
        }
    }
    return machineNames;
}

bool PowerMgmtTestApp::userInputMachineName(std::string &machineName) {
    char delimiter = '\n';
    std::string input = "";
    std::vector<std::string> machineNames;
    telux::common::Status status = tcuActivityMgr_->getAllMachineNames(machineNames);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to get all machine name " << std::endl;
    }
    std::cout << "Select machine from "<< machineNames.size()+2 << " available machines: "
        << std::endl;

    std::cout << "1 : LOCAL_MACHINE" << std::endl;
    std::cout << "2 : ALL_MACHINES" << std::endl;
    for (size_t i = 0; i < machineNames.size(); i++){
        std::cout << i+3 << " : " << machineNames[i] << std::endl;
    }

    std::getline(std::cin, input, delimiter);
    int opt = -1;
    if(!input.empty()) {
        try {
            opt = std::stoi(input);
        } catch(const std::exception &e) {
            std::cout << " ERROR: Invalid input, Enter numerical value " << opt << std::endl;
            return false;
        }
    } else {
        std::cout << " No input, try again " << std::endl;
        return false;
    }

    switch (opt) {
        case 1:
            machineName = LOCAL_MACHINE;
            break;
        case 2:
            machineName = ALL_MACHINES;
            break;
        default:
            if((opt - 3 < (int)machineNames.size()) && (opt - 3) >= 0) {
                machineName = machineNames[opt-3];
            } else {
                std::cout << " Invalid option, try again " << std::endl;
                return false;
            }
            break;
    }
    return true;
}

TcuActivityState PowerMgmtTestApp::getTcuActivityState() {
    TcuActivityState state = tcuActivityMgr_->getActivityState();
    printTcuActivityState(state);
    return state;
}

void PowerMgmtTestApp::setModemActivityState() {
    TcuActivityState state = TcuActivityState::UNKNOWN;
    char delimiter = '\n';
    std::string input;
    std::cout << "Select modem activity state(1-Suspend/2-Resume): ";
    std::getline(std::cin, input, delimiter);
    int opt = -1;
    if(!input.empty()) {
        try {
            opt = std::stoi(input);
        } catch(const std::exception &e) {
            std::cout << " ERROR: Invalid input, Enter numerical value " << opt << std::endl;
        }
    } else {
        std::cout << " No input, try again " << std::endl;
        return;
    }
    if(opt == 1) {
        state = TcuActivityState::SUSPEND;
    } else if(opt == 2) {
        state = TcuActivityState::RESUME;
    }
    telux::common::Status status = tcuActivityMgr_->setModemActivityState(state);
    if(status == telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " Modem activity state is set successfully" << std::endl;
    } else {
        std::cout << APP_NAME << " Failed to set Modem activity state" << std::endl;
    }
    return;
}

int PowerMgmtTestApp::start(ClientInstanceConfig config) {
    if(config.clientType == ClientType::MASTER) {
        std::cout << APP_NAME << " Initializing the client as a MASTER "
            << ", Machine name: " << config.machineName
            << ", Client name: " << config.clientName << std::endl;
    } else {
        std::cout << APP_NAME << " Initializing the client as a SLAVE "
            << ", Machine name: " << config.machineName
            << ", Client name: " << config.clientName << std::endl;
    }
    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();
    // Get TCU-activity manager object
    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
    tcuActivityMgr_ = powerFactory.getTcuActivityManager(config,
                        [&](telux::common::ServiceStatus status) {
                             prom.set_value(status);
                        });
    if(tcuActivityMgr_ == nullptr)
    {
        std::cout << APP_NAME << " ERROR - Failed to get manager instance" << std::endl;
        return -1;
    }
    // Wait for TCU-activity manager to be ready
    std::cout << " Waiting for TCU Activity Manager to be ready " << std::endl;
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << APP_NAME << " TCU-activity manager is ready" << std::endl;
    } else {
        std::cout << APP_NAME << " Failed to initialize TCU-activity manager" << std::endl;
        return -1;
    }
    getTcuActivityState();
    return 0;
}

void PowerMgmtTestApp::registerForUpdates() {
    // Registering a listener for TCU-activity state updates
    telux::common::Status status = tcuActivityMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to register for TCU-activity state updates"
                << std::endl;
    } else {
        std::cout << APP_NAME << " Registered Listener for TCU-activity state updates" << std::endl;
    }
    // Registering a listener for TCU-activity management service status updates
    status = tcuActivityMgr_->registerServiceStateListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to register for Service status updates"
                << std::endl;
    }
}

void PowerMgmtTestApp::deregisterForUpdates() {
    // De-registering a listener for TCU-activity state updates
    telux::common::Status status = tcuActivityMgr_->deregisterListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to de-register for TCU-activity state updates"
                << std::endl;
    } else {
        std::cout << APP_NAME << " De-registered listener" << std::endl;
    }
    // De-registering a listener for TCU-activity management service status updates
    status = tcuActivityMgr_->deregisterServiceStateListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to de-register for Service status updates"
                << std::endl;
    }
}

void PowerMgmtTestApp::consoleinit() {
   std::shared_ptr<ConsoleAppCommand> suspendSytemCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Suspend_System", {},
         std::bind(&PowerMgmtTestApp::sendActivityStateCommand, this, TcuActivityState::SUSPEND)));
   std::shared_ptr<ConsoleAppCommand> resumeSytemCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Resume_System", {},
         std::bind(&PowerMgmtTestApp::sendActivityStateCommand, this, TcuActivityState::RESUME)));
   std::shared_ptr<ConsoleAppCommand> shutdownSytemCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Shutdown_System", {},
         std::bind(&PowerMgmtTestApp::sendActivityStateCommand, this, TcuActivityState::SHUTDOWN)));
   std::shared_ptr<ConsoleAppCommand> getTcuStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Get_System_State", {},
         std::bind(&PowerMgmtTestApp::getTcuActivityState, this)));
   std::shared_ptr<ConsoleAppCommand> setModemActivityStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "5", "Set_Modem_Activity_State", {},
         std::bind(&PowerMgmtTestApp::setModemActivityState, this)));
    std::shared_ptr<ConsoleAppCommand> getMachineNameCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "Get_Local_Machine_Name", {},
         std::bind(&PowerMgmtTestApp::getMachineName, this)));
    std::shared_ptr<ConsoleAppCommand> getAllMachineNamesCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "7", "Get_All_Machine_Names", {},
         std::bind(&PowerMgmtTestApp::getAllMachineNames, this)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListPowerMenu
      = {suspendSytemCommand, resumeSytemCommand, shutdownSytemCommand, getTcuStateCommand,
         setModemActivityStateCommand, getMachineNameCommand, getAllMachineNamesCommand};
   ConsoleApp::addCommands(commandsListPowerMenu);
   ConsoleApp::displayMenu();
}

std::shared_ptr<PowerMgmtTestApp> init(ClientType clientType, std::string clientName,
    std::string machineName) {
    std::shared_ptr<PowerMgmtTestApp> powerMgmtTest = std::make_shared<PowerMgmtTestApp>();
    if (!powerMgmtTest) {
        std::cout << "Failed to instantiate PowerMgmtTestApp" << std::endl;
        return nullptr;
    }
    ClientInstanceConfig config;
    config.clientType = clientType;
    config.clientName = clientName;
    config.machineName =  machineName;
    if( 0 != powerMgmtTest->start(config)) {
        std::cout << APP_NAME << " Failed to initialize the TCU-activity management service"
            << std::endl;
        return nullptr;
    }
    return powerMgmtTest;
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {

    bool inputCommand = false;
    TcuActivityState state = TcuActivityState::UNKNOWN;
    ClientType clientType = ClientType::SLAVE;
    std::string clientName = "telux_power_test_app_"+std::to_string(getpid());
    std::string machineName = ALL_MACHINES;
    bool isGetAllMachineNames = false;
    bool isGetMachineName = false;

    if(argc <= 1) {
        printHelp();
        return -1;
    }
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-l") {
            listenerEnabled = true;
        } else if (std::string(argv[i]) == "-n") {
            ++i;
            if(i >= argc) {
                std::cout << APP_NAME << " Please provide client name" << std::endl;
                return -1;
            }
            clientName = std::string(argv[i]);
        } else  if (std::string(argv[i]) == "-s") {
            clientType = ClientType::MASTER;
            inputCommand=true;
            state=TcuActivityState::SUSPEND;
        } else if (std::string(argv[i]) == "-r") {
            clientType = ClientType::MASTER;
            inputCommand=true;
            state=TcuActivityState::RESUME;
        } else if (std::string(argv[i]) == "-p") {
            clientType = ClientType::MASTER;
            inputCommand=true;
            state=TcuActivityState::SHUTDOWN;
        } else if (std::string(argv[i]) == "-M") {
            ++i;
            if(i >= argc) {
                std::cout << APP_NAME << " Please provide machine name" << std::endl;
                return -1;
            }
            machineName = std::string(argv[i]);
        } else if (std::string(argv[i]) == "-L") {
            machineName = LOCAL_MACHINE;
        } else if (std::string(argv[i]) == "-m") {
            isGetMachineName = true;
        } else if (std::string(argv[i]) == "-A") {
            machineName = ALL_MACHINES;
        } else if (std::string(argv[i]) == "-a") {
            isGetAllMachineNames = true;
        } else if (std::string(argv[i]) == "-c") {
            clientType = ClientType::MASTER;
            std::shared_ptr<PowerMgmtTestApp> myPowerMgmtTest =
                init(clientType, clientName, machineName);
            if(myPowerMgmtTest == nullptr) {
                std::cout << "Exiting application..." << std::endl;
                return 0;
            }
            myPowerMgmtTest->registerForUpdates();
            listenerEnabled = true;
            myPowerMgmtTest->consoleinit();
            myPowerMgmtTest->mainLoop();
            myPowerMgmtTest->deregisterForUpdates();
            return 0;
        } else {
            printHelp();
            return -1;
        }
    }
    std::shared_ptr<PowerMgmtTestApp> myPowerMgmtTest = init(clientType, clientName, machineName);
    if(myPowerMgmtTest == nullptr) {
        std::cout << "Exiting application..." << std::endl;
        return 0;
    }
    if(listenerEnabled) {
        myPowerMgmtTest->registerForUpdates();
    }
    signal(SIGINT, signalHandler);
    std::unique_lock<std::mutex> lock(mutex);
    if(inputCommand) {
        myPowerMgmtTest->registerForUpdates();
        myPowerMgmtTest->sendActivityStateCommandEx(machineName, state);
    }
    if(isGetAllMachineNames) {
        myPowerMgmtTest->getAllMachineNames();
        return 0;
    }
    if(isGetMachineName) {
        myPowerMgmtTest->getMachineName();
        return 0;
    }

    std::cout << APP_NAME << " Press CTRL+C to exit" << std::endl;
    cv.wait(lock);
    if(listenerEnabled || inputCommand) {
        myPowerMgmtTest->deregisterForUpdates();
    }

    std::cout << "Exiting application..." << std::endl;
    return 0;
}
