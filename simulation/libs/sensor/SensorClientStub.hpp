/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorClientStub.hpp
 *
 *
 */

#ifndef SENSORCLIENTSTUB_HPP
#define SENSORCLIENTSTUB_HPP

#include <string>
#include <mutex>
#include <bitset>
#include <SensorDefinesStub.hpp>
#include <telux/sensor/SensorDefines.hpp>
#include <telux/sensor/SensorClient.hpp>

#include "common/ListenerManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "libs/common/CommandCallbackManager.hpp"
#include "SensorReportListener.hpp"
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/sensor_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using sensorStub::SensorClientService;

namespace telux{
namespace sensor{

enum SelfTestFail {
    ACCEL = (1 << 0),
    GYRO = (1 << 1)
};

class SensorClientStub: public ISensorClient,
                        public IEventListener,
                        public std::enable_shared_from_this<SensorClientStub> {
  public:
    SensorClientStub(SensorInfo sensorInfo, std::shared_ptr<::sensorStub::SensorClientService::Stub> stub);
    ~SensorClientStub();
    SensorInfo getSensorInfo() override;
    telux::common::Status configure(SensorConfiguration configuration) override;
    SensorConfiguration getConfiguration() override;
    telux::common::Status activate() override;
    telux::common::Status deactivate() override;
    telux::common::Status enableLowPowerMode() override;
    telux::common::Status disableLowPowerMode() override;
    telux::common::Status selfTest(SelfTestType selfTestType, SelfTestResultCallback cb) override;
    telux::common::Status registerListener(std::weak_ptr<ISensorEventListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<ISensorEventListener> listener) override;
    telux::common::Status selfTest(SelfTestType selfTestType, SelfTestExResultCallback cb) override;
    void init();
    void onEventUpdate(google::protobuf::Any event) override;

  private:
    SensorConfiguration mergeConfiguration(SensorConfiguration requestedConfig);
    bool checkStreamingConfiguration(SensorConfiguration configuration);
    void onConfigurationUpdate(int sensorId, float samplingRate, int batchCount, bool isRotated);
    SensorConfigMask updateConfig(SensorConfiguration &configuration);
    void notifyConfigurationUpdate(SensorConfiguration configuration);
    float updateSamplingRate(float sampleRate);
    uint32_t updateBatchCount(uint32_t batchCount);
    void batchSensorEvents(std::vector<std::string> message);
    void parseRequest(::sensorStub::StartReportsEvent startEvent);
    void handleStreamingStoppedEvent();
    void handleSelfTestFailedEvent(::sensorStub::SelfTestFailedEvent &selfTestFailedEvent);
    void notifySelfTestFailedEvent();
    void notifySensorEvent(std::shared_ptr<std::vector<SensorEvent>> events);
    void parseSensorEvents(std::vector<std::vector<std::string>> events, uint32_t count);
    void updateSensorSamplingMap();
    uint64_t getSamplesToSkip(uint64_t sampleRate);
    SensorInfo sensorInfo_;
    std::string sensorLogPrefix_;
    telux::common::CommandCallbackManager cmdCallbackMgr_;
    std::shared_ptr<::sensorStub::SensorClientService::Stub> stub_;
    std::shared_ptr<telux::common::ListenerManager<ISensorEventListener>> listenerMgr_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    std::mutex mutex_;
    bool isConfigurationRequired_;
    bool sensorSessionActive_;
    SensorConfiguration config_;
    uint64_t lastReceivedEvent_;
    uint64_t lastReceivedSample_=0;
    uint64_t outgoingSampleCount_=0;
    uint64_t receivedSampleCount_=0;
    uint64_t reqTimeGap_ =0;
    std::vector<std::vector<std::string>> events_;
    std::map<uint64_t, uint64_t> sensorSamplingMap_;
    uint64_t sampleCountFromMap_;
    bool firstSample_ = true;
    std::weak_ptr<telux::sensor::SensorClientStub> myself_;
};


}

}
#endif