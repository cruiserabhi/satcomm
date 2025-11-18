/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef IAUDIOMSGLISTENER_HPP
#define IAUDIOMSGLISTENER_HPP

#include "AudioClient.hpp"
#include "AudioRequest.hpp"

namespace telux {
namespace audio {

/*
 * This interface is used by transport specific communicator to pass messages
 * from audio clients to audio service.
 */
class IAudioMsgListener {

public:
    virtual telux::common::Status onClientConnected(
        std::shared_ptr<AudioClient> audioClient,
        std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher) = 0;

    virtual telux::common::Status onClientDisconnected(
        std::shared_ptr<AudioClient> audioClient) = 0;

    virtual void getSupportedDevices(std::shared_ptr<AudioRequest> audioRequest) = 0;

    virtual void getSupportedStreamTypes(std::shared_ptr<AudioRequest> audioRequest) = 0;

    virtual void getCalibrationStatus(std::shared_ptr<AudioRequest> audioRequest) = 0;

    virtual void createStream(std::shared_ptr<AudioRequest> audioRequest,
        StreamConfiguration config) = 0;

    virtual void deleteStream(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId) = 0;

    virtual void start(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId) = 0;

    virtual void stop(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId) = 0;

    virtual void setDevice(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, std::vector<DeviceType> const &devices) = 0;

    virtual void getDevice(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId) = 0;

    virtual void setVolume(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, StreamDirection direction,
        std::vector<ChannelVolume> channelsVolume) = 0;

    virtual void getVolume(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, StreamDirection direction) = 0;

    virtual void setMuteState(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, StreamMute muteInfo) = 0;

    virtual void getMuteState(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, StreamDirection direction) = 0;

    virtual void write(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, uint8_t *data, uint32_t dataLength,
        uint32_t offset, int64_t timeStamp, bool isLastBuffer) = 0;

    virtual void read(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, uint32_t readLengthRequested) = 0;

    virtual void startDtmf(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, uint16_t gain, uint16_t duration, DtmfTone dtmfTone) = 0;

    virtual void stopDtmf(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, StreamDirection direction) = 0;

    virtual void startTone(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, uint16_t gain, uint16_t duration,
        std::vector<uint16_t> toneFrequencies) = 0;

    virtual void stopTone(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId) = 0;

    virtual void drain(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId) = 0;

    virtual void flush(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId) = 0;

    virtual void registerForIndication(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, uint32_t indicationType) = 0;

    virtual void deRegisterForIndication(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t streamId, uint32_t indicationType) = 0;

    virtual void createTranscoder(std::shared_ptr<AudioRequest> audioRequest,
        TranscodingFormatInfo inInfo, TranscodingFormatInfo outInfo) = 0;

    virtual void deleteTranscoder(std::shared_ptr<AudioRequest> audioRequest,
        uint32_t inStreamId, uint32_t outStreamId) = 0;

    virtual bool isSSRInProgress(void) = 0;
};

} // end namespace audio
} // end namespace telux

#endif  // IAUDIOMSGLISTENER_HPP
