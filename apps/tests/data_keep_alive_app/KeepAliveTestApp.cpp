/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <memory>
#include <cstdlib>
#include <csignal>

#include <telux/data/DataFactory.hpp>
#include <telux/data/KeepAliveManager.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "KeepAliveTestApp.hpp"

#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

using namespace telux::data;
using namespace telux::common;

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
/**
 * @file: KeepAliveTestApp.cpp
 *
 * @brief: Test application to demonstrate TCP KeepAlive offloading.
 */

static std::mutex mutex;
static std::condition_variable cv;
std::shared_ptr<KeepAliveTestApp> myKeepAliveTestApp;

void KeepAliveTestApp::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::cout << "\n";
    PRINT_NOTIFICATION << " ** Service status has changed ** \n";
    if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << "Service Unavailable.";
    } else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Service Available.";
    } else {
        std::cout << "Unknown service state.";
    }
}

void KeepAliveTestApp::onKeepAliveStatusChange(ErrorCode error, TCPKAOffloadHandle handle) {
    std::cout << "\n";
    PRINT_NOTIFICATION << " ** Keep alive status has changed ** \n";
    std::cout << " handle: " << handle << std::endl;
    if (error == ErrorCode::SUCCESS) {
        std::cout << "TCP keep-alive offloading started.\n";
    } else if (error == ErrorCode::NETWORK_ERR) {
        std::cout << "TCP keep-alive offloading error NETWORK_ERR.\n";
    } else if (error == ErrorCode::CANCELLED) {
        std::cout << "TCP keep-alive offloading error ErrorCode::CANCELLED.\n";
    } else {
        std::cout << "TCP keep-alive offloading error : " << static_cast<int>(error) << std::endl;
    }
}

void KeepAliveTestApp::startServerThread() {
    server_->startServer();
}

void KeepAliveTestApp::startTCPServer(std::vector<std::string> inputCommand) {
    if (server_) {
        std::cout << "Server already running !!\n";
        return;
    }

    std::string ipaddr;
    int port;

    std::cout << "Enter IPv4/IPV6 address: ";
    std::cin >> ipaddr;
    std::cout << "Enter port number: ";
    std::cin >> port;
    serverWorker_ = std::make_shared<TCPServerWorker<kaproto>>();
    server_ = std::make_shared<TCPServer<kaproto>>(serverWorker_, port, ipaddr);
    serverWorker_->setServer(server_);
    serverThread_ = std::thread{&KeepAliveTestApp::startServerThread, this};
    serverThread_.detach();
}

void KeepAliveTestApp::stopTCPServer(std::vector<std::string> inputCommand) {
    if(server_) {
        server_->disconnect();
        server_ = nullptr;
    }
}

void KeepAliveTestApp::sendMessage(std::vector<std::string> inputCommand) {
    kaproto msg;
    memset(&msg, 0, sizeof(msg));
    const char* message = (inputCommand[1]+ "\n").c_str();
    std::copy(message, message + strlen(message) + 1, msg.msg);
    if(isServer_) {
        if(server_) {
            server_->sendMessage(&msg);
        } else {
            std::cout << " start server first\n";
        }
    } else {
        if(client_){
            client_->sendMessage(&msg);
        } else {
            std::cout << " start client first\n";
        }

    }
}

void KeepAliveTestApp::startClientThread() {
    client_->connectTo();
}

void KeepAliveTestApp::startTCPClient(std::vector<std::string> inputCommand) {
    if (client_) {
        std::cout << "Client already running !!\n";
        return;
    }
    std::shared_ptr<TCPClient<kaproto>> client;

    std::string serverIpAddr;
    int serverPort;
    std::string clientIpAddr = "";
    int clientPort = 0;

    std::cout << "Enter IPv4/IPV6 server address to connect to: ";
    std::cin >> serverIpAddr;
    std::cout << "Enter server port number: ";
    std::cin >> serverPort;

    int userChoice;
    std::cout << "Bind client address and port? (1-Yes, 0-No): ";
    std::cin >> userChoice;
    Utils::validateInput(userChoice, {0, 1});
    std::cout << std::endl;
    if(userChoice) {
        std::cout << "Enter IPv4/IPV6 client address start listening on: ";
        std::cin >> clientIpAddr;
        std::cout << "Enter client port number: ";
        std::cin >> clientPort;
    }

    clientWorker_ = std::make_shared<TCPClientWorker<kaproto>>();
    client_ = std::make_shared<TCPClient<kaproto>>(clientWorker_, serverPort, serverIpAddr,
        clientPort, clientIpAddr);
    clientThread_ = std::thread{&KeepAliveTestApp::startClientThread, this};
    clientThread_.detach();
}

