/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2023,2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       AudioClient.cpp
 *
 * @brief      AudioClient class provides methods to start and stop voice session.
 *             It manages the audio subsystem using Telematics-SDK APIs.
 */

#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "AudioClient.hpp"

#define FILE_PATH "/etc"
#define FILE_NAME "telsdk_app.conf"
#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_CHANNEL_MASK 1
#define DEFAULT_DEVICE_SPEAKER 1
#define DEFAULT_DEVICE_MIC 257
#define DEFAULT_AUDIO_FORMAT 1
#define DEFAULT_ECNR_MODE 0

AudioClient::AudioClient()
    : audioMgr_(nullptr), ready_(false) {
}

AudioClient::~AudioClient() {
}

std::shared_ptr<AudioClient> AudioClient::getInstance() {
   static std::shared_ptr<AudioClient> instance(new AudioClient);
   return instance;
}

bool AudioClient::isReady() {
    return ready_;
}

// Initialize the audio subsystem
Status AudioClient::init() {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED
    // Get the AudioFactory and AudioManager instances.
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();

    //  Get the AudioFactory and AudioManager instances.
    auto &audioFactory = AudioFactory::getInstance();

    audioMgr_ = audioFactory.getAudioManager([&prom](
            telux::common::ServiceStatus status) {
        if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            prom.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        } else {
            prom.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
        }
    });

    if (!audioMgr_) {
        ready_ = false;
        std::cout << "Failed to get AudioManager object" << std::endl;
        return Status::FAILED;
    }

    //  Check if audio subsystem is ready
    ServiceStatus managerStatus = audioMgr_->getServiceStatus();

    //  If audio subsystem is not ready, wait for it to be ready
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }

    //  Exit the application, if SDK is unable to initialize audio subsystems
    if (managerStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        ready_ = true;
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Audio Subsystems to ready : " << elapsedTime.count() << "s"
                << std::endl;
        setActiveSession(DEFAULT_SLOT_ID);
        loadConfFileData();
    } else {
        ready_ = false;
        std::cout << " *** ERROR - Unable to initialize audio subsystem" << std::endl;
        return Status::FAILED;
    }
    auto status = audioMgr_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Audio Listener Registeration failed" <<std::endl;
    }

    return Status::SUCCESS;
#else
    return Status::FAILED;
#endif
}

// Function to start an audio on voice call
void AudioClient::startVoiceSession(SlotId slotId) {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED
    if (!audioMgr_) {
        std::cout << "Invalid Audio Manager" << std::endl;
        return;
    }

    if (hasConcurrentVoiceCall_) {
        /* If the device doesn't have real DSDA support, then only one voice call
         * can be active at any time, therefore, delete all streams to make room
         * for the new audio stream */
        for (auto &session : voiceSessions_) {
            stopVoiceSession(session.first);
        }
        voiceSessions_.clear();
        activeSession_ = nullptr;
    }

    setActiveSession(slotId);
    queryInputType();
    config_.slotId = slotId;
    config_.type = StreamType::VOICE_CALL;
    config_.format = AudioFormat::PCM_16BIT_SIGNED;

    auto status = activeSession_->createStream(config_);
    if (status == Status::SUCCESS) {
        status = activeSession_->startAudio();
    }
    if (status == Status::SUCCESS) {
        std::cout << "Audio is enabled for call on slotId : " << slotId << std::endl;
        if (hasConcurrentVoiceCall_) {
            currentSlotId_ = slotId;
            if (slotId == SLOT_ID_1) {
                audioStartedOnSim1_ = true;
            }
            if (slotId == SLOT_ID_2) {
                audioStartedOnSim2_ = true;
            }
        }
    } else {
        std::cout << "Error in enabling audio on slotId : " << slotId << std::endl;
    }
    return;
#else
    return;
#endif
}

