/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STREAM_HPP
#define STREAM_HPP

#include <memory>
#include <cstring>

#include "libs/common/TaskDispatcher.hpp"
#include "TransportDefines.hpp"
#include "IStreamEventListener.hpp"
#include "ClientCache.hpp"
#include "IAudioBackend.hpp"
#include "IAudioMsgDispatcher.hpp"

namespace telux {
namespace audio {

/*
 * Represents an audio stream from audio service's point of view.
 */
class Stream : public IStreamEventListener,
               public std::enable_shared_from_this<Stream> {

 public:
    Stream(std::shared_ptr<IAudioBackend> audioBackend,
        std::shared_ptr<ClientCache> clientCache_);
    ~Stream();

    telux::common::ErrorCode setupStream(StreamConfiguration config,
        uint32_t streamId, uint32_t& readMinSize, uint32_t& writeMinSize);

    telux::common::ErrorCode setupInTranscodeStream(TranscodingFormatInfo inInfo,
        CreatedTranscoderInfo *createdTranscoderInfo);

    telux::common::ErrorCode setupOutTranscodeStream(TranscodingFormatInfo outInfo,
        CreatedTranscoderInfo *createdTranscoderInfo);

    telux::common::ErrorCode cleanupStream(std::vector<int>& voiceCallList);

    void start(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void stop(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void setDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            std::vector<DeviceType> const &devices);

    void getDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void setVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction, std::vector<ChannelVolume> channelsVolume);

    void getVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction);

    void setMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamMute muteInfo);

    void getMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction);

    void startDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint16_t gain, uint16_t duration, DtmfTone dtmfTone);

    void stopDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction);

    void write(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint8_t *data, uint32_t dataLength, uint32_t offset, int64_t timeStamp,
            bool isLastBuffer, std::vector<int> &voiceCallList);

    void read(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        uint32_t readLengthRequested, std::vector<int> &voiceCallList);

    void startTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequencies);

    void stopTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void flush(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void drain(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    /* IStreamEventListener overrides */

    void onWriteReadyEvent(uint32_t streamId) override;

    void onDrainDoneEvent(uint32_t streamId);

    void onDTMFDetectedEvent(uint32_t streamId, uint32_t lowFreq,
            uint32_t highFreq, StreamDirection streamDirection);

 private:
    bool isIncallStream = false;
    bool isHpcmStream = false;
    /* No. of buffers in the pipeline to play. */
    int pipelineLength = 0;
    /* Keep track of buffers played. When this no. becomes a multiple of maxPipeLineLen, then send a
       send a pipeline full notification to simulate the notifications for compressed playback. */
    int sendPipelineFull = 0;
    /* Max no. of bufffers after which pipeline full notification is sent. */
    int maxPipeLineLen = 0;
    bool isBtStream = false;
    std::shared_ptr<std::vector<uint8_t>> buffer_;
    StreamHandle streamHandle_;
    StreamParams streamParams_;
    std::shared_ptr<IAudioBackend> audioBackend_;
    std::shared_ptr<ClientCache> clientCache_;
    std::unique_ptr<telux::common::TaskDispatcher> streamTaskExecutor_;

    void doStart(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void doStop(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void doSetDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            std::vector<DeviceType> devices);

    void doGetDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void doSetVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction, std::vector<ChannelVolume> channelsVolume);

    void doGetVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction);

    void doGetMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction);

    void doSetMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamMute muteInfo);

    void doRead(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint32_t readLengthRequested, std::vector<int> voiceCallList);

    void doWrite(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint32_t dataLength, uint32_t offset, int64_t timeStamp, bool isLastBuffer,
            uint8_t *data, std::vector<int> voiceCallList);

    void doStartDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint16_t gain, uint16_t duration, DtmfTone dtmfTone);

    void doStopDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction);

    void doStartTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequencies);

    void doStopTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void doDrain(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void doFlush(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId);

    void doRegisterForIndication(std::shared_ptr<AudioRequest> audioReq,
            uint32_t streamId, uint32_t indicationType);

    void doDeRegisterForIndication(std::shared_ptr<AudioRequest> audioReq,
            uint32_t streamId, uint32_t indicationType);

    void doOnWriteReadyEvent(uint32_t streamId);

    void doOnDrainDoneEvent(uint32_t streamId);

    void doOnDTMFDetectedEvent(uint32_t streamId, uint32_t lowFreq,
        uint32_t highFreq, StreamDirection streamDirection);
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // STREAM_HPP