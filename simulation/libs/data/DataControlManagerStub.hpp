/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef DATA_CONTROL_MANAGER_STUB_HPP
 #define DATA_CONTROL_MANAGER_STUB_HPP

#include <telux/data/DataControlManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "SimulationManagerStub.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using dataStub::DataControlManager;

namespace telux {
namespace data {

class DataControlManagerStub : public IDataControlManager,
                            public telux::common::IEventListener,
                            public SimulationManagerStub<DataControlManager>,
                            public std::enable_shared_from_this<DataControlManagerStub> {
public:

    using SimulationManagerStub::init;

    DataControlManagerStub();
    ~DataControlManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<IDataControlListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IDataControlListener> listener) override;

    telux::common::Status initSyncComplete(telux::common::ServiceStatus srvcStatus) override;

    void onEventUpdate(google::protobuf::Any event) override;

    telux::common::ErrorCode setDataStallParams(const SlotId &slotId,
            const DataStallParams &params) override;

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
    std::shared_ptr<telux::common::ListenerManager<IDataControlListener>> listenerMgr_;
    ClientEventManager &clientEventMgr_;

    void handleSSREvent(google::protobuf::Any event);
    void onServiceStatusChange(telux::common::ServiceStatus srvcStatus);
};

} // end of namespace data
} // end of namespace telux

 #endif //DATA_CONTROL_MANAGER_STUB_HPP