void KeepAliveTestApp::stopTCPClient(std::vector<std::string> inputCommand) {
    if(client_) {
        client_->disconnect();
        client_ = nullptr;
    }
}

void KeepAliveTestApp::enableTCPMonitor(std::vector<std::string> inputCommand) {
    if (!keepAliveMgr_) {
        std::cout << "KeepAlive Manager not ready !\n";
        return;
    }

    struct TCPKAParams params{};

    std::cout << "Enter source IPv4/IPv6 address: ";
    std::cin >> params.srcIp;
    std::cout << "Enter destination IPv4/IPv6 address: ";
    std::cin >> params.dstIp;
    std::cout << "Enter source port: ";
    std::cin >> params.srcPort;
    std::cout << "Enter destination port: ";
    std::cin >> params.dstPort;

    MonitorHandleType monHandle;
    auto err = keepAliveMgr_->enableTCPMonitor(params, monHandle);
    if (err != ErrorCode::SUCCESS) {
        std::cout << "Operation failed with errorcode: " << static_cast<int>(err) << std::endl;
    } else {
        std::cout << "Operation completed. Monitor handle : " << monHandle << std::endl;
    }
}

void KeepAliveTestApp::disableTCPMonitor(std::vector<std::string> inputCommand) {
    if (!keepAliveMgr_) {
        std::cout << "KeepAlive Manager not ready !\n";
        return;
    }

    MonitorHandleType monHandle;
    std::cout << "Enter monitor handle: ";
    std::cin >> monHandle;

    auto err = keepAliveMgr_->disableTCPMonitor(monHandle);
    if (err != ErrorCode::SUCCESS) {
        std::cout << "Operation failed with errorcode: " << static_cast<int>(err) << std::endl;
    }
}

void KeepAliveTestApp::startTCPKeepAliveOffload(std::vector<std::string> inputCommand) {
    if (!keepAliveMgr_) {
        std::cout << "KeepAlive Manager not ready !\n";
        return;
    }

    int mode = 0;
    std::cout << "Enter type of startTCPKeepAliveOffload API (0: default mode, 1: monitor mode) : ";
    std::cin >> mode;
    TCPKAOffloadHandle handle;
    ErrorCode err = ErrorCode::SUCCESS;
    struct TCPKAParams params{};
    struct TCPSessionParams session{};
    uint32_t interval = 0;
    MonitorHandleType monHandle;

    switch(mode) {
        case 0:
            std::cout << "Enter source IPv4/IPv6 address: ";
            std::cin >> params.srcIp;
            std::cout << "Enter destination IPv4/IPv6 address: ";
            std::cin >> params.dstIp;
            std::cout << "Enter source port: ";
            std::cin >> params.srcPort;
            std::cout << "Enter destination port: ";
            std::cin >> params.dstPort;
            std::cout << "Enter recvNext: ";
            std::cin >> session.recvNext;
            std::cout << "Enter recvWindow: ";
            std::cin >> session.recvWindow;
            std::cout << "Enter sendNext: ";
            std::cin >> session.sendNext;
            std::cout << "Enter sendWindow: ";
            std::cin >> session.sendWindow;
            std::cout << "Enter interval: ";
            std::cin >> interval;

            err = keepAliveMgr_->startTCPKeepAliveOffload(params, session, interval, handle);
            break;

        case 1:
            std::cout << "Enter interval: ";
            std::cin >> interval;
            std::cout << "Enter monitor handle: ";
            std::cin >> monHandle;

            err = keepAliveMgr_->startTCPKeepAliveOffload(monHandle, interval, handle);
            break;

        default:
            std::cout << "Wrong type!\n";
            return;
    }
    if (err != ErrorCode::SUCCESS) {
        std::cout << "Operation failed with errorcode: " << static_cast<int>(err) << std::endl;
    } else {
        std::cout << "Operation completed. TCPKAOffload handle : " << handle << std::endl;
    }
}

