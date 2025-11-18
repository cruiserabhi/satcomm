/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOSTREAMIMPL_HPP
#define AUDIOSTREAMIMPL_HPP

#include <telux/audio/AudioManager.hpp>

#include "common/CommonUtils.hpp"
#include "ICommunicator.hpp"

namespace telux {
namespace audio {

/*
 * Represents a generic audio stream primarily intended to be subclassed to
 * represent specific audio stream type.
 */
class AudioStreamImpl : virtual public IAudioStream,
        public ISetGetDeviceCb,
        public ISetGetVolumeCb,
        public ISetGetMuteCb,
        public telux::common::enable_inheritable_shared_from_this<AudioStreamImpl> {

 public:
    AudioStreamImpl(uint32_t streamId, StreamType streamType,
        std::shared_ptr<ICommunicator> transportClient);

    ~AudioStreamImpl();

    uint32_t getStreamId(void);

    StreamType getType() override;

    telux::common::Status setDevice(std::vector<DeviceType> devices,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status getDevice(
        GetStreamDeviceResponseCb callback = nullptr) override;

    telux::common::Status setVolume(StreamVolume volume,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status getVolume(StreamDirection dir,
        GetStreamVolumeResponseCb callback = nullptr) override;

    telux::common::Status setMute(StreamMute mute,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status getMute(StreamDirection dir,
        GetStreamMuteResponseCb callback = nullptr ) override;

    void onSetDeviceResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onGetDeviceResult(telux::common::ErrorCode ec, uint32_t streamId,
            std::vector<DeviceType> devices, int cmdId) override;

    void onSetVolumeResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onGetVolumeResult(telux::common::ErrorCode ec, uint32_t streamId,
            StreamVolume volume, int cmdId) override;

    void onSetMuteResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onGetMuteResult(telux::common::ErrorCode ec, uint32_t streamId,
            StreamMute streamMute, int cmdId) override;

    void onServiceStatusChange();

 protected:
    uint32_t streamId_;
    StreamType streamType_;
    std::shared_ptr<ICommunicator> transportClient_;
    telux::common::CommandCallbackManager cmdCallbackMgr_;

    AudioStreamImpl(AudioStreamImpl const &) = delete;
    AudioStreamImpl &operator=(AudioStreamImpl const &) = delete;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // AUDIOSTREAMIMPL_HPP