// Function to stop an active voice session
void AudioClient::stopVoiceSession(SlotId slotId) {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED

    if (hasConcurrentVoiceCall_) {
        /* If the stream doesn't exist, return early */
        if ((slotId == SLOT_ID_1) && !audioStartedOnSim1_) {
            return;
        }
        if ((slotId == SLOT_ID_2) && !audioStartedOnSim2_) {
            return;
        }
    }

    setActiveSession(slotId);
    auto status = activeSession_->stopAudio();
    if (status == Status::SUCCESS) {
        status = activeSession_->deleteStream();
    }
    if (status == Status::SUCCESS) {
        std::cout << "Audio is disabled for call on slotId : " << slotId << std::endl;
        if (hasConcurrentVoiceCall_) {
            if (slotId == SLOT_ID_1){
                audioStartedOnSim1_ = false;
            }
            if (slotId == SLOT_ID_2){
                audioStartedOnSim2_ = false;
            }
            currentSlotId_ = INVALID_SLOT_ID;
            if (!audioStartedOnSim1_ && !audioStartedOnSim2_) {
                previousSlotId_ = INVALID_SLOT_ID;
            }
        }
    } else {
        std::cout << "Error in disabling audio on slotId : " << slotId << std::endl;
    }
    return;
#else
    return;
#endif
}

void AudioClient::setActiveSession(SlotId slotId) {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED
    std::lock_guard<std::mutex> lk(mutex_);
    if (!voiceSessions_.count(slotId)) {
        auto session = std::make_shared<VoiceSession>();
        voiceSessions_.insert(std::pair<SlotId, std::shared_ptr<VoiceSession>>(slotId, session));
    }
    activeSession_ = voiceSessions_[slotId];
#else
    return;
#endif
}

void AudioClient::loadConfFileData() {
    std::string input = "";
    int command = -1;
    ConfigParser parser(FILE_NAME, FILE_PATH);
    std::cout << "----- Default Parameters -----" << std::endl;
    config_.format = static_cast<AudioFormat>(DEFAULT_AUDIO_FORMAT);
    try {
        input = parser.getValue("SAMPLE_RATE");
        config_.sampleRate = static_cast<uint32_t>(std::stoi(input));
        input = parser.getValue("DEVICE_TYPE_SPEAKER");
        DeviceType device = static_cast<DeviceType>(std::stoi(input));
        config_.deviceTypes.clear();
        config_.deviceTypes.emplace_back(device);
        input = parser.getValue("DEVICE_TYPE_MIC");
        device = static_cast<DeviceType>(std::stoi(input));
        config_.deviceTypes.emplace_back(device);
        input = parser.getValue("CHANNEL_MASK");
        command = std::stoi(input);
        if (command == 1 || command == 2 || command == 3){
            if (command == 1) {
                config_.channelTypeMask = ChannelType::LEFT;
            } else if (command == 2) {
                config_.channelTypeMask = ChannelType::RIGHT;
            } else {
                config_.channelTypeMask = (ChannelType::LEFT | ChannelType::RIGHT);
            }
        } else {
            std::cout << "Invalid channel mask using default value" << std::endl;
            config_.channelTypeMask = static_cast<ChannelTypeMask>(DEFAULT_CHANNEL_MASK);
        }
        input = parser.getValue("ECNR_MODE");
        command = std::stoi(input);
        if (command == 0) {
            config_.ecnrMode = EcnrMode::DISABLE;
        } else if (command == 1) {
            config_.ecnrMode = EcnrMode::ENABLE;
        } else {
            std::cout << "Invalid ecnr mode using default value" << std::endl;
            config_.ecnrMode = EcnrMode::DISABLE;
        }
        input = parser.getValue("MULTISIM_VOICE_CONCURRENCY");
        command = std::stoi(input);
        hasConcurrentVoiceCall_ = (command == 1) ? true : false;
    } catch (const std::exception &e) {
        std::cout << "ERROR: "<< "Unable to read from file" << std::endl;
        std::cout << "Using default parameters" << std::endl;
        config_.sampleRate = DEFAULT_SAMPLE_RATE;
        config_.deviceTypes.clear();
        config_.deviceTypes.emplace_back(static_cast<DeviceType>(DEFAULT_DEVICE_SPEAKER));
        config_.deviceTypes.emplace_back(static_cast<DeviceType>(DEFAULT_DEVICE_MIC));
        config_.channelTypeMask = static_cast<ChannelTypeMask>(DEFAULT_CHANNEL_MASK);
        config_.ecnrMode = static_cast<EcnrMode>(DEFAULT_ECNR_MODE);
    }
    std::cout << "The sample rate is " << config_.sampleRate << std::endl;
    std::cout << "The devices are " << static_cast<int>(config_.deviceTypes[0]) << " and " <<
    static_cast<int>(config_.deviceTypes[1])<< std::endl;
    std::cout << "Channel mask is " << static_cast<int>(config_.channelTypeMask) << std::endl;
    std::cout << "ECNR Mode is " << static_cast<int>(config_.ecnrMode) << std::endl;
    return;
}

