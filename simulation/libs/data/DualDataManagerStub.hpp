/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef DUAL_DATA_MANAGER_STUB_HPP
 #define DUAL_DATA_MANAGER_STUB_HPP

#include <telux/data/DualDataManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {

class DualDataManagerStub : public IDualDataManager,
                            public telux::common::IEventListener,
                            public std::enable_shared_from_this<DualDataManagerStub> {
public:
    DualDataManagerStub();
    ~DualDataManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<IDualDataListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IDualDataListener> listener) override;

    // API to get dual data capability.
    telux::common::ErrorCode getDualDataCapability(bool &isCapable) override;

    // API to get dual data usage recommendation.
    telux::common::ErrorCode getDualDataUsageRecommendation(
        DualDataUsageRecommendation &recommendation) override;

    // API to request DDS switch
    telux::common::Status requestDdsSwitch(DdsInfo request,
        telux::common::ResponseCallback callback = nullptr) override;

    // API to request current DDS
    telux::common::Status requestCurrentDds(RequestCurrentDdsRespCb callback) override;

    // API to configure DDS switch recommendation
    telux::common::ErrorCode configureDdsSwitchRecommendation(
        const DdsSwitchRecommendationConfig recommendationConfig) override;

    // API to get current DDS switch recommendation
    telux::common::ErrorCode getDdsSwitchRecommendation(
        DdsSwitchRecommendation &ddsSwitchRecommendation) override;

    void onEventUpdate(google::protobuf::Any event) override;
    void handleCapabilityChangeEvent(::dataStub::DualDataCapabilityEvent capabilityEvent);
    void handleRecommendationChangeEvent(
        ::dataStub::DualDataUsageRecommendationEvent recommendationEvent);

private:
    std::mutex mtx_;
    std::mutex initMtx_;

    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::DualDataManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<IDualDataListener>> listenerMgr_;

    void initSync(telux::common::InitResponseCb callback);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void onServiceStatusChange(telux::common::ServiceStatus status);
};

} // end of namespace data
} // end of namespace telux

 #endif //DUAL_DATA_MANAGER_STUB_HPP