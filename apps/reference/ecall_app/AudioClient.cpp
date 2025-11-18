/*
 *  Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       AudioClient.cpp
 *
 * @brief      AudioClient class provides methods to start and stop voice session.
 *             It manages the audio subsystem using Telematics-SDK APIs.
 */

#include <iostream>

#include "AudioClient.hpp"

#define CLIENT_NAME "ECall-Audio-Client: "

AudioClient::AudioClient()
    : voiceEnabled_(false)
    , audioMgr_(nullptr)
    , audioVoiceStream_(nullptr) {
}

AudioClient::~AudioClient() {
    setVoiceState(false);
}

// Callback which provides response to createStream
void AudioClient::createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        std::cout << CLIENT_NAME << "Voice stream created" << std::endl;
        audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream);
        auto status = audioVoiceStream_->startAudio(std::bind(&AudioClient::startAudioCallback,
                                                this, std::placeholders::_1));
        if (status == telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Request to start voice session sent." << std::endl;
        } else {
            std::cout << CLIENT_NAME << "Request to start voice session failed." << std::endl;
        }
    } else {
        std::cout << CLIENT_NAME << "Failed to create voice stream, error - " << (int)error
                    << std::endl;
    }
}

// Callback which provides response to deleteStream
void AudioClient::deleteStreamCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        std::cout << CLIENT_NAME << "Voice stream deleted successfully" << std::endl;
        audioVoiceStream_ = nullptr;
    } else {
        std::cout << CLIENT_NAME << "Failed to delete voice stream, error - " << (int)error
                    << std::endl;
    }
}

// Callback which provides response to startAudio.
void AudioClient::startAudioCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        setVoiceState(true);
        std::cout << CLIENT_NAME << "Voice session started successfully" << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Failed to start voice session, error - " << (int)error
                    << std::endl;
    }
}

// Callback which provides response to stopAudio.
void AudioClient::stopAudioCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        std::cout << CLIENT_NAME << "Voice session stopped successfully" << std::endl;
        setVoiceState(false);
        auto status = audioMgr_->deleteStream(audioVoiceStream_, std::bind(
                            &AudioClient::deleteStreamCallback, this, std::placeholders::_1));
        if(status == telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Request to delete voice stream sent." << std::endl;
        } else {
            std::cout << CLIENT_NAME << "Request to delete voice stream failed." << std::endl;
        }
    } else {
        std::cout << CLIENT_NAME << "Failed to stop voice session, error - " << (int)error
                    << std::endl;
    }
}

// Callback to notify audio subsystem restart
void AudioClient::onServiceStatusChange(ServiceStatus status) {
    if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << "Audio subsystem is UNAVAILABLE" << std::endl;
        setVoiceState(false);
        // Existing voice stream object is no longer valid
        audioVoiceStream_ = nullptr;
    } else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio subsystem is AVAILABLE" << std::endl;
        // In case of an SSR, automatically start audio session post SSR
        if(keepVoiceSessionActive_) {
            startVoiceSession(streamConfig_.modemSubId, streamConfig_.deviceTypes,
                streamConfig_.sampleRate, streamConfig_.format, streamConfig_.channelTypeMask,
                streamConfig_.ecnrMode);
        }
    }
}

// Callback which is invoked when Audio Manager initialization is processed(success or failure)
static void initCb(telux::common::ServiceStatus status) {
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << CLIENT_NAME << " Audio Manager is initialized successfully " << std::endl;
    } else if(status == telux::common::ServiceStatus::SERVICE_FAILED) {
        std::cout << CLIENT_NAME << " Audio Manager initialization failed" << std::endl;
    }
}

// Initialize the audio subsystem
telux::common::Status AudioClient::init() {
    // Get the AudioFactory and AudioManager instances.
    auto &audioFactory = AudioFactory::getInstance();
    audioMgr_ = audioFactory.getAudioManager(&initCb);
    if(audioMgr_ == nullptr) {
        std::cout << CLIENT_NAME << "*** ERROR - Failed to get Audio Manager instance" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = audioMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to register Audio listener" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Indicates whether a voice session is already active
bool AudioClient::isVoiceEnabled() {
    std::unique_lock<std::mutex> lock(mutex_);
    return voiceEnabled_;
}

void AudioClient::setVoiceState(bool state) {
    std::unique_lock<std::mutex> lock(mutex_);
    voiceEnabled_ = state;
}

// Function to start an active voice session
telux::common::Status AudioClient::startVoiceSession(int phoneId, std::vector<DeviceType> devices,
    uint32_t sampleRate, AudioFormat voiceFormat, ChannelTypeMask channels, EcnrMode ecnrMode) {
    keepVoiceSessionActive_ = true;
    if(isVoiceEnabled()) {
        std::cout << CLIENT_NAME << "Voice stream is enabled already" << std::endl;
        return telux::common::Status::SUCCESS;
    }
    if(!audioMgr_) {
        std::cout << CLIENT_NAME << "Invalid Audio Manager" << std::endl;
        return telux::common::Status::FAILED;
    } else if(ServiceStatus::SERVICE_AVAILABLE != audioMgr_->getServiceStatus()) {
        std::cout << CLIENT_NAME << " Audio Subsystem is not yet ready" << std::endl;
        return telux::common::Status::NOTREADY;
    }
    if(!audioVoiceStream_) {
        // Create a Voice Stream
        streamConfig_ = {};
        streamConfig_.type = StreamType::VOICE_CALL;
        streamConfig_.modemSubId = phoneId;
        streamConfig_.sampleRate = sampleRate;
        streamConfig_.format = voiceFormat;
        streamConfig_.channelTypeMask = channels;
        streamConfig_.deviceTypes.clear();
        streamConfig_.deviceTypes = devices;
        streamConfig_.ecnrMode = ecnrMode;
        auto status = audioMgr_->createStream(streamConfig_,
            std::bind(&AudioClient::createStreamCallback, this, std::placeholders::_1,
            std::placeholders::_2));
        if (status == telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Request to create voice stream sent." << std::endl;
        } else {
            std::cout << CLIENT_NAME << "Request to create voice stream failed" << std::endl;
            return telux::common::Status::FAILED;
        }
    } else {
        std::cout << CLIENT_NAME << "Voice stream is available already" << std::endl;
        auto status = audioVoiceStream_->startAudio(std::bind(&AudioClient::startAudioCallback,
                                                this, std::placeholders::_1));
        if (status == telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Request to start voice session sent" << std::endl;
        } else {
            std::cout << CLIENT_NAME << "Request to start voice session failed" << std::endl;
            return telux::common::Status::FAILED;
        }
    }

    return telux::common::Status::SUCCESS;
}

// Function to stop an active voice session
telux::common::Status AudioClient::stopVoiceSession() {
    keepVoiceSessionActive_ = false;
    if(!isVoiceEnabled()) {
        std::cout << CLIENT_NAME << "Voice stream is disabled already" << std::endl;
        return telux::common::Status::SUCCESS;
    }
    if(!audioVoiceStream_) {
        std::cout << CLIENT_NAME << "Invalid voice stream handle" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = audioVoiceStream_->stopAudio(std::bind(&AudioClient::stopAudioCallback,
                                                this, std::placeholders::_1));
    if(status == telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Request to stop voice session sent." << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Request to stop voice session failed." << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}
