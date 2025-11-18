/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NAT_MANAGER_SERVER_HPP
#define NAT_MANAGER_SERVER_HPP

#include <telux/data/net/NatManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "libs/common/CommonUtils.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

#define WWAN_BH_IDX 0
#define ETH_BH_IDX 1

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class NatServerImpl final:
    public dataStub::SnatManager::Service {
public:
    NatServerImpl();
    ~NatServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::InitRequest* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status AddStaticNatEntry(ServerContext* context,
        const dataStub::StaticNatRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RemoveStaticNatEntry(ServerContext* context,
        const dataStub::StaticNatRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestStaticNatEntries(ServerContext* context,
        const dataStub::RequestStaticNatEntriesRequest* request,
        dataStub::RequestStaticNatEntriesReply* response) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    template <typename T>
    bool isNatEntryAvailable(std::string subsystem, const JsonData& data,
        const T* request, int& entryIdx = 0) {
        LOG(DEBUG, __FUNCTION__);
        bool entryExists = false;

        auto bh_info = request->static_nat_entry().backhaul_type();
        int32_t profile_id, slot_id, vlan_id, backhaul;

        Json::Value entry;
        if (bh_info == ::dataStub::BackhaulPreference::PREF_WWAN) {
            backhaul = WWAN_BH_IDX;
            profile_id = request->static_nat_entry().profile_id();
            slot_id = request->static_nat_entry().slot_id();
        } else if (bh_info == ::dataStub::BackhaulPreference::PREF_ETH) {
            backhaul = ETH_BH_IDX;
            vlan_id = request->static_nat_entry().vlan_id();
        }

        int currentEntryCount = data.stateRootObj[subsystem][backhaul]["snatEntries"].size();
        auto addr = request->static_nat_entry().nat_config().address();
        auto port = request->static_nat_entry().nat_config().port();
        auto global_port = request->static_nat_entry().nat_config().global_port();
        auto proto = request->static_nat_entry().nat_config().ip_protocol();

        int index = 0;
        for (; index < currentEntryCount; index++) {
            Json::Value currentEntry = data.stateRootObj[subsystem][backhaul]["snatEntries"][index];

            if (bh_info == ::dataStub::BackhaulPreference::PREF_WWAN) {
                if (currentEntry["profileId"].asInt() != profile_id) {
                    continue;
                }

                if (currentEntry["slotId"].asInt() != slot_id) {
                    continue;
                }

            } else if (bh_info == ::dataStub::BackhaulPreference::PREF_ETH) {
                if (currentEntry["vlanId"].asInt() != vlan_id) {
                    continue;
                }
            }

            if (currentEntry["addr"].asString() != addr) {
                continue;
            }

            if (currentEntry["port"].asInt() != int(port)) {
                continue;
            }

            if (currentEntry["globalPort"].asInt() != int(global_port)) {
                continue;
            }

            if (currentEntry["proto"].asString() != proto) {
                continue;
            }
            entryIdx = index;
            entryExists = true;
            break;
        }
        return entryExists;
    }
};

#endif //NAT_MANAGER_SERVER_HPP
