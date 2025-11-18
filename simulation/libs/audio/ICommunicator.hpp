/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ICOMMUNICATOR_HPP
#define ICOMMUNICATOR_HPP

#include <cstring>
#include <unistd.h>
#include <thread>
#include <memory>
#include <vector>
#include "IAudioCallBacks.hpp"
#include "AudioDefinesLibInternal.hpp"
#include "common/CommandCallbackManager.hpp"
#include "common/TaskDispatcher.hpp"

namespace telux {
namespace audio {

class ICommunicator {
 public:
    virtual ~ICommunicator() {}

    virtual telux::common::Status setup(void) = 0;

    virtual bool isReady(void) = 0;
    virtual std::future<bool> onReady() = 0;

    /* Sending audio requests */

    virtual telux::common::Status registerForVoiceStreamEvents(uint32_t streamId,
    std::weak_ptr<telux::audio::IVoiceStreamEventsCb> listener) = 0;

    virtual telux::common::Status registerForServiceStatusEvents(
        std::weak_ptr<telux::audio::IServiceStatusEventsCb> listener) = 0;

    virtual telux::common::Status registerForPlayStreamEvents(
        std::weak_ptr<telux::audio::IPlayStreamEventsCb> listener) = 0;

    virtual telux::common::Status getDevices(
        std::shared_ptr<telux::audio::IGetDevicesCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status getStreamTypes(
        std::shared_ptr<telux::audio::IGetStreamsCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status getCalibrationInitStatus(
        std::shared_ptr<telux::audio::IGetCalInitStatusCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status createStream(telux::audio::StreamConfig streamConfig,
        std::shared_ptr<telux::audio::ICreateStreamCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status deleteStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IDeleteStreamCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status createTranscoder(telux::audio::FormatInfo inInfo,
        telux::audio::FormatInfo outInfo,
        std::shared_ptr<telux::audio::ITranscodeCreateCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status deleteTranscoder(uint32_t inStreamId, uint32_t outStreamId,
        std::shared_ptr<telux::audio::ITranscodeDeleteCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status startStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IStartStreamCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status stopStream(uint32_t streamId,
        std::shared_ptr<telux::audio::IStopStreamCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status playDtmfTone(telux::audio::DtmfTone dtmfTone, uint16_t duration,
        uint16_t gain, uint32_t streamId,
        std::shared_ptr<telux::audio::IDTMFCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status stopDtmfTone(telux::audio::StreamDirection direction,
        uint32_t streamId, std::shared_ptr<telux::audio::IDTMFCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status setDevice(uint32_t streamId,
        std::vector<telux::audio::DeviceType> devices,
        std::shared_ptr<telux::audio::ISetGetDeviceCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status getDevice(uint32_t streamId,
        std::shared_ptr<telux::audio::ISetGetDeviceCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status setVolume(uint32_t streamId, telux::audio::StreamVolume volume,
        std::shared_ptr<telux::audio::ISetGetVolumeCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status getVolume(uint32_t streamId, telux::audio::StreamDirection
        direction, std::shared_ptr<telux::audio::ISetGetVolumeCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status setMute(uint32_t streamId, telux::audio::StreamMute mute,
        std::shared_ptr<telux::audio::ISetGetMuteCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status getMute(uint32_t streamId, telux::audio::StreamDirection
        direction, std::shared_ptr<telux::audio::ISetGetMuteCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status write(uint32_t streamId, uint8_t *transportBuffer,
        uint32_t isLastBuffer, std::shared_ptr<telux::audio::IWriteCb> resultListener,
        telux::audio::AudioUserData *userData, uint32_t dataLength) = 0;

    virtual telux::common::Status read(uint32_t streamId, uint32_t numBytesToRead,
        uint8_t *transportBuffer, std::shared_ptr<telux::audio::IReadCb> resultListener,
        telux::audio::AudioUserData *audioUserData) = 0;

    virtual telux::common::Status playTone(uint32_t streamId, std::vector<uint16_t> frequency,
        uint16_t duration, uint16_t gain,
        std::shared_ptr<telux::audio::IToneCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status stopTone(uint32_t streamId,
        std::shared_ptr<telux::audio::IToneCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status flush(uint32_t streamId,
        std::shared_ptr<telux::audio::IFlushCb> resultListener, int cmdId) = 0;

    virtual telux::common::Status drain(uint32_t streamId,
        std::shared_ptr<telux::audio::IDrainCb> resultListener, int cmdId) = 0;
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // ICOMMUNICATOR_HPP
