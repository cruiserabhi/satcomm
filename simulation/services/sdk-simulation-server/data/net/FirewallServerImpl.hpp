/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FIREWALL_MANAGER_SERVER_HPP
#define FIREWALL_MANAGER_SERVER_HPP

#include <telux/data/net/FirewallManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include "libs/data/DataUtilsStub.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class FirewallServerImpl final:
    public dataStub::FirewallManager::Service {
public:
    FirewallServerImpl();
    ~FirewallServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::InitRequest* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetFirewall(ServerContext* context,
        const dataStub::SetFirewallRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestFirewallStatus(ServerContext* context,
        const dataStub::FirewallStatusRequest* request,
        dataStub::RequestFirewallStatusReply* response) override;

    grpc::Status AddFirewallEntry(ServerContext* context,
        const dataStub::AddFirewallEntryRequest* request,
        dataStub::AddFirewallEntryReply* response) override;

    grpc::Status RemoveFirewallEntry(ServerContext* context,
        const dataStub::RemoveFirewallEntryRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestFirewallEntries(ServerContext* context,
        const dataStub::FirewallEntriesRequest* request,
        dataStub::RequestFirewallEntriesReply* response) override;

    grpc::Status EnableDMZ(ServerContext* context,
        const dataStub::EnableDMZRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status DisableDMZ(ServerContext* context,
        const dataStub::DisableDmzRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestDMZEntry(ServerContext* context,
        const dataStub::DMZEntryRequest* request,
        dataStub::RequestDMZEntryReply* response) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    uint32_t getHandle();
    template <typename T>
    bool isConfigAvailable(std::string subsystem, std::string method, const JsonData& data,
        const T* request, int& configIdx = 0) {
        const Json::Value& config = data.stateRootObj[subsystem][method];
        int count = config.size();
        bool isFound = false;
        int idx = 0;
        for (idx=0; idx < count; idx++) {
            if (config[idx]["backhaul"].asString() !=
                    DataUtilsStub::convertEnumToBackhaulPrefString(request->backhaul_type())) {
                continue;
            }
            if (config[idx]["slotId"].asInt() != request->slot_id()) {
                continue;
            }
            if (config[idx]["profileId"].asInt() != request->profile_id()) {
                continue;
            }
            isFound = true;
            break;
        }
        if (isFound) {
            configIdx = idx;
        }
        return isFound;
    }

    template <typename T>
    bool isConfigAvailableForBackhaul(std::string subsystem, std::string method,
            const JsonData& data, const T* request, int& configIdx = 0) {
        const Json::Value& config = data.stateRootObj[subsystem][method];
        int count = config.size();
        bool isFound = false;
        int idx = 0;
        for (idx=0; idx < count; idx++) {
            if (config[idx]["backhaul"].asString() !=
                    DataUtilsStub::convertEnumToBackhaulPrefString(request->backhaul_type())) {
                continue;
            }
            if (request->backhaul_type() == ::dataStub::BackhaulPreference::PREF_WWAN) {
                if (config[idx]["slotId"].asInt() != request->slot_id()) {
                    continue;
                }
                if (config[idx]["profileId"].asInt() != request->profile_id()) {
                    continue;
                }
            } else if (request->backhaul_type() == ::dataStub::BackhaulPreference::PREF_ETH) {
                if (config[idx]["vlanId"].asInt() != request->vlan_id()) {
                    continue;
                }
            }
            isFound = true;
            break;
        }
        if (isFound) {
            configIdx = idx;
        }
        return isFound;
    }

    template <typename T>
    bool isFirewallEntryAvailable(std::string subsystem, std::string method, const JsonData& data,
        const T* request, int& configIdx = 0) {
        const Json::Value& config = data.stateRootObj[subsystem][method];
        int count = config.size();
        bool isFound = false;
        int idx = 0;
        for (idx=0; idx < count; idx++) {
            if (config[idx]["backhaul"].asString() !=
                DataUtilsStub::convertEnumToBackhaulPrefString(request->backhaul_type())) {
                continue;
            }
            if (config[idx]["slotId"].asInt() != request->slot_id()) {
                continue;
            }
            if (config[idx]["profileId"].asInt() != request->profile_id()) {
                continue;
            }
            if (config[idx]["fw_direction"].asInt() !=
                request->fw_direction().fw_direction()) {
                continue;
            }
            if (config[idx]["protocol"].asString() != request->protocol()) {
                continue;
            }

            std::string ipFamily = DataUtilsStub::convertIpFamilyEnumToString(
                request->ip_family_type().ip_family_type());

            if (ipFamily ==  "IPV4") {
                if (request->ipv4_params().ipv4_src_address() !=
                    config[idx]["ipv4_srcAddr"].asString()) {
                        continue;
                }
            }
            if (ipFamily == "IPV6") {
                if (request->ipv6_params().ipv6_src_address() !=
                    config[idx]["ipv6_srcAddr"].asString()) {
                        continue;
                }
            }
            isFound = true;
            break;
        }
        if (isFound) {
            configIdx = idx;
        }
        return isFound;
    }
};

#endif //FIREWALL_MANAGER_SERVER_HPP
