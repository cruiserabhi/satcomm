/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerStub.hpp
 *
 *
 */


#ifndef SENSOR_FEATURE_MANAGER_HPP
#define SENSOR_FEATURE_MANAGER_HPP

#include <vector>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityListener.hpp>

#include "telux/sensor/SensorFeatureManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "protos/proto-src/sensor_simulation.grpc.pb.h"

using sensorStub::SensorFeatureManagerService;
using namespace telux::power;

namespace telux{
namespace sensor{
class SensorFeatureManagerStub :public ISensorFeatureManager,
                                public ITcuActivityListener,
                                public IEventListener,
                                public std::enable_shared_from_this<SensorFeatureManagerStub> {
    public:
      telux::common::Status init(telux::common::InitResponseCb initCb);
      telux::common::ServiceStatus getServiceStatus() override;
      telux::common::Status getAvailableFeatures(std::vector<SensorFeature> &features) override;
      telux::common::Status enableFeature(std::string name) override;
      telux::common::Status disableFeature(std::string name) override;
      telux::common::Status registerListener(
          std::weak_ptr<ISensorFeatureEventListener> listener) override;
      telux::common::Status deregisterListener(
          std::weak_ptr<ISensorFeatureEventListener> listener) override;
      SensorFeatureManagerStub();
      ~SensorFeatureManagerStub();
      void cleanup();
      void onEventUpdate(google::protobuf::Any event) override;
      void onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) override;

    private:
      void initSync(telux::common::InitResponseCb callback);
      void handleFeatureEvent(::sensorStub::FeatureEvent event);
      void invokeEventListener(SensorFeatureEvent event);
      void invokeBufferedEventListener(std::string sensorName,
        std::shared_ptr<std::vector<SensorEvent>> events, bool isLast);
      void parseBufferedEvent(std::string eventString,
        std::shared_ptr<std::vector<SensorEvent>> &events, std::string &sensorName);
      void initTcuPowerManager();
      bool getSystemState();
      std::unique_ptr<::sensorStub::SensorFeatureManagerService::Stub> stub_;
      std::vector<std::weak_ptr<ISensorFeatureEventListener>> listeners_;
      telux::common::AsyncTaskQueue<void> taskQ_;
      telux::common::ServiceStatus serviceStatus_;
      std::weak_ptr<telux::sensor::SensorFeatureManagerStub> myself_;
      std::mutex mutex_;
      bool isSystemSuspended_ = false;
      std::shared_ptr<ITcuActivityManager> tcuActivityMgr_;
};
}

}
#endif  // SENSOR_FEATURE_MANAGER_HPP