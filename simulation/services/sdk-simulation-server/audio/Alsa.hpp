/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ALSA_HPP
#define ALSA_HPP

#include <unordered_map>
#include <thread>
extern "C" {
    #include <alsa/asoundlib.h>
}

#include "IAudioBackend.hpp"
#include "libs/common/Logger.hpp"
#include "TransportDefines.hpp"
#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/common/event-manager/EventManager.hpp"

#define FILTER_ORDER 2

namespace telux {
namespace audio {

/*
 * When mapping audio devices, specifies the parameter mapped.
 */
enum MappedValueType {
    DEVICE_TYPE,
    DEVICE_DIR,
    Alsa_DEVICE_TYPE
};

/*
 * Represents cookie exchanged between telsdk and Alsa:: to process
 * SSR event.
 */
struct PrivateSSRData {
    std::weak_ptr<ISSREventListener> ssrEventListener;
};

struct DeviceMappingTable {
    /* <Number of device types supported> */
    uint32_t numDevices;
    /* <Direction of the device> */
    DeviceDirection deviceDir[12];
    /* <Device type as defined by telsdk> */
    DeviceType deviceType[12];
};

class Alsa : public IAudioBackend,
             public IServerEventListener,
             public std::enable_shared_from_this<Alsa> {

 public:
    Alsa();
    ~Alsa();

    telux::common::ErrorCode init(std::shared_ptr<ISSREventListener> ssrEventListener) override;
    telux::common::ErrorCode deinit() override;

    telux::common::ErrorCode getSupportedDevices(std::vector<DeviceType> &devices,
            std::vector<DeviceDirection> &devicesDirection) override;

    telux::common::ErrorCode getSupportedStreamTypes(
            std::vector<StreamType> &streamTypes) override;

    telux::common::ErrorCode createStream(StreamHandle &streamHandle,
        StreamParams streamParams, uint32_t &readBufferMinSize,
        uint32_t &writeBufferMinSize) override;

    telux::common::ErrorCode deleteStream(StreamHandle &streamHandle) override;

    telux::common::ErrorCode start(StreamHandle streamHandle) override;

    telux::common::ErrorCode stop(StreamHandle streamHandle) override;

    telux::common::ErrorCode setDevice(StreamHandle streamHandle,
        std::vector<DeviceType> &deviceTypes) override;

    telux::common::ErrorCode getDevice(StreamHandle streamHandle,
        std::vector<DeviceType> &deviceTypes) override;

    telux::common::ErrorCode setVolume(StreamHandle streamHandle,
        StreamDirection direction, std::vector<ChannelVolume> channelsVolume) override;

    telux::common::ErrorCode getVolume(StreamHandle streamHandle,
        int channelTypeMask, std::vector<ChannelVolume> &channelsVolume) override;

    telux::common::ErrorCode setMuteState(StreamHandle streamHandle,
        StreamMute muteInfo, std::vector<ChannelVolume> channelsVolume,
        bool prevMuteState) override;

    telux::common::ErrorCode getMuteState(StreamHandle streamHandle,
        StreamMute &muteInfo, StreamDirection direction) override;

    telux::common::ErrorCode write(StreamHandle &streamHandle,
       uint8_t *data, uint32_t writeLengthRequested,
        uint32_t offset, int64_t timeStamp, bool isLastBuffer,
        int64_t &actualLengthWritten) override;

    telux::common::ErrorCode read(StreamHandle &streamHandle,
        std::shared_ptr<std::vector<uint8_t>> data, uint32_t readLengthRequested,
        int64_t &actualReadLength) override;

    telux::common::ErrorCode drain(StreamHandle streamHandle) override;

    telux::common::ErrorCode flush(StreamHandle streamHandle) override;

    telux::common::ErrorCode startDtmf(StreamHandle streamHandle,
        uint16_t gain, uint16_t duration, DtmfTone dtmfTone) override;

    telux::common::ErrorCode stopDtmf(StreamHandle streamHandle,
        StreamDirection direction) override;

    telux::common::ErrorCode registerDTMFDetection(StreamHandle streamHandle) override;

    telux::common::ErrorCode deRegisterDTMFDetection(StreamHandle streamHandle) override;

    telux::common::ErrorCode setupInTranscodeStream(StreamHandle &streamHandle,
        uint32_t streamId, TranscodingFormatInfo inInfo,
        std::shared_ptr<IStreamEventListener> streamEventListener,
        uint32_t& writeMinSize) override;

    telux::common::ErrorCode setupOutTranscodeStream(StreamHandle &streamHandle,
        uint32_t streamId, TranscodingFormatInfo outInfo,
        std::shared_ptr<IStreamEventListener> streamEventListener,
        uint32_t& readMinSize) override;

    telux::common::ErrorCode startTone(StreamHandle &streamHandle, uint32_t sampleRate,
        uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequency) override;

    telux::common::ErrorCode stopTone(StreamHandle &streamHandle) override;

    telux::common::ErrorCode getCalibrationStatus(CalibrationInitStatus &status) override;

    /*** Overrides - IServerEventListener ***/
    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

private:
    static const std::vector<StreamType> SUPPORTED_STREAM_TYPES;
    static const DeviceMappingTable DEFAULT_DEVS_TABLE;

    PrivateSSRData *privateSSRData_;
    DeviceMappingTable finalDevicesTable_{};
    std::vector<std::thread> runningThreads_;
    std::shared_ptr<SimulationConfigParser> config_;
    std::unordered_map<uint32_t, std::shared_ptr<IStreamEventListener>> dtmfIndicationListenerMap_;
    std::vector<std::shared_ptr<ISSREventListener>> ssrListenerMap_;
    bool isBTScoEnabled_ = false;
    std::atomic<bool> runLoopback_;
    std::atomic<bool> runTone_;
    std::vector<std::thread> toneThread_, loopThread_;
    float RegFreq1_[FILTER_ORDER]={1,0};
    float InFreq1_;
    float RegFreq2_[FILTER_ORDER]={1,0};
    float InFreq2_;
    std::string pcmDevice_;
    std::string sndCardCtlDevice_;
    uint32_t inTranscodeStreamId_ = INT_MAX;
    uint32_t outTranscodeStreamId_ = INT_MAX;
    int sendWriteReady;
    int pipelineLen;

    int loadMappingArray(std::string key, MappedValueType mappedValueType,
        uint32_t numOfValues, DeviceMappingTable& deviceTbl);
    int loadUserDeviceMapping(void);
    telux::common::ErrorCode mapStreamType(StreamType streamType, snd_pcm_stream_t& stream);
    telux::common::ErrorCode mapStreamChannelMask( uint32_t channelTypeMask, int& channels);
    telux::common::ErrorCode setBufferSize(StreamHandle streamHandle,
        size_t& inSize, size_t& outSize);
    telux::common::ErrorCode startLoopback(snd_pcm_t *captureHandle, snd_pcm_t *playHandle,
        int channels);
    telux::common::ErrorCode generateTone(StreamHandle streamHandle, uint32_t sampleRate,
    uint16_t gain, uint16_t duration, std::vector<uint16_t> toneFrequency);
    void genTone(std::vector<uint16_t> toneFrequency, int channels, uint32_t sampleRate,
        uint16_t gain, float buf[]);
    float generateSignal(float t1, float t2);
    void onEventUpdate(std::string event);
    void handleDTMFDetectedEvent(std::string eventParams);
    void handleSSREvent(std::string eventParams);
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // ALSA_HPP
