/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef IAUDIOMSGDISPATCHER_HPP
#define IAUDIOMSGDISPATCHER_HPP

#include <iostream>
#include <memory>

#include <telux/audio/AudioDefines.hpp>

#include "AudioDefinesInternal.hpp"
#include "AudioRequest.hpp"

namespace telux {
namespace audio {

/*
 * This interface is used by audio service to send response to request made previously
 * by audio clients through transport specific communicator.
 */
class IAudioMsgDispatcher {

 public:
    virtual ~IAudioMsgDispatcher(){}

    /** Responses to request **/

    virtual void broadcastServiceStatus(uint32_t newStatus) = 0;

    virtual void sendGetSupportedDevicesResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, std::vector<DeviceType> const &devices,
        std::vector<DeviceDirection> const &devicesDirection) = 0;

    virtual void sendGetSupportedStreamTypesResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, std::vector<StreamType> const &streamTypes) = 0;

    virtual void sendCreateStreamResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId,
        StreamType streamType, uint32_t readMinSize, uint32_t writeMinSize) = 0;

    virtual void sendDeleteStreamResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendStartResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendStopResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendSetDeviceResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendGetDeviceResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId,
        std::vector<DeviceType> const &devices) = 0;

    virtual void sendSetVolumeResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendGetVolumeResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId,
        StreamDirection direction, std::vector<ChannelVolume> channelsVolume) = 0;

    virtual void sendSetMuteStateResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendGetMuteStateResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId, StreamMute muteInfo) = 0;

    virtual void sendReadResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId,
        std::shared_ptr<std::vector<uint8_t>> data, uint32_t actualReadLength,
        uint32_t offset, int64_t timeStamp, bool isIncallStream, bool isHpcmStream) = 0;

    virtual void sendWriteResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId, uint32_t actualDataLengthWritten,
        bool isIncallStream, bool isHpcmStream) = 0;

    virtual void sendStartDtmfResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendStopDtmfResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendStartToneResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendStopToneResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendDrainResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendFlushResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, uint32_t streamId) = 0;

    virtual void sendCreateTranscoderResponse(
            std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
            CreatedTranscoderInfo createdTranscoderInfo) = 0;

    virtual void sendDeleteTranscoderResponse(
        std::shared_ptr<AudioRequest> audioReq, telux::common::ErrorCode ec,
        uint32_t inStreamId, uint32_t outStreamId) = 0;

    virtual void sendGetCalibrationStatusResponse(std::shared_ptr<AudioRequest> audioRequest,
        telux::common::ErrorCode ec, CalibrationInitStatus status) = 0;

    virtual void sendDTMFDetectedEvent(int clientId,
        uint32_t streamId, uint32_t lowFreq, uint32_t highFreq,
        StreamDirection streamDirection) = 0;

    virtual void sendDrainDoneEvent( int clientId, uint32_t streamId) = 0;

    virtual void sendWriteReadyEvent(int clientId, uint32_t streamId) = 0;
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // IAUDIOMSGDISPATCHER_HPP
