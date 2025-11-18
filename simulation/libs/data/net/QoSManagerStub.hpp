/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef QOS_MANAGER_STUB_HPP
 #define QOS_MANAGER_STUB_HPP

#include <telux/data/net/QoSManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

namespace telux {
namespace data {
namespace net {

class QoSFilterImpl : public IQoSFilter {
 public:
    uint32_t getHandle() override;
    TrafficClass getTrafficClass() override;
    std::shared_ptr<ITrafficFilter> getTrafficFilter() override;
    QoSFilterStatus getStatus() override;
    std::string toString() override;

    void setHandle(uint32_t handle);
    void setTrafficClass(TrafficClass trafficClass);
    void setTrafficFilter(std::shared_ptr<ITrafficFilter> trafficFilter);
    void setStatus(QoSFilterStatus status);

 private:
    uint32_t handle_ = 0;
    TrafficClass trafficClass_ = -1;
    std::shared_ptr<ITrafficFilter> trafficFilter_ = nullptr;
    QoSFilterStatus status_ = {FilterInstallationStatus::NOT_APPLICABLE,
        FilterInstallationStatus::NOT_APPLICABLE, FilterInstallationStatus::NOT_APPLICABLE};
    std::string filterInstallationStatusToString(
        FilterInstallationStatus filterInstallationStatus);
};

class TcConfigImpl : public ITcConfig {
 public:
    TrafficClass getTrafficClass() override;
    Direction getDirection() override;
    DataPath getDataPath() override;
    BandwidthConfig getBandwidthConfig() override;
    TcConfigValidFields getTcConfigValidFields() override;
    std::string toString() override;

    void setTrafficClass(TrafficClass trafficClass);
    void setDirection(Direction direction);
    void setDataPath(DataPath dataPath);
    void setBandwidthConfig(BandwidthConfig bandwidthConfig);

 private:
    TrafficClass trafficClass_ = -1;
    Direction direction_;
    DataPath dataPath_;
    BandwidthConfig bandwidthConfig_;
    TcConfigValidFields validityMask_ = 0;
};

class QoSManagerStub : public IQoSManager {
public:
    QoSManagerStub ();
    ~QoSManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<IQoSListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IQoSListener> listener) override;

    telux::common::ErrorCode addQoSFilter(QoSFilterConfig qosFilterConfig,
        QoSFilterHandle &filterHandle, QoSFilterErrorCode &QoSFilterErrorCode) override;

    telux::common::ErrorCode getQosFilter(QoSFilterHandle filterHandle,
        std::shared_ptr<IQoSFilter> &qosFilter) override;

    telux::common::ErrorCode getQosFilters(
        std::vector<std::shared_ptr<IQoSFilter>> &qosFilter) override;

    telux::common::ErrorCode deleteQosFilter(uint32_t policyHandle) override;

    telux::common::ErrorCode deleteAllQosConfigs() override;

    telux::common::ErrorCode createTrafficClass(
        std::shared_ptr<ITcConfig> tcConfig, TcConfigErrorCode &tcConfigErrorCode) override;

    telux::common::ErrorCode getAllTrafficClasses(
        std::vector<std::shared_ptr<ITcConfig>> &tcConfigs) override;

    telux::common::ErrorCode deleteTrafficClass(std::shared_ptr<ITcConfig> tcConfig) override;

private:
    std::mutex mtx_;
    std::mutex initMtx_;

    telux::common::ServiceStatus subSystemStatus_;
    std::unique_ptr<::dataStub::QoSManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<IQoSListener>> listenerMgr_;

    void initSync(telux::common::InitResponseCb callback);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void onServiceStatusChange(ServiceStatus status);
};

} // end of namespace net
} // end of namespace data
} // end of namespace telux

 #endif //QOS_MANAGER_STUB_HPP