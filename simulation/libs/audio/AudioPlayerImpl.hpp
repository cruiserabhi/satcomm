/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AudioPlayerImpl_HPP
#define AudioPlayerImpl_HPP

#include <queue>
#include <cstdio>
#include <mutex>

#include "common/AsyncTaskQueue.hpp"

#include <telux/audio/AudioPlayer.hpp>
#include <telux/audio/AudioListener.hpp>
#include <telux/audio/AudioManager.hpp>

namespace telux {
namespace audio {

class GetVolumeResponseListener {
 public:
    GetVolumeResponseListener(std::mutex &streamMtx);
    bool responseReady = false;
    float volumeLevel;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void getVolumeComplete(StreamVolume volume, telux::common::ErrorCode errorCode);

 private:
    std::mutex &streamMutex_;
};

class SetVolumeResponseListener {
 public:
    SetVolumeResponseListener(std::mutex &streamMtx);
    bool responseReady = false;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void setVolumeComplete(telux::common::ErrorCode errorCode);

 private:
    std::mutex &streamMutex_;
};

class GetMuteResponseListener {
 public:
    GetMuteResponseListener(std::mutex &streamMtx);
    bool responseReady = false;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void getMuteComplete(StreamMute mute, telux::common::ErrorCode errorCode);

 private:
    std::mutex &streamMutex_;
};

class SetMuteResponseListener {
 public:
    SetMuteResponseListener(std::mutex &streamMtx);
    bool responseReady = false;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void setMuteComplete(telux::common::ErrorCode errorCode);

 private:
    std::mutex &streamMutex_;
};

class GetDeviceResponseListener {
 public:
    GetDeviceResponseListener(std::mutex &streamMtx);
    bool responseReady = false;
    std::vector<DeviceType> devices;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void getDeviceComplete(std::vector<DeviceType> devices, telux::common::ErrorCode errorCode);

 private:
    std::mutex &streamMutex_;
};

class SetDeviceResponseListener {
 public:
    SetDeviceResponseListener(std::mutex &streamMtx);
    bool responseReady = false;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void setDeviceComplete(telux::common::ErrorCode errorCode);

 private:
    std::mutex &streamMutex_;
};

class AudioPlayerImpl : public IAudioPlayer,
                        public IPlayListener,
                        public IAudioListener,
                        public std::enable_shared_from_this<AudioPlayerImpl> {

 public:
    AudioPlayerImpl(std::shared_ptr<IAudioManager> audioManager);
    ~AudioPlayerImpl();

    telux::common::ErrorCode startPlayback(std::vector<PlaybackConfig> &playbackConfigs,
        std::weak_ptr<IPlayListListener> statusListener) override;

    telux::common::ErrorCode stopPlayback() override;

    telux::common::ErrorCode setVolume(float volumeLevel) override;
    telux::common::ErrorCode getVolume(float &volumeLevel) override;

    telux::common::ErrorCode setMute(bool enable) override;
    telux::common::ErrorCode getMute(bool &enable) override;

    telux::common::ErrorCode setDevice(std::vector<DeviceType> devices) override;
    telux::common::ErrorCode getDevice(std::vector<DeviceType> &devices) override;

    void onReadyForWrite() override;
    void onPlayStopped() override;
    void onServiceStatusChange(telux::common::ServiceStatus status) override;

    void executePlayback();

    void createStreamComplete(
        std::shared_ptr<IAudioStream> &stream, telux::common::ErrorCode result);

    void deleteStreamComplete(telux::common::ErrorCode result);

    void writeComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytesWritten,
        telux::common::ErrorCode error);

    void stopAudioComplete(telux::common::ErrorCode result);

    WriteResponseCb writeCompleteCb_;

 private:
    /* Number of buffers used for playback */
    const size_t BUFFER_POOL_SIZE = 2;

    /* Time in seconds for which player thread waits for the response from audio server */
    const int32_t TIME_10_SECONDS = 10;

    bool isCompressed_          = false;
    bool isAdspWriteReady_      = true;
    bool hasSsrOccurred_        = false;
    bool isFileOpened_          = false;
    bool isStreamOpened_        = false;
    bool isPlayInProgress_      = false;
    bool hasUserRequestedStop_  = false;
    bool isCreateResponseReady_ = false;
    bool isDeleteResponseReady_ = false;
    bool isStopResponseReady_   = false;
    bool isStopAudioReady_      = false;
    bool buffersAllocated_      = false;
    bool applyCachedVolume_     = false;
    bool applyCachedMute_       = false;
    bool isStreamMuted_         = false;
    uint32_t bufferSize_        = 0;
    long contentOffset_         = 0;
    ChannelTypeMask curChannelTypeMask_;

    float cachedVolumeLevel_;
    std::vector<DeviceType> cachedDevices_;
    std::vector<DeviceType> lastUsedDevices_;
    std::FILE *curFile_;
    std::string curFileName_;
    telux::common::Status status_;
    std::mutex writeMtx_;
    std::mutex playerMtx_;
    std::mutex streamMtx_;
    std::vector<PlaybackConfig> playbackConfigs_;
    std::condition_variable adspReady_;
    std::condition_variable asyncResponse_;
    std::condition_variable bufferAvailable_;
    std::condition_variable compressedPlayStopped_;
    telux::common::ErrorCode errToReport_;

    std::weak_ptr<IPlayListListener> statusListener_;
    std::shared_ptr<IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioPlayStream> audioPlayStream_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> bufferPool_;
    telux::common::AsyncTaskQueue<void> asyncTaskQ_;

    telux::common::ErrorCode initAudioStream(StreamConfig streamConfig);
    telux::common::ErrorCode reinitAudioStream(uint32_t curFileIdx);
    telux::common::ErrorCode deinitAudioStream();
    telux::common::ErrorCode initFileToPlay();
    telux::common::ErrorCode deinitFileToPlay();
    telux::common::ErrorCode prepareBuffers();
    telux::common::ErrorCode playAudioSamples();
    telux::common::ErrorCode finalizePlayback();
    telux::common::ErrorCode finalizeCompressedPlayback();
    telux::common::ErrorCode registerForSSREvent();
    telux::common::ErrorCode deregisterForSSREvent();
    telux::common::ErrorCode waitAllWriteResponse();
    telux::common::ErrorCode setFormatAndOffset(AudioFormat audioFormat);
    telux::common::ErrorCode adjustFileAndState();
    telux::common::ErrorCode updateVolume(
        float volumeLevel, std::unique_lock<std::mutex> &streamLock);
    telux::common::ErrorCode updateMute(bool enable, std::unique_lock<std::mutex> &streamLock);
    telux::common::ErrorCode updateDevice(
        std::vector<DeviceType> devices, std::unique_lock<std::mutex> &streamLock);

    bool isStreamConfigurationSame(uint32_t curFileIdx);
    void resetState();
    void terminatePlayback();
    void reportError(telux::common::ErrorCode ec, std::string file);
    void reportPlayed();
    void reportPlaybackFinished();
    void reportPlaybackStarted();
    void reportPlaybackStopped();
    void unblockPlayerThread(bool setSSRStatus);
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // AudioPlayerImpl_HPP