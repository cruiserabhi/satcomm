/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef IAUDIOBACKEND_HPP
#define IAUDIOBACKEND_HPP

#include <vector>
#include <telux/common/CommonDefines.hpp>

#include "AudioDefinesInternal.hpp"
#include "ISSREventListener.hpp"
#include "IStreamEventListener.hpp"

namespace telux {
namespace audio {

/*
 * IAudioBackend abstracts actual HAL/PAL specific telsdk implementation from
 * rest of the server code.
 */
class IAudioBackend {

 public:
    virtual ~IAudioBackend() {}

    virtual telux::common::ErrorCode init(std::shared_ptr<ISSREventListener> ssrEventListener) = 0;

    virtual telux::common::ErrorCode deinit() = 0;

    virtual telux::common::ErrorCode getSupportedDevices(std::vector<DeviceType> &devices,
        std::vector<DeviceDirection> &devicesDirection) = 0;

    virtual telux::common::ErrorCode getSupportedStreamTypes(
        std::vector<StreamType> &streamTypes) = 0;

    virtual telux::common::ErrorCode createStream(StreamHandle &streamHandle,
        StreamParams streamParams, uint32_t& readBufferMinSize,
        uint32_t &writeBufferMinSize) = 0;

    virtual telux::common::ErrorCode deleteStream(StreamHandle &streamHandle) = 0;

    virtual telux::common::ErrorCode start(StreamHandle streamHandle) = 0;

    virtual telux::common::ErrorCode stop(StreamHandle streamHandle) = 0;

    virtual telux::common::ErrorCode setDevice(StreamHandle streamHandle,
        std::vector<DeviceType> &deviceTypes) = 0;

    virtual telux::common::ErrorCode getDevice(StreamHandle streamHandle,
        std::vector<DeviceType> &deviceTypes) = 0;

    virtual telux::common::ErrorCode setVolume(StreamHandle streamHandle,
        StreamDirection direction, std::vector<ChannelVolume> channelsVolume) = 0;

    virtual telux::common::ErrorCode getVolume(StreamHandle streamHandle,
        int channelTypeMask, std::vector<ChannelVolume> &channelsVolume) = 0;

    virtual telux::common::ErrorCode setMuteState(StreamHandle streamHandle,
        StreamMute muteInfo, std::vector<ChannelVolume> channelsVolume, bool prevMuteState) = 0;

    virtual telux::common::ErrorCode getMuteState(StreamHandle streamHandle,
        StreamMute &muteInfo, StreamDirection direction) = 0;

    virtual telux::common::ErrorCode write(StreamHandle &streamHandle,
        uint8_t *data, uint32_t writeLengthRequested,
        uint32_t offset, int64_t timeStamp, bool isLastBuffer,
        int64_t &actualLengthWritten) = 0;

    virtual telux::common::ErrorCode read(StreamHandle &streamHandle,
        std::shared_ptr<std::vector<uint8_t>> data, uint32_t readLengthRequested,
        int64_t &actualReadLength) = 0;

    virtual telux::common::ErrorCode drain(StreamHandle streamHandle) = 0;

    virtual telux::common::ErrorCode flush(StreamHandle streamHandle) = 0;

    virtual telux::common::ErrorCode startDtmf(StreamHandle streamHandle,
        uint16_t gain, uint16_t duration, DtmfTone dtmfTone) = 0;

    virtual telux::common::ErrorCode stopDtmf(StreamHandle streamHandle,
        StreamDirection direction) = 0;

    virtual telux::common::ErrorCode registerDTMFDetection(StreamHandle streamHandle) = 0;

    virtual telux::common::ErrorCode deRegisterDTMFDetection(StreamHandle streamHandle) = 0;

    virtual telux::common::ErrorCode setupInTranscodeStream(StreamHandle &streamHandle,
        uint32_t streamId, TranscodingFormatInfo inInfo,
        std::shared_ptr<IStreamEventListener> streamEventListener,
        uint32_t &writeMinSize) = 0;

    virtual telux::common::ErrorCode setupOutTranscodeStream(StreamHandle &streamHandle,
        uint32_t streamId, TranscodingFormatInfo outInfo,
        std::shared_ptr<IStreamEventListener> streamEventListener,
        uint32_t &readMinSize) = 0;

    virtual telux::common::ErrorCode startTone(StreamHandle &streamHandle, uint32_t sampleRate,
        uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequency) = 0;

    virtual telux::common::ErrorCode stopTone(StreamHandle &streamHandle) = 0;

    virtual telux::common::ErrorCode getCalibrationStatus(CalibrationInitStatus &status) = 0;
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // IAUDIOBACKEND_HPP
