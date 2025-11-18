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
 * @file       SensorManagerStub.hpp
 *
 *
 */
#ifndef SENSORMANAGERSTUB_HPP
#define SENSORMANAGERSTUB_HPP

#include <telux/sensor/SensorManager.hpp>
#include <telux/sensor/SensorClient.hpp>
#include <telux/sensor/SensorDefines.hpp>
#include <grpcpp/grpcpp.h>
#include <map>

#include "SensorClientStub.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "libs/common/CommandCallbackManager.hpp"
#include "protos/proto-src/sensor_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using sensorStub::SensorClientService;



namespace telux{

namespace sensor{

class SensorManagerStub : public ISensorManager,
                          public std::enable_shared_from_this<SensorManagerStub> {
public:

    telux::common::Status init(telux::common::InitResponseCb initCb);

/**
 * This status indicates whether the object is in a usable state.
 *
 * returns SERVICE_AVAILABLE    -  If sensor client is ready for service.
 *          SERVICE_UNAVAILABLE  -  If sensor client is temporarily unavailable.
 *          SERVICE_FAILED       -  If sensor client encountered an irrecoverable failure.
 */
    telux::common::ServiceStatus getServiceStatus() override;

/**
 * This API retrieves the available sensors information.
 *
 * param[in] info - vector of SensorInfo to get details of available sensors
 *
 * returns Status of getAvailableSensorInfo i.e success or suitable status code.
 *
 */
    telux::common::Status getAvailableSensorInfo(std::vector<SensorInfo> &info) override;
    telux::common::Status getSensor(
    std::shared_ptr<ISensorClient> &sensor, std::string name) override;
    telux::common::Status getSensorClient(
    std::shared_ptr<ISensorClient> &sensor, std::string name) override;
    telux::common::Status setEulerAngleConfig(EulerAngleConfig eulerAngleConfig) override;
    SensorManagerStub();
    ~SensorManagerStub();
    void cleanup();

private:
  void updateSensorInfo();
  void initSync(telux::common::InitResponseCb callback);
  void setServiceStatus(telux::common::ServiceStatus status);
  telux::common::Status apiJsonReader(std::string apiName);
  SensorInfo &getSensorInfo(std::string name);
  telux::common::ServiceStatus serviceStatus_;
  std::mutex serviceStatusMutex_;
  telux::common::InitResponseCb initCb_;
  telux::common::AsyncTaskQueue<void> taskQ_;
  std::vector<SensorInfo> sensorInfo_;
  std::shared_ptr<::sensorStub::SensorClientService::Stub> stub_;
};


}

}
#endif  // SENSORMANAGERSTUB_HPP