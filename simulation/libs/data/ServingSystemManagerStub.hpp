/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef SERVING_SYSTEM_MANAGER_STUB_HPP
 #define SERVING_SYSTEM_MANAGER_STUB_HPP

#include <telux/data/ServingSystemManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {

class ServingSystemManagerStub : public IServingSystemManager,
                                 public IServingSystemListener {
public:
    ServingSystemManagerStub (SlotId slotId);
    ~ServingSystemManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    DrbStatus getDrbStatus() override;

    telux::common::Status requestServiceStatus(RequestServiceStatusResponseCb callback) override;

    telux::common::Status requestRoamingStatus(RequestRoamingStatusResponseCb callback) override;

    telux::common::Status makeDormant(
       telux::common::ResponseCallback callback) override;

    telux::common::Status requestNrIconType(RequestNrIconTypeResponseCb callback) override;
    SlotId getSlotId() override;

    telux::common::Status registerListener(std::weak_ptr<IServingSystemListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IServingSystemListener> listener) override;

    void onRoamingStatusChanged(RoamingStatus status);
    void onNrIconTypeChanged(NrIconType type);
    void onServiceStateChangeInd(ServiceStatus status);
    void onDrbStatusChanged(DrbStatus status);

private:
    std::mutex mtx_;
    std::mutex initMtx_;
    std::mutex mutex_;

    SlotId slotId_ = DEFAULT_SLOT_ID;
    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::DataServingSystemManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::vector<std::weak_ptr<IServingSystemListener>> listeners_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<IServingSystemListener>> listenerMgr_;

    void initSync(telux::common::InitResponseCb callback);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay );
};

} // end of namespace data
} // end of namespace telux

 #endif //SERVING_SYSTEM_MANAGER_STUB_HPP