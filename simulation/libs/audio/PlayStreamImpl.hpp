/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef PLAYSTREAMIMPL_HPP
#define PLAYSTREAMIMPL_HPP

#include <telux/audio/AudioListener.hpp>
#include <telux/common/SDKListener.hpp>
#include "common/ListenerManager.hpp"
#include "AudioStreamImpl.hpp"


namespace telux {
namespace audio {

/*
 * Represents an audio stream meant to play audio.
 */
class PlayStreamImpl : public IAudioPlayStream,
                       public AudioStreamImpl,
                       public IWriteCb,
                       public IFlushCb,
                       public IDrainCb,
                       public IPlayStreamEventsCb {

 public:
    PlayStreamImpl(uint32_t streamId, uint32_t writeMinSize, uint32_t writeMaxSize,
        std::shared_ptr<ICommunicator> transportClient);

    ~PlayStreamImpl();

    telux::common::Status init(void);

    std::shared_ptr<IStreamBuffer> getStreamBuffer() override;

    telux::common::Status write(std::shared_ptr<IStreamBuffer> buffer,
        WriteResponseCb callback = nullptr) override;

    void onWriteResult(telux::common::ErrorCode ec, uint32_t streamId,
        uint32_t bytesWritten, AudioUserData *audioUserData) override;

    void onFlushResult(telux::common::ErrorCode ec, uint32_t streamId, int cmdId) override;

    void onDrainResult(telux::common::ErrorCode ec, uint32_t streamId, int cmdId) override;

    telux::common::Status stopAudio(StopType stopType,
        telux::common::ResponseCallback callback = nullptr);

    telux::common::Status registerListener(std::weak_ptr<IPlayListener> listener);

    telux::common::Status deRegisterListener(std::weak_ptr<IPlayListener> listener);

    void onDrainDone(uint32_t streamId) override;

    void onWriteReady(uint32_t streamId) override;

 private:
    uint32_t writeMinSize_ = 0;
    uint32_t writeMaxSize_ = 0;
    std::shared_ptr<telux::common::ListenerManager<IPlayListener>> eventListenerMgr_;

    PlayStreamImpl(PlayStreamImpl const &) = delete;
    PlayStreamImpl &operator=(PlayStreamImpl const &) = delete;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // PLAYSTREAMIMPL_HPP
