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
 *  Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       AudioSession.cpp
 *
 * @brief      AudioSession class provides methods to create & delete the various types of streams.
 *             Also this class provides the methods for device switch, volume and mute control
 *             which are applicable to various streams.
 */

#include <iostream>
#include <sstream>

#include <telux/audio/AudioFactory.hpp>

#include "AudioSession.hpp"

AudioSession::AudioSession()
    : stream_(nullptr) {
}

AudioSession::~AudioSession() {
}

Status AudioSession::createStream(StreamConfig config) {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (!stream_) {
        std::promise<bool> p;
        auto &audioFactory = AudioFactory::getInstance();
        auto audioManager = audioFactory.getAudioManager();
        if (audioManager) {
            // Sending request to create audio stream
            statusFromRequest = audioManager->createStream(config,
                [&p, &statusFromResponse, this](std::shared_ptr<IAudioStream> &audioStream,
                    ErrorCode error) {
                    if (error == ErrorCode::SUCCESS) {
                        stream_ = audioStream;
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
            LOG(ERROR, "Invalid audio Manager");
            return Status::FAILED;
        }
    } else {
        LOG(DEBUG, "Stream already exist");
        return Status::ALREADY;
    }
    return statusFromRequest;
}

Status AudioSession::deleteStream() {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (stream_) {
        std::promise<bool> p;
        auto &audioFactory = AudioFactory::getInstance();
        auto audioManager = audioFactory.getAudioManager();
        if (audioManager) {
            statusFromRequest = audioManager-> deleteStream(stream_,
                    [&p, &statusFromResponse, this](ErrorCode error) {
                if (error == ErrorCode::SUCCESS) {
                    stream_ = nullptr;
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
            LOG(ERROR, "Invalid audio Manager");
            return Status::FAILED;
        }
    } else {
        LOG(ERROR, "No stream exists");
        return Status::FAILED;
    }
    return statusFromRequest;
}

Status AudioSession::getStreamDevice(std::vector<DeviceType> &devices) {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (stream_) {
        std::promise<bool> p;
        statusFromRequest = stream_->getDevice([&p, &statusFromResponse, &devices, this]
                (std::vector<DeviceType> deviceTypes, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                statusFromResponse = telux::common::Status::SUCCESS;
                devices = deviceTypes;
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
        return Status::FAILED;
    }
    return statusFromRequest;
}

Status AudioSession::setStreamDevice(std::vector<DeviceType> devices) {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (stream_) {
        std::promise<bool> p;
        statusFromRequest = stream_->setDevice(devices,
            [&p, &statusFromResponse, this](ErrorCode error) {
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
        return Status::FAILED;
    }
    return statusFromRequest;
}

Status AudioSession::setVolume(StreamVolume streamVol) {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (stream_) {
        std::promise<bool> p;
        statusFromRequest = stream_->setVolume(streamVol,
            [&p, &statusFromResponse, this](ErrorCode error) {
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
        return Status::FAILED;
    }
    return statusFromRequest;
}

Status AudioSession::getVolume(StreamVolume &volume) {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (stream_) {
        std::promise<bool> p;
        statusFromRequest = stream_->getVolume(volume.dir,
            [&p, &statusFromResponse, &volume, this](StreamVolume vol, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                statusFromResponse = telux::common::Status::SUCCESS;
                volume = vol;
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
        return Status::FAILED;
    }
    return statusFromRequest;
}

Status AudioSession::setMute(StreamMute mute) {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (stream_) {
        std::promise<bool> p;
        statusFromRequest = stream_->setMute(mute, [&p, &statusFromResponse, this](ErrorCode error) {
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
        return Status::FAILED;
    }
    return statusFromRequest;
}

Status AudioSession::getMute(StreamMute &muteStatus) {
    Status statusFromRequest = Status::FAILED;
    telux::common::Status statusFromResponse;

    if (stream_) {
        std::promise<bool> p;
        statusFromRequest = stream_->getMute(muteStatus.dir,
            [&p, &statusFromResponse, &muteStatus, this](StreamMute mute, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                statusFromResponse = telux::common::Status::SUCCESS;
                muteStatus = mute;
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
        return Status::FAILED;
    }
    return statusFromRequest;
}
