/*
 *  Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023,2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>
#include <sstream>

#include <telux/audio/AudioFactory.hpp>

#include "VoiceSession.hpp"

VoiceSession::VoiceSession()
    : audioStarted_(false) {
}

VoiceSession::~VoiceSession() {
}

Status VoiceSession::startAudio() {
    auto statusFromRequest = Status::FAILED;
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    telux::common::Status statusFromResponse;

    if (audioVoiceStream_) {
        if (!audioStarted_) {
            std::promise<bool> p;
            statusFromRequest = audioVoiceStream_->startAudio(
                [&p, &statusFromResponse, &audioVoiceStream_, this](ErrorCode error) {
                if (error == ErrorCode::SUCCESS) {
                    statusFromResponse = telux::common::Status::SUCCESS;
                    audioStarted_ = true;
                    p.set_value(true);
                } else {
                    statusFromResponse = telux::common::Status::FAILED;
                    p.set_value(false);
                }
            });
            if(statusFromRequest == Status::SUCCESS) {
                p.get_future().wait();
                return statusFromResponse;
            }
        } else {
            LOG(ERROR, "Audio already started");
            statusFromRequest = Status::ALREADY;
        }
    } else {
        LOG(ERROR, "No stream exists");
    }
    return statusFromRequest;
}

Status VoiceSession::stopAudio() {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (audioVoiceStream_ && audioStarted_) {
        std::promise<bool> p;
        statusFromRequest = audioVoiceStream_->stopAudio(
            [&p, &statusFromResponse, &audioVoiceStream_, this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                statusFromResponse = telux::common::Status::SUCCESS;
                audioStarted_ = false;
                p.set_value(true);
            } else {
                statusFromResponse = telux::common::Status::FAILED;
                p.set_value(false);
            }
        });
        if(statusFromRequest == Status::SUCCESS) {
            p.get_future().wait();
            return statusFromResponse;
        }
    } else {
        LOG(ERROR, "Audio not started yet");
    }
    return statusFromRequest;
}

Status VoiceSession::startDtmf(DtmfTone tone, uint32_t duration, uint16_t gain) {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (audioVoiceStream_ && audioStarted_) {
        std::promise<bool> p;
        statusFromRequest = audioVoiceStream_->playDtmfTone(tone, duration, gain,
            [&p, &statusFromResponse, &audioVoiceStream_](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                statusFromResponse = telux::common::Status::SUCCESS;
                p.set_value(true);
            } else {
                statusFromResponse = telux::common::Status::FAILED;
                p.set_value(false);
            }
        });
        if(statusFromRequest == Status::SUCCESS) {
            p.get_future().wait();
            return statusFromResponse;
        }
    } else {
        LOG(ERROR, "Audio not started yet");
    }
    return statusFromRequest;
}

Status VoiceSession::stopDtmf() {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (audioVoiceStream_) {
        std::promise<bool> p;
        statusFromRequest = audioVoiceStream_->stopDtmfTone(StreamDirection::RX,
            [&p, &statusFromResponse, &audioVoiceStream_](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                statusFromResponse = telux::common::Status::SUCCESS;
                p.set_value(true);
            } else {
                statusFromResponse = telux::common::Status::FAILED;
                p.set_value(false);
            }
            });
        if(statusFromRequest == Status::SUCCESS) {
            p.get_future().wait();
            return statusFromResponse;
        }
    } else {
        LOG(ERROR, "No stream exists");
    }
    return statusFromRequest;
}

Status VoiceSession::registerListener(std::weak_ptr<IVoiceListener> listener) {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (audioVoiceStream_ && audioStarted_) {
        std::promise<bool> p;
        statusFromRequest = audioVoiceStream_ ->registerListener(
            listener, [&p, &statusFromResponse, &audioVoiceStream_](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                statusFromResponse = telux::common::Status::SUCCESS;
                p.set_value(true);
            } else {
                statusFromResponse = telux::common::Status::FAILED;
                p.set_value(false);
                LOG(ERROR, "Failed to register Listener");
            }
            });
        if(statusFromRequest == Status::SUCCESS) {
            p.get_future().wait();
            return statusFromResponse;
        }
    } else {
        LOG(ERROR, "Audio is not started yet");
    }
    return statusFromRequest;
}

Status VoiceSession::deRegisterListener(std::weak_ptr<IVoiceListener> listener) {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto statusFromRequest = Status::FAILED;
    if (audioVoiceStream_ && audioStarted_) {
        statusFromRequest = audioVoiceStream_ ->deRegisterListener(listener);
        if (statusFromRequest == Status::SUCCESS) {
            LOG(DEBUG, "Request to deregister DTMF Sent");
        }
    } else {
        LOG(ERROR, "Audio is not started yet");
    }
    return statusFromRequest;
}

SlotId VoiceSession::getSlotId() {
    return slotId_;
}
