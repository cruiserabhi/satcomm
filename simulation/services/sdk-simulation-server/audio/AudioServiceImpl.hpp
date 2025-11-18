/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOSERVICEIMPL_HPP
#define AUDIOSERVICEIMPL_HPP

#include "AudioDefinesInternal.hpp"
#include "ISSREventListener.hpp"
#include "StreamCache.hpp"
#include "ClientCache.hpp"
#include "IAudioMsgListener.hpp"
#include "Alsa.hpp"


namespace telux {
namespace audio {

/*
 * Represents audio service, responsible for business logic.
 */
class AudioServiceImpl : public IAudioMsgListener,
                         public ISSREventListener,
                         public std::enable_shared_from_this<AudioServiceImpl> {
 public:
    AudioServiceImpl();
    telux::common::Status initService(void);
    ~AudioServiceImpl();

    /*** Overrides - ISSREventListener ***/

    void onSSREvent(SSREvent event) override;

    /*** Overrides - IAudioMsgListener ***/

    telux::common::Status onClientConnected(
        std::shared_ptr<AudioClient> audioClient,
        std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher) override;

    telux::common::Status onClientDisconnected(
        std::shared_ptr<AudioClient> audioClient) override;

    void getSupportedDevices(std::shared_ptr<AudioRequest> audioReq) override;

    void getSupportedStreamTypes(std::shared_ptr<AudioRequest> audioReq) override;

    void getCalibrationStatus(std::shared_ptr<AudioRequest> audioReq) override;

    void createStream(std::shared_ptr<AudioRequest> audioReq,
        StreamConfiguration config) override;

    void deleteStream(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) override;

    void start(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) override;

    void stop(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) override;

    void setDevice(std::shared_ptr<AudioRequest> audioReq,
            uint32_t streamId, std::vector<DeviceType> const &devices) override;

    void getDevice(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) override;

    void setVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction, std::vector<ChannelVolume> channelsVolume) override;

    void getVolume(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction) override;

    void setMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamMute muteInfo) override;

    void getMuteState(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction) override;

    void write(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint8_t *data, uint32_t writeLengthRequested, uint32_t offset,
            int64_t timeStamp, bool isLastBuffer) override;

    void read(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint32_t readLengthRequested) override;

    void startDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint16_t gain, uint16_t duration, DtmfTone dtmfTone) override;

    void stopDtmf(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            StreamDirection direction) override;

    void startTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequencies) override;

    void stopTone(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) override;

    void drain(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) override;

    void flush(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId) override;

    void registerForIndication(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint32_t indicationType) override;

    void deRegisterForIndication(std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
            uint32_t indicationType) override;

    void createTranscoder(std::shared_ptr<AudioRequest> audioReq,
        TranscodingFormatInfo inInfo, TranscodingFormatInfo outInfo) override;

    void deleteTranscoder(std::shared_ptr<AudioRequest> audioReq,
        uint32_t inStreamId, uint32_t outStreamId) override;

    bool isSSRInProgress(void) override;

    std::shared_ptr<ClientCache> getClientCache();

 private:
    /*** Helper routines ***/

    void handleSSREvent(SSREvent event);

    void handleClientDisconnect(std::shared_ptr<AudioClient> audioClient);

    void handleClientConnect(std::shared_ptr<AudioClient> audioClient);

    void doGetSupportedDevices(std::shared_ptr<AudioRequest> audioReq);

    void doGetSupportedStreamTypes(std::shared_ptr<AudioRequest> audioReq);

    void doGetCalibrationStatus(std::shared_ptr<AudioRequest> audioReq);

    telux::common::ErrorCode doCreateStream(
        std::shared_ptr<AudioRequest> audioReq, StreamConfiguration config,
        TranscodingFormatInfo inInfo, TranscodingFormatInfo outInfo,
        StreamPurpose streamPurpose, CreatedTranscoderInfo *createdTranscoderInfo);

    telux::common::ErrorCode doDeleteStream(
        std::shared_ptr<AudioRequest> audioReq, uint32_t streamId,
        bool sendResponse);

    void doCreateTranscoder(std::shared_ptr<AudioRequest> audioReq,
        TranscodingFormatInfo inInfo, TranscodingFormatInfo outInfo);

    void doDeleteTranscoder(std::shared_ptr<AudioRequest> audioReq,
        uint32_t inStreamId, uint32_t outStreamId);

    /*
     * Audio service-wide flag to indicate we are currently undergoing SSR therefore
     * appropriate actions needs to be taken. For ex; stop executing new request
     * from audio applicatons. This flag is set/reset only from audio service. Rest
     * all places it is just referred to know current state.
     *
     * Purpose of ssrInProgress_ is to influence what to do when a SSR state-update is
     * detected. Value 'true' represents SSR has occurred, 'false' represents we have
     * overcome SSR.
     */
    std::atomic<bool> ssrInProgress_{false};

    std::shared_ptr<StreamCache> streamCache_;

    std::shared_ptr<ClientCache> clientCache_;

    std::weak_ptr<IAudioMsgDispatcher> audioMsgDispatcher_;

    /*
     * AudioServiceImpl holds a shared pointer to concrete implementation of
     * IAudioBacked interface by HAL or PAL classes. The HAL/PAL holds a weak pointer
     * to AudioServiceImpl. This breaks circular dependency between these two
     * classes.
     */
    std::shared_ptr<IAudioBackend> audioBackend_;

    /*
     * Creation of stream and handling SSR is done on the same thread using this
     * dispatcher. This ensures when doing cleanup, server has a consistent
     * view of resources allocated (for ex; either a stream exist or it doesn't).
     *
     * This also executes tasks which are not associated with any stream like get
     * supported audio devices, get calibration status etc.
     */
    std::unique_ptr<telux::common::TaskDispatcher> serviceCommonTaskExecutor_;
};

} // end namespace audio
} // end namespace telux

#endif  // AUDIOSERVICEIMPL_HPP