void AudioClient::setMuteStatus(SlotId slotId, bool muteStatus) {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED
    if (muteStatus) {
        return muteStream(slotId);
    }
    return unmuteStream(slotId);
#endif
}

void AudioClient::muteStream(SlotId slotId) {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED
    setActiveSession(slotId);
    StreamMute mute{};
    mute.enable = true;
    mute.dir = StreamDirection::RX;
    auto status = activeSession_->setMute(mute);
    if (status == Status::SUCCESS) {
        std::cout << " Muted stream on slotId " << slotId << std::endl;
        if (hasConcurrentVoiceCall_ && (previousSlotId_ == INVALID_SLOT_ID)) {
            previousSlotId_ = currentSlotId_;
        }
    } else {
        std::cout << " Failed mute stream on slotId " << slotId << std::endl;
    }
#endif
}

void AudioClient::unmuteStream(SlotId slotId) {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED
    if (hasConcurrentVoiceCall_) {
        SlotId tmpSlotId{};
        tmpSlotId = previousSlotId_;
        previousSlotId_ = currentSlotId_;
        currentSlotId_ = previousSlotId_;
        if (activeSession_->getSlotId() != slotId) {
            return startVoiceSession(slotId);
        }
    }

    setActiveSession(slotId);
    StreamMute mute{};
    mute.enable = false;
    mute.dir = StreamDirection::RX;
    auto status = activeSession_->setMute(mute);
    if (status == Status::SUCCESS) {
        std::cout << " Unmuted stream on slotId " << slotId << std::endl;
    } else {
        std::cout << " Failed unmute stream on slotId " << slotId << std::endl;
    }
#endif
}

void AudioClient::queryInputType() {
#ifdef TELSDK_FEATURE_AUDIO_ENABLED
    std::string inputSelection;
    char delimiter = '\n';
    int consoleFlag = 0;
    std::cout << "Enter 0 to specify audio parameters, press 1 to use default: ";
    std::getline(std::cin, inputSelection, delimiter);
    if (!inputSelection.empty()) {
        try {
            consoleFlag = std::stoi(inputSelection);
            if (consoleFlag < 0 || consoleFlag > 1) {
                std::cout << "ERROR: Invalid selection" << std::endl;
                return;
            }
        } catch (const std::exception &e) {
            std::cout << "ERROR: "<< e.what() << std::endl;
            return;
        }
    } else {
        std::cout << "Empty input, enter correct choice" << std::endl;
        return;
    }
    if (consoleFlag) {
        loadConfFileData();
        return;
    }
    AudioHelper::getUserSampleRateInput(config_.sampleRate);
    AudioHelper::getUserChannelInput(config_.channelTypeMask);
    config_.deviceTypes.clear();
    AudioHelper::getUserDeviceInput(config_.deviceTypes);
    AudioHelper::getUserEcnrModeInput(config_.ecnrMode);
#else
    return;
#endif
}

void AudioClient::setSystemReady() {
    ready_ = true;
}

void AudioClient::cleanup() {
    ready_ = false;
    voiceSessions_.clear();
    activeSession_ = nullptr;
}

void AudioClient::onServiceStatusChange(telux::common::ServiceStatus status) {
    if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
        cleanup();
    }
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Service AVAILABLE" << std::endl;
        setSystemReady();
    }
}
