/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TRANSCODERIMPL_HPP
#define TRANSCODERIMPL_HPP

#include <telux/audio/AudioTranscoder.hpp>
#include <telux/audio/AudioListener.hpp>
#include "ICommunicator.hpp"
#include "common/ListenerManager.hpp"

namespace telux {
namespace audio {

/*
 * Provides routines to transcode audio from AMR* to PCM format.
 */
class TranscoderImpl : public ITranscoder,
                       public ITranscodeDeleteCb,
                       public IWriteCb,
                       public IReadCb,
                       public IPlayStreamEventsCb,
                       public std::enable_shared_from_this<TranscoderImpl> {
 public:
    TranscoderImpl(CreatedTranscoderInfo,
        std::shared_ptr<ICommunicator> transportClient);

    ~TranscoderImpl();

    telux::common::Status init(void);

    std::shared_ptr<IAudioBuffer> getWriteBuffer() override;

    std::shared_ptr<IAudioBuffer> getReadBuffer() override;

    telux::common::Status write(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t isLastBuffer, TranscoderWriteResponseCb callback = nullptr) override;

    telux::common::Status read(std::shared_ptr<IAudioBuffer> buffer, uint32_t numBytesToRead,
        TranscoderReadResponseCb callback = nullptr) override;

    telux::common::Status tearDown(telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status registerListener(std::weak_ptr<ITranscodeListener> listener) override;

    telux::common::Status deRegisterListener(std::weak_ptr<ITranscodeListener> listener) override;

    void onWriteResult(telux::common::ErrorCode ec, uint32_t streamId,
        uint32_t bytesWritten, AudioUserData *audioUserData) override;

    void onReadResult(telux::common::ErrorCode ec, uint32_t streamId,
        uint32_t numBytesActuallyRead, AudioUserData *audioUserData) override;

    void onWriteReady(uint32_t streamId) override;

    void onDrainDone(uint32_t streamId) override;

    void onDeleteTranscoderResult(telux::common::ErrorCode ec,
        uint32_t inStreamId_, uint32_t outStreamId_, int cmdId) override;

    void onServiceStatusChange();

 private:
    uint32_t inStreamId_ = 0;
    uint32_t outStreamId_ = 0;
    uint32_t readMinSize_ = 0;
    uint32_t readMaxSize_ = 0;
    uint32_t writeMinSize_ = 0;
    uint32_t writeMaxSize_ = 0;
    uint32_t isLastBuffer_ = 0;
    telux::common::CommandCallbackManager cmdCallbackMgr_;
    std::shared_ptr<ICommunicator> transportClient_;
    std::shared_ptr<telux::common::ListenerManager<ITranscodeListener>> eventListenerMgr_;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // TRANSCODERIMPL_HPP