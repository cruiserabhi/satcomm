/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMAL_GRPC_SERVER_HPP
#define THERMAL_GRPC_SERVER_HPP

#include <algorithm>
#include <memory>
#include <vector>
#include <telux/common/CommonDefines.hpp>
#include "Logger.hpp"
#include "protos/proto-src/therm_simulation.grpc.pb.h"
#include <telux/therm/ThermalManager.hpp>
#include "CommonUtils.hpp"

#include "common/therm/ThermalZoneImpl.hpp"
#include "common/therm/CoolingDeviceImpl.hpp"
#include "SimulationServer.hpp"
#include "ThermalJsonImpl.hpp"
#include "ThermalManagerServerImpl.hpp"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using thermStub::Thermal;
using commonStub::ServiceStatus;
using commonStub::GetServiceStatusReply;

class ThermalGrpcServerImpl final : public thermStub::Thermal::Service,
                                    public ThermalManagerServerImpl {
    public:

        ThermalGrpcServerImpl();
        ~ ThermalGrpcServerImpl();

        /* ############## Overrided methods ####################### */
        grpc::Status InitService(ServerContext *context,
            const google::protobuf::Empty *request,
            commonStub::GetServiceStatusReply* response) override ;

        grpc::Status GetServiceStatus(ServerContext* context,
            const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;

        grpc::Status GetThermalZones(ServerContext* context,
            const ::thermStub::GetThermalZonesRequest* request,
            ::thermStub::GetThermalZonesReply* response) override;
        grpc::Status GetCoolingDevices(ServerContext* context,
            const ::thermStub::GetCoolingDevicesRequest* request,
            ::thermStub::GetCoolingDevicesReply* response) override;

        grpc::Status GetThermalZoneById(ServerContext* context,
            const ::thermStub::GetThermalZoneByIdRequest* request,
            ::thermStub::GetThermalZoneByIdReply* response) override;
        grpc::Status GetCoolingDeviceById(ServerContext* context,
            const ::thermStub::GetCoolingDeviceByIdRequest* request,
            ::thermStub::GetCoolingDeviceByIdReply* response) override;

        grpc::Status RegisterOnCoolingDeviceLevelChange(ServerContext* context,
            const google::protobuf::Empty* request,
            ServerWriter<::thermStub::RegisterOnCoolingDeviceLevelChangeReply>* response) override;
        grpc::Status DeRegisterOnCoolingDeviceLevelChange(ServerContext* context,
            const ::commonStub::DeRegisterNotificationRequest* request,
            google::protobuf::Empty* response) override;

        grpc::Status RegisterOnTripEvent(ServerContext* context,
            const google::protobuf::Empty* request,
            ServerWriter<::thermStub::RegisterOnTripEventReply>* response) override;

        grpc::Status DeRegisterOnTripEvent(ServerContext* context,
            const ::commonStub::DeRegisterNotificationRequest* request,
            google::protobuf::Empty* response) override;

    private:

        /* ############## Helper methods ####################### */
        grpc::Status setResponse(telux::common::ServiceStatus srvStatus,
                commonStub::GetServiceStatusReply* response);

        ::thermStub::TripPoint_TripType getTripType(telux::therm::TripType tripType);

        telux::common::Status sendTripUpdateEvent(std::shared_ptr<telux::therm::ITripPoint> &tp,
                int tZoneId, int event);
        telux::common::Status sendCdevUpdateEvent(std::shared_ptr<telux::therm::CoolingDeviceImpl> &cd,
                unsigned int newState);

        telux::common::Status getNewCdevStateUpdate(int trend, int tZoneId, int tripId);
        telux::common::Status getTripUpdate(int prevTemp, int newTemp, int tZoneId,
                std::shared_ptr<telux::therm::ITripPoint> &tp);

        // Call this when Temp changes for particular Tzone
        telux::common::Status getTripAndCdevUpdate(int tZoneId, int prevTzoneTemp, int newTzoneTemp,
                std::shared_ptr<telux::therm::ITripPoint> tp);
        telux::common::Status setThermalZone(int tZoneId, int newTemp);

        void onSSREvent(telux::common::ServiceStatus srvStatus);

        template <class T>
            grpc::Status registerNotification(ServerWriter<T>* writer, std::map<uint16_t,
                    ServerWriter<T>*> &writerList, std::mutex &mutex, std::condition_variable &cv,
                    uint16_t &clientId) {
                std::unique_lock<std::mutex> lk(mutex);
                    writerList.insert(std::pair<uint16_t, ServerWriter<T>*>(clientId, writer));
                    T reply;
                    int client_id = clientId;
                    clientId++;
                    reply.set_client_id(client_id);
                    if (writer) {
                        writer->Write(reply);
                    }
                cv.wait(lk, [&]{return (writerList.find(client_id) == writerList.end());});
                    return grpc::Status::OK;
            }

            template <class T>
            grpc::Status deRegisterNotification(std::map<uint16_t, ServerWriter<T>*> &writerList,
                    std::mutex &mutex, std::condition_variable &cv, uint16_t clientId) {
                std::unique_lock<std::mutex> lk(mutex);
                    typename std::map<uint16_t, ServerWriter<T>*>::iterator it;
                    it = writerList.find(clientId);
                    if (it != writerList.end()) {
                        writerList.erase(it);
                    }
                cv.notify_all();
                    return grpc::Status::OK;
            }

            grpc::Status registerTripEvent(
                    ServerWriter<::thermStub::RegisterOnTripEventReply>* response);
            grpc::Status deRegisterTripEvent(int clientId);
            grpc::Status registerCdevStateChangeEvent(
                    ServerWriter<::thermStub::RegisterOnCoolingDeviceLevelChangeReply>* response);
            grpc::Status deRegisterCdevStateChangeEvent(int clientId);

    private:

        std::shared_ptr<telux::therm::CoolingDeviceImpl> setCoolingDevice(int tZoneId, int tripId,
                int cDevId, int trend, int nextCdevState);

        std::shared_ptr<ThermalJsonImpl> jsonHelper_;
        std::map<uint16_t, ServerWriter<thermStub::RegisterOnCoolingDeviceLevelChangeReply>*>
            onCoolingDeviceLevelChangeReplyWriters_;
        std::mutex onCoolingDeviceLevelChangeMutex_;
        std::condition_variable onCoolingDeviceLevelChangeCv_;
        static uint16_t OnCdevLevelChngNotifyCnt_;
        std::map<uint16_t, ServerWriter<thermStub::RegisterOnTripEventReply>*>
            onTripEventReplyWriters_;
        std::mutex onTripEventMutex_;
        std::mutex setTempMutex_;
        std::mutex setCdevMutex_;
        std::condition_variable onTripEventCv_;
        static uint16_t OnTripEvntNotifyCnt_;
};

#endif // THERMAL_GRPC_SERVER_HPP
