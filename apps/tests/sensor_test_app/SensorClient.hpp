/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SENSORCLIENT_HPP
#define SENSORCLIENT_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "SensorUtils.hpp"
#include <telux/sensor/Sensor.hpp>
#include <telux/sensor/SensorDefines.hpp>

using namespace telux::sensor;
using namespace telux::common;

class SensorClient : public ISensorEventListener,
                     public std::enable_shared_from_this<SensorClient> {
 public:
    SensorClient(
        int id, std::shared_ptr<ISensorClient> sensor, SensorTestAppArguments commandLineArgs);
    ~SensorClient();
    void init();
    void cleanup();
    void printInfo();
    virtual void onEvent(std::shared_ptr<std::vector<SensorEvent>> events) override;
    virtual void onConfigurationUpdate(SensorConfiguration configuration) override;
    virtual void onSelfTestFailed() override;
    telux::common::Status configure(SensorConfiguration config);
    telux::common::Status activate();
    telux::common::Status deactivate();
    void enableLowPowerMode();
    void disableLowPowerMode();
    telux::common::Status selfTest(SelfTestType selfTestType);
    telux::common::Status selfTestEx(SelfTestType selfTestType);
    std::shared_ptr<ISensorClient> getSensorClient() const {
        return sensor_;
    }
    bool isActive() const {
        return activated_;
    }
    void setRecordingFlag(bool enable);
    const int id_;

 private:
    std::mutex mtx_;
    std::shared_ptr<ISensorClient> sensor_;
    std::string tag_;
    uint64_t lastBatchReceivedAt_;
    uint32_t totalEvents_;
    bool stop_;
    bool activated_;
    std::mutex qMutex_;
    std::condition_variable cv_;
    std::shared_ptr<std::thread> workerThread_;
    // Structure instance to store the command line args passed
    SensorTestAppArguments commandLineArgs_;
    bool isRecordingEnabled_ = false;
};

#endif  // SENSORCLIENT_HPP
