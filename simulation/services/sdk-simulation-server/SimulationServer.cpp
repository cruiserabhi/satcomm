/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       SimulationServer.cpp
 *
 * @brief      Implements the @ref SimulationServer class.
 *
 */

#include <iostream>
#include <thread>
#include <telux/common/CommonDefines.hpp>

#include <grpcpp/grpcpp.h>

#include "../../libs/common/SimulationConfigParser.hpp"
#include "../../libs/common/Logger.hpp"

#include "SimulationServer.hpp"
#include "cv2x/Cv2xManagerServerImpl.hpp"
#include "cv2x/Cv2xThrottleManagerServerImpl.hpp"
#include "cv2x/Cv2xConfigServerImpl.hpp"
#include "cv2x/Cv2xRadioServer.hpp"
#include "tel/CardManagerServerImpl.hpp"
#include "tel/PhoneManagerServerImpl.hpp"
#include "tel/SubscriptionManagerServerImpl.hpp"
#include "tel/SmsManagerServerImpl.hpp"
#include "tel/ImsServingManagerServerImpl.hpp"
#include "tel/ImsSettingsManagerServerImpl.hpp"
#include "tel/ServingManagerServerImpl.hpp"
#include "tel/NetworkSelectionManagerServerImpl.hpp"
#include "data/DataConnectionServerImpl.hpp"
#include "data/DataProfileServerImpl.hpp"
#include "data/DataSettingsServerImpl.hpp"
#include "data/ServingSystemServerImpl.hpp"
#include "data/DataFilterServerImpl.hpp"
#include "data/DualDataServerImpl.hpp"
#include "data/DataControlServerImpl.hpp"
#include "data/DataLinkServerImpl.hpp"
#include "data/net/SocksServerImpl.hpp"
#include "data/net/NatServerImpl.hpp"
#include "data/net/FirewallServerImpl.hpp"
#include "data/net/L2tpServerImpl.hpp"
#include "data/net/BridgeServerImpl.hpp"
#include "data/net/VlanServerImpl.hpp"
#include "loc/LocationManagerServerImpl.hpp"
#include "loc/LocationConfiguratorServerImpl.hpp"
#include "tel/CallManagerServerImpl.hpp"
#include "therm/ThermalGrpcServerImpl.hpp"
#include "event/EventService.hpp"
#include "sensor/SensorFeatureManagerServerImpl.hpp"
#include "loc/LocationReportService.hpp"
#include "audio/AudioGrpcServiceImpl.hpp"
#include "power/PowerManagerServiceImpl.hpp"
#include "sensor/SensorClientServerImpl.hpp"
#include "sensor/SensorReportService.hpp"
#include "tel/SuppServicesManagerServerImpl.hpp"
#include "platform/DeviceInfoManagerServerImpl.hpp"
#include "platform/AntennaManagerServerImpl.hpp"
#include "platform/FsManagerServerImpl.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

#define LOCAL_HOST "127.0.0.1"

/* Defining the SimulationServer app instance */
SimulationServer::SimulationServer() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

SimulationServer::~SimulationServer(){
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = nullptr;
}

SimulationServer &SimulationServer::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static SimulationServer instance;
    return instance;
}

telux::common::Status SimulationServer::start() {
    LOG(DEBUG, __FUNCTION__);

    std::thread grpc_sim_server([this] {
            startGrpcServer();
        }
    );

    grpc_sim_server.join();
    return telux::common::Status::SUCCESS;
}

std::string SimulationServer::createServerAddress(std::string ipAddress, std::string portNo) {
    return ipAddress+ ":" + portNo;
}

