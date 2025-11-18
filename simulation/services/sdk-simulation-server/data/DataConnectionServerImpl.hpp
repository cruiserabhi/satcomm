/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_CONNECTION_SERVER_HPP
#define DATA_CONNECTION_SERVER_HPP

#include <iostream>
#include <memory>
#include <list>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/AsyncTaskQueue.hpp"

#include "event/ServerEventManager.hpp"

#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using ::dataStub::DataConnectionManager;

/**
 * Throttle information for the corresponding APN
 */
struct APNThrottleInfo {
    std::string apn;                            /**< APN name */
    std::vector<int> profileIds;                /**< Profile IDs with the same APN */
    uint32_t ipv4Time;                          /**< Remaining IPv4 throttled time in milliseconds*/
    uint32_t ipv6Time;                          /**< Remaining IPv6 throttled time in milliseconds*/
    bool isBlocked;                             /**< Is APN blocked on all plmns */
    std::string mcc;                            /**< Mobile Country Code */
    std::string mnc;                            /**< Mobile Network Code */
};


struct DataCallParams {
    int slotId;
    std::string ifaceName;
    std::string ipFamilyType;
    std::string v4IpAddress;
    std::string v4GwAddress;
    std::string v4dnsPrimaryAddress;
    std::string v4dnsSecondaryAddress;
    std::string v6IpAddress;
    std::string v6GwAddress;
    std::string v6dnsPrimaryAddress;
    std::string v6dnsSecondaryAddress;
    std::set<int> ownersId;
};

class DataConnectionServerImpl final:
    public dataStub::DataConnectionManager::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<DataConnectionServerImpl> {
public:
    DataConnectionServerImpl();
    ~DataConnectionServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::SlotInfo* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetDefaultProfile(ServerContext* context,
        const dataStub::SetDefaultProfileRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status GetDefaultProfile(ServerContext* context,
        const dataStub::GetDefaultProfileRequest* request,
        dataStub::GetDefaultProfileReply* response) override;

    grpc::Status SetRoamingMode(ServerContext* context,
        const dataStub::SetRoamingModeRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestRoamingMode(ServerContext* context,
        const dataStub::RequestRoamingModeRequest* request,
        dataStub::RequestRoamingModeReply* response) override;

    grpc::Status StartDatacall(ServerContext* context,
        const dataStub::DataCallInputParams* request,
        dataStub::DefaultReply* response) override;

    grpc::Status StopDatacall(ServerContext* context,
        const dataStub::DataCallInputParams* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestDatacallList(ServerContext* context,
        const dataStub::DataCallInputParams* request,
        dataStub::RequestDataCallListReply* response) override;

    grpc::Status RequestThrottledApnInfo(ServerContext* context,
        const dataStub::SlotInfo* request,
        dataStub::ThrottleInfoReply* response) override;

    grpc::Status CleanUpService(ServerContext* context,
        const ::dataStub::ClientInfo* request,
        ::google::protobuf::Empty* response) override;

    grpc::Status requestConnectedDataCallLists(ServerContext* context,
        const dataStub::CachedDataCallsRequest* request,
        dataStub::CachedDataCalls* response) override;

    /* Could be used if all the datacalls need to be teared down.
     * For ex: if WWAN connectivity is disabled via DataSettingsManager, then
     * all the datacalls need to be teared down.
     */
    void stopActiveDataCalls(SlotId slotId);

    /* Could be used to check if any datacall exist.
     * For ex: DataRestrictMode is enabled only if atleast one datacall exist.
     */
    bool isAnyDataCallActive(SlotId slotId);

    void onEventUpdate(::eventService::UnsolicitedEvent message);

private:
    bool getIpv4Address(const std::string &ifaceName,
        std::string &ipAddress, std::string &gatewayAddress,
        std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress);
    bool getIpv6Address(const std::string &ifaceName,
        std::string &ipAddress, std::string &gatewayAddress,
        std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress);

    void triggerStartDataCallEvent(int profileId, int slotId, std::string ipFamilyType,
        unsigned int client_id, std::string ifaceName = "");
    void triggerStopDataCallEvent(int profileId, int slotId, std::string ipFamilyType,
        std::string ifaceName);
    void triggerThrottledApnInfoChangedEvent(dataStub::APNThrottleInfoList* response);


    void getInactiveInterfaces();
    bool isWwanConnectivityAllowed(int slotId);

    void onEventUpdate(std::string event);
    void handleThrottleApnEvent(std::string event);
    bool checkRetryTimeElapsed();
    bool updateThrottleApnInfo();
    void startThrottleRetryTimer();
    void notifyThrottledApnInfoEvent();

    void clearCachedDataCall(std::map<int, std::shared_ptr<DataCallParams>>& dataCallsMap,
        bool stopAllCalls = false, const unsigned int& client_id = 0);

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::map<int, std::shared_ptr<DataCallParams>> dataCallsSlot1_;
    std::map<int, std::shared_ptr<DataCallParams>> dataCallsSlot2_;
    /* Everytime datacall is triggered, we are reading list of interfaces from conf file.
     * In activeNwIfaces_ we are maintaining interfaces that are associated with a datacall
     * & in inactiveNwIfaces_ we are maintaining interfaces that are not yet associated with
     * datacall.
     */
    std::list<std::string> activeNwIfaces_;
    std::list<std::string> inactiveNwIfaces_;
    std::vector<APNThrottleInfo> apnThrottleInfo_;
    std::mutex apnThrottleInfoMtx_;
    std::atomic<bool> timerStarted_;
    std::future<void> timerFuture_;
    telux::common::ServiceStatus serviceStatus_ =
          telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex mtx_;
};

#endif //DATA_CONNECTION_SERVER_HPP