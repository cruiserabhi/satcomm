/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef VOICESTREAMIMPL_HPP
#define VOICESTREAMIMPL_HPP

#include "AudioStreamImpl.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"

namespace telux {
namespace audio {

/*
 * Represents an audio stream used for exchaning audio over a voice-call.
 */
class VoiceStreamImpl : public AudioStreamImpl,
                        public IAudioVoiceStream,
                        public IDTMFCb,
                        public IStartStreamCb,
                        public IStopStreamCb,
                        public IVoiceStreamEventsCb {

 public:
    VoiceStreamImpl(uint32_t streamId,
        std::shared_ptr<ICommunicator> transportClient);

    ~VoiceStreamImpl();

    telux::common::Status init(void);

    telux::common::Status startAudio(
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status stopAudio(
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status playDtmfTone(DtmfTone dtmfTone, uint16_t duration, uint16_t gain,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status stopDtmfTone(StreamDirection direction,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status registerListener(std::weak_ptr<IVoiceListener> listener,
        telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status deRegisterListener(std::weak_ptr<IVoiceListener> listener) override;

    void onStreamStartResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onStreamStopResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onPlayDtmfResult(telux::common::ErrorCode ec, uint32_t streamId,
            int cmdId) override;

    void onStopDtmfResult(telux::common::ErrorCode ec, uint32_t streamId,
           int cmdId) override;
    void onDtmfToneDetected(DtmfTone dtmfTone) override;

 private:
    telux::common::AsyncTaskQueue<void> asyncTaskQueue_;
    std::shared_ptr<telux::common::ListenerManager<IVoiceListener>> eventListenerMgr_;

    VoiceStreamImpl(VoiceStreamImpl const &) = delete;
    VoiceStreamImpl &operator=(VoiceStreamImpl const &) = delete;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // VOICESTREAMIMPL_HPP
