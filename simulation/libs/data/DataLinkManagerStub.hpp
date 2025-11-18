/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef DATA_LINK_MANAGER_STUB_HPP
 #define DATA_LINK_MANAGER_STUB_HPP

#include <telux/data/DataLinkManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "SimulationManagerStub.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using dataStub::DataLinkManager;

namespace telux {
namespace data {

class DataLinkManagerStub : public IDataLinkManager,
                            public telux::common::IEventListener,
                            public SimulationManagerStub<DataLinkManager>,
                            public std::enable_shared_from_this<DataLinkManagerStub> {
public:

    using SimulationManagerStub::init;

    DataLinkManagerStub();
    ~DataLinkManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<IDataLinkListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IDataLinkListener> listener) override;

    telux::common::Status initSyncComplete(telux::common::ServiceStatus srvcStatus) override;

    void onEventUpdate(google::protobuf::Any event) override;

    telux::common::ErrorCode setEthDataLinkState(telux::data::LinkState linkState) override;
    telux::common::Status getEthCapability(telux::data::EthCapability &ethCapability) override;
    telux::common::Status setPeerEthCapability(telux::data::EthCapability ethCapability) override;
    telux::common::Status setLocalEthOperatingMode(telux::data::EthModeType ethModeType,
            telux::common::ResponseCallback callback = nullptr);
    telux::common::Status setPeerModeChangeRequestStatus(telux::data::LinkModeChangeStatus status)
        override;

protected:
    telux::common::Status init();
    void createListener();
    void cleanup();
    void setInitCbDelay(uint32_t cbDelay);
    uint32_t getInitCbDelay();
    void notifyServiceStatus(telux::common::ServiceStatus srvcStatus);
    telux::common::Status registerDefaultIndications();

private:
    std::mutex mtx_;
    std::mutex initMtx_;

    uint32_t cbDelay_ = 0;
    telux::common::ServiceStatus subSystemStatus_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<IDataLinkListener>> listenerMgr_;
    ClientEventManager &clientEventMgr_;

    void handleSSREvent(google::protobuf::Any event);
    void handleEthDatalinkChangeEvent(google::protobuf::Any event);

    void onServiceStatusChange(telux::common::ServiceStatus status);
};

} // end of namespace data
} // end of namespace telux

 #endif //DATA_LINK_MANAGER_STUB_HPP