void KeepAliveTestApp::stopTCPKeepAliveOffload(std::vector<std::string> inputCommand) {
    if (!keepAliveMgr_) {
        std::cout << "KeepAlive Manager not ready !\n";
        return;
    }

    TCPKAOffloadHandle handle;
    std::cout << "Enter TCPKAOffload handle: ";
    std::cin >> handle;

    auto err = keepAliveMgr_->stopTCPKeepAliveOffload(handle);
    if (err != ErrorCode::SUCCESS) {
        std::cout << "Operation failed with errorcode: " << static_cast<int>(err) << std::endl;
    }
}

void KeepAliveTestApp::consoleInit(bool isServer) {
    isServer_ = isServer;
   std::shared_ptr<ConsoleAppCommand> startTCPServerCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "1", "startTCPServer", {},
         std::bind(&KeepAliveTestApp::startTCPServer, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> stopTCPServerCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "2", "stopTCPServer", {},
         std::bind(&KeepAliveTestApp::stopTCPServer, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> startTCPClientCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "1", "startTCPClient", {},
         std::bind(&KeepAliveTestApp::startTCPClient, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> stopTCPClientCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "2", "stopTCPClient", {},
         std::bind(&KeepAliveTestApp::stopTCPClient, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> sendMessageCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "3", "sendMessage", {"message"},
         std::bind(&KeepAliveTestApp::sendMessage, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableTCPMonitorCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "4", "enableTCPMonitor", {},
         std::bind(&KeepAliveTestApp::enableTCPMonitor, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> disableTCPMonitorCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "5", "disableTCPMonitor", {},
         std::bind(&KeepAliveTestApp::disableTCPMonitor, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> startTCPKeepAliveOffloadCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "6", "startTCPKeepAliveOffload", {},
         std::bind(&KeepAliveTestApp::startTCPKeepAliveOffload, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> stopTCPKeepAliveOffloadCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand( "7", "stopTCPKeepAliveOffload", {},
         std::bind(&KeepAliveTestApp::stopTCPKeepAliveOffload, this, std::placeholders::_1)));

    if(isServer){
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListTCPKAMenu = {
            startTCPServerCommand, stopTCPServerCommand, sendMessageCommand,
            enableTCPMonitorCommand, disableTCPMonitorCommand, startTCPKeepAliveOffloadCommand,
            stopTCPKeepAliveOffloadCommand};
        ConsoleApp::addCommands(commandsListTCPKAMenu);
    } else {
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListTCPKAMenu = {
            startTCPClientCommand, stopTCPClientCommand, sendMessageCommand,
            enableTCPMonitorCommand, disableTCPMonitorCommand, startTCPKeepAliveOffloadCommand,
            stopTCPKeepAliveOffloadCommand};
        ConsoleApp::addCommands(commandsListTCPKAMenu);
    }
   ConsoleApp::displayMenu();
}

void KeepAliveTestApp::registerForUpdates() {
   Status status = keepAliveMgr_->registerListener(shared_from_this());
   if(status != Status::SUCCESS) {
      std::cout << APP_NAME << " ERROR - Failed to register for keep-alive notification"
         << std::endl;
   } else {
      std::cout << APP_NAME << " Registered Listener for keep-alive notification" << std::endl;
   }
}

void KeepAliveTestApp::deregisterForUpdates() {
   Status status = keepAliveMgr_->deregisterListener(shared_from_this());
   if(status != Status::SUCCESS) {
      std::cout << APP_NAME << " ERROR - Failed to de-register for keep-alive notification"
         << std::endl;
   } else {
      std::cout << APP_NAME << " De-registered listener" << std::endl;
   }
}

std::shared_ptr<KeepAliveTestApp> init() {
    std::shared_ptr<KeepAliveTestApp> keepAliveApp = std::make_shared<KeepAliveTestApp>();
    if (!keepAliveApp) {
        std::cout << "Failed to instantiate KeepAliveTestApp" << std::endl;
        return nullptr;
    }

    auto &dataFactory = DataFactory::getInstance();
    telux::common::ServiceStatus subSystemStatus;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    std::promise<telux::common::ServiceStatus> kaProm;
    keepAliveApp->keepAliveMgr_ = dataFactory.getKeepAliveManager(static_cast<SlotId>(slotId),
        [&kaProm](telux::common::ServiceStatus status) {
        std::cout << " Callback invoked "<< static_cast<int>(status);
        kaProm.set_value(status);
    });

    if (!keepAliveApp->keepAliveMgr_) {
        std::cout << " Failed to get keepAliveMgr object";
        return nullptr;
    }

    std::cout << " Initializing keep alive subsystem Please wait";
    subSystemStatus = kaProm.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout <<" Keep alive Manager is ready";

    } else {
        std::cout << " Keep alive Manager is failed";
        keepAliveApp->keepAliveMgr_ = nullptr;
        return nullptr;
    }

    if(keepAliveApp->keepAliveMgr_->getServiceStatus() ==
        telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** KeepAlive Sub System is Ready *** " << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize keep-alive subsystem *** " << std::endl;
        return nullptr;
    }

   return keepAliveApp;
}

static void printHelp() {
    std::cout <<
    "-----------------------------------------------\n"
    "./keepAlive_test_app <-cs> <-S> <-h>\n"
    "   -c : run as client\n"
    "   -s : run as server\n"
    "   -h : print the help menu" << std::endl;
}

KeepAliveTestApp::KeepAliveTestApp()
    : ConsoleApp("TCP KeepAlive Test app Menu", "tcpka-test> ")
    , keepAliveMgr_(nullptr)
    , server_(nullptr)
    {
}

KeepAliveTestApp::~KeepAliveTestApp() {
    server_ = nullptr;
    client_ = nullptr;
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {

    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
    }

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        // We can call exit() here if no cleanups needed,
        // or maybe just set a flag, and let the main thread to decide
        // when to exit.
        if(myKeepAliveTestApp) {
            //cleanup server/client thread
            std::cout << APP_NAME << " Cleanup server/client" << std::endl;
            myKeepAliveTestApp->stopTCPClient({});
            myKeepAliveTestApp->stopTCPServer({});

            std::cout << APP_NAME << " deregisterForUpdates" << std::endl;
            myKeepAliveTestApp->deregisterForUpdates();

            std::cout << APP_NAME << " keepAliveMgr_ = nullptr" << std::endl;
            myKeepAliveTestApp->keepAliveMgr_ = nullptr;

            std::cout << APP_NAME << " myKeepAliveTestApp = nullptr " << std::endl;
            myKeepAliveTestApp = nullptr;

            std::cout << APP_NAME << " exit " << std::endl;
        }

        void *array[10];
        size_t size;

        // Get void*'s for all entries on the stack
        size = backtrace(array, 10);

        // Print out all the frames to stderr
        fprintf(stderr, "Error: signal %d:\n", sig);
        backtrace_symbols_fd(array, size, STDERR_FILENO);
        exit(sig);
    };
    SignalHandler::registerSignalHandler(sigset, cb);

    std::cout <<argc<<std::endl;
    if(argc != 2) {
        printHelp();
        return -1;
    }
   std::cout
       << "\n#################################################\n"
       << "KeepAlive Offload Test Application\n"
       << "#################################################\n" << std::endl;

    bool isServer = true;
    int opt;

    while ((opt = getopt(argc, argv, "sch")) != -1) {
        switch (opt) {
            case 's':
                isServer = true;
                break;
            case 'c':
                isServer = false;
                break;
            case 'h':
            default:
                printHelp();
                exit(-EINVAL);
        }
    }

    myKeepAliveTestApp = init();
    myKeepAliveTestApp->registerForUpdates();
    myKeepAliveTestApp->consoleInit(isServer);
    myKeepAliveTestApp->mainLoop();
    std::cout << "Exiting application..." << std::endl;
    return 0;
}