void SimulationServer::startGrpcServer() {
    LOG(DEBUG, __FUNCTION__);
    std::string serverIpAddress = LOCAL_HOST;
    auto config = std::make_shared<SimulationConfigParser>();
    std::string serverAddress = createServerAddress(serverIpAddress, config->getValue("RPC_PORT"));
    std::string server_address(serverAddress);

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    std::shared_ptr<CardManagerServerImpl> cardService = std::make_shared<CardManagerServerImpl>();
    builder.RegisterService(cardService.get());

    std::shared_ptr<SubscriptionManagerServerImpl> subscriptionService =
        std::make_shared<SubscriptionManagerServerImpl>();
    builder.RegisterService(subscriptionService.get());

    std::shared_ptr<SmsManagerServerImpl> smsService = std::make_shared<SmsManagerServerImpl>();
    builder.RegisterService(smsService.get());

    std::shared_ptr<Cv2xManagerServerImpl> cv2xRadioMgrService =
        std::make_shared<Cv2xManagerServerImpl>();
    builder.RegisterService(cv2xRadioMgrService.get());

    std::shared_ptr<Cv2xThrottleManagerServerImpl> cv2xThrottleMgrService =
        std::make_shared<Cv2xThrottleManagerServerImpl>();
    builder.RegisterService(cv2xThrottleMgrService.get());

    std::shared_ptr<Cv2xConfigServerImpl> cv2xConfigService =
        std::make_shared<Cv2xConfigServerImpl>();
    builder.RegisterService(cv2xConfigService.get());

    std::shared_ptr<Cv2xRadioServer> cv2xRadioServer =
        std::make_shared<Cv2xRadioServer>();
    if (cv2xRadioServer) {
        std::shared_ptr<telux::cv2x::ICv2xListener> self = cv2xRadioServer;
        cv2xRadioServer->init(self);
    }
    builder.RegisterService(cv2xRadioServer.get());

    std::shared_ptr<DataConnectionServerImpl> dcmService =
        std::make_shared<DataConnectionServerImpl>();
    builder.RegisterService(dcmService.get());

    std::shared_ptr<DataProfileServerImpl> dataprofileService =
        std::make_shared<DataProfileServerImpl>();
    builder.RegisterService(dataprofileService.get());

    std::shared_ptr<DataSettingsServerImpl> dataSettingsService =
        std::make_shared<DataSettingsServerImpl>(dcmService);
    builder.RegisterService(dataSettingsService.get());

    std::shared_ptr<ServingSystemServerImpl> servingSystemService =
        std::make_shared<ServingSystemServerImpl>();
    builder.RegisterService(servingSystemService.get());

    std::shared_ptr<DataFilterServerImpl> dataFilterService =
        std::make_shared<DataFilterServerImpl>(dcmService);
    builder.RegisterService(dataFilterService.get());

    std::shared_ptr<SocksServerImpl> socksService =
        std::make_shared<SocksServerImpl>();
    builder.RegisterService(socksService.get());

    std::shared_ptr<NatServerImpl> natService =
        std::make_shared<NatServerImpl>();
    builder.RegisterService(natService.get());

    std::shared_ptr<L2tpServerImpl> l2tpService =
        std::make_shared<L2tpServerImpl>();
    builder.RegisterService(l2tpService.get());

    std::shared_ptr<FirewallServerImpl> firewallService =
        std::make_shared<FirewallServerImpl>();
    builder.RegisterService(firewallService.get());

    std::shared_ptr<BridgeServerImpl> bridgeService =
        std::make_shared<BridgeServerImpl>();
    builder.RegisterService(bridgeService.get());

    std::shared_ptr<VlanServerImpl> vlanService =
        std::make_shared<VlanServerImpl>();
    builder.RegisterService(vlanService.get());

    std::shared_ptr<DualDataServerImpl> dualDataService =
        std::make_shared<DualDataServerImpl>();
    builder.RegisterService(dualDataService.get());

    std::shared_ptr<DataControlServerImpl> dataControlService =
        std::make_shared<DataControlServerImpl>();
    builder.RegisterService(dataControlService.get());

    std::shared_ptr<DataLinkServerImpl> dataLinkService =
        std::make_shared<DataLinkServerImpl>();
    builder.RegisterService(dataLinkService.get());

    auto& locEventService = LocationReportService::getInstance();
    builder.RegisterService(&locEventService);

    std::shared_ptr<LocationManagerServerImpl> locManagerService =
        std::make_shared<LocationManagerServerImpl>();
    builder.RegisterService(locManagerService.get());

    std::shared_ptr<LocationConfiguratorServerImpl> locConfigService =
        std::make_shared<LocationConfiguratorServerImpl>();
    builder.RegisterService(locConfigService.get());

    std::shared_ptr<CallManagerServerImpl> callService =
        std::make_shared<CallManagerServerImpl>();
    builder.RegisterService(callService.get());

    std::shared_ptr<PhoneManagerServerImpl> phoneService =
        std::make_shared<PhoneManagerServerImpl>();
    builder.RegisterService(phoneService.get());

    std::shared_ptr<ThermalGrpcServerImpl> thermalService =
        std::make_shared<ThermalGrpcServerImpl>();
    builder.RegisterService(thermalService.get());

    auto& eventService = EventService::getInstance();
    builder.RegisterService(&eventService);

    std::shared_ptr<telux::audio::AudioGrpcServiceImpl> audioService =
        std::make_shared<telux::audio::AudioGrpcServiceImpl>();
    builder.RegisterService(audioService.get());

    std::shared_ptr<SensorFeatureManagerServerImpl> sensorService =
        std::make_shared<SensorFeatureManagerServerImpl>();
    builder.RegisterService(sensorService.get());

    std::shared_ptr<ImsServingManagerServerImpl> imsServingSystemService =
        std::make_shared<ImsServingManagerServerImpl>();
    builder.RegisterService(imsServingSystemService.get());

    std::shared_ptr<ImsSettingsManagerServerImpl> imsSettingsService =
        std::make_shared<ImsSettingsManagerServerImpl>();
    builder.RegisterService(imsSettingsService.get());

    std::shared_ptr<ServingManagerServerImpl> ServingSystemService =
        std::make_shared<ServingManagerServerImpl>();
    builder.RegisterService(ServingSystemService.get());

    std::shared_ptr<NetworkSelectionManagerServerImpl> NetworkSelectionSystemService =
        std::make_shared<NetworkSelectionManagerServerImpl>();
    builder.RegisterService(NetworkSelectionSystemService.get());

    std::shared_ptr<SensorClientServerImpl> sensorClientService =
        std::make_shared<SensorClientServerImpl>();
    builder.RegisterService(sensorClientService.get());

    auto& sensorEventService = SensorReportService::getInstance();
    builder.RegisterService(&sensorEventService);

    std::shared_ptr<PowerManagerServiceImpl> powerService =
        std::make_shared<PowerManagerServiceImpl>();
    builder.RegisterService(powerService.get());

    std::shared_ptr<DeviceInfoManagerServerImpl> DeviceInfoManagerService =
        std::make_shared<DeviceInfoManagerServerImpl>();
    builder.RegisterService(DeviceInfoManagerService.get());

    std::shared_ptr<SuppServicesManagerServerImpl> suppService =
        std::make_shared<SuppServicesManagerServerImpl>();
    builder.RegisterService(suppService.get());

    std::shared_ptr<AntennaManagerServerImpl> AntennaManagerService =
        std::make_shared<AntennaManagerServerImpl>();
    builder.RegisterService(AntennaManagerService.get());

    std::shared_ptr<FsManagerServerImpl> FsManagerService =
        std::make_shared<FsManagerServerImpl>();
    builder.RegisterService(FsManagerService.get());

    std::unique_ptr<Server> server(builder.BuildAndStart());
    LOG(DEBUG, __FUNCTION__, " Server listening on ", server_address);
    server->Wait();
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {
    auto &simulationServer = SimulationServer::getInstance();

    if (simulationServer.start() != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed to start ", APP_NAME);
        return -1;
    }

    std::cout << "\nInfo: Exiting application..." << std::endl;
    return 0;
}
