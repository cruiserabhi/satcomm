/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * Steps to create a voice call stream and generate DTMF tone on the local
 * speaker are as follows:
 *
 * 1. Get an AudioFactory instance.
 * 2. Get an IAudioManager instance from the AudioFactory.
 * 3. Wait for the audio service to become available.
 * 4. Create a voice call stream (IAudioVoiceStream).
 * 5. Start voice call stream.
 * 6. Configure parameters for DTMF tone and generate it.
 * 7. When the use-case is complete, stop voice call stream.
 * 8. Delete voice call stream.
 *
 * Usage:
 * # generate_dtmf_tone
 */

#include <errno.h>

#include <cstdio>
#include <iostream>
#include <thread>

#include <telux/audio/AudioFactory.hpp>

#include "GenerateDTMF.hpp"

/*
 * Initialize application and get an audio service.
 */
int GenerateDTMF::init() {

    std::promise<telux::common::ServiceStatus> p{};
    telux::common::ServiceStatus serviceStatus;

    /* Step - 1 */
    auto &audioFactory = telux::audio::AudioFactory::getInstance();

    /* Step - 2 */
    audioManager_ = audioFactory.getAudioManager(
            [&p](telux::common::ServiceStatus srvStatus) {
        p.set_value(srvStatus);
    });

    if (!audioManager_) {
        std::cout << "Can't get IAudioManager" << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "audio service unavailable" << std::endl;
        return -EIO;
    }

    std::cout << "Initialization finished" << std::endl;
    return 0;
}

/*
 *  Step - 4, create a voice call stream.
 */
int GenerateDTMF::createVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::VOICE_CALL;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

    /* For voice-call both sink and source device are required.
     * First device should be sink (speaker) and second should be source (mic). */
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            audioVoiceStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioVoiceStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't create voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream created" << std::endl;
    return 0;
}

/*
 *  Step - 8, delete voice call stream.
 */
int GenerateDTMF::deleteVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(audioVoiceStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream deleted" << std::endl;
    return 0;
}

/*
 *  Step - 5, start voice call stream.
 */
int GenerateDTMF::startVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioVoiceStream_->startAudio([&p] (telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't start voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed start voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream started" << std::endl;
    return 0;
}

/*
 * Step - 7, stop voice call stream.
 */
int GenerateDTMF::stopVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioVoiceStream_->stopAudio([&p] (telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't stop voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed stop voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream stopped" << std::endl;
    return 0;
}

/*
 *  Step - 6, configure parameters for DTMF tone and generate it.
 */
int GenerateDTMF::generateDTMFTone() {

    uint16_t toneGain;
    uint16_t toneDuration;
    telux::common::ErrorCode ec;
    telux::common::Status status;
    telux::audio::DtmfTone dtmfTone{};
    std::promise<telux::common::ErrorCode> p{};

    toneGain = 6000; /* 6000 as loudness */
    toneDuration = 1000; /* 1000 milliseconds */
    dtmfTone.direction = telux::audio::StreamDirection::RX;
    dtmfTone.lowFreq = telux::audio::DtmfLowFreq::FREQ_697;
    dtmfTone.highFreq = telux::audio::DtmfHighFreq::FREQ_1209;

    status = audioVoiceStream_->playDtmfTone(dtmfTone, toneDuration, toneGain,
            [&p] (telux::common::ErrorCode error) {
        p.set_value(error);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't generate tone, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed to generate tone, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Generation started" << std::endl;
    return 0;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<GenerateDTMF> app;

    try {
        app = std::make_shared<GenerateDTMF>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate GenerateDTMF" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->createVoiceStream();
    if (ret < 0) {
        return ret;
    }

    ret = app->startVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->generateDTMFTone();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    /* Application's business logic goes here.
     * We are sleeping just as an example */
    std::this_thread::sleep_for(std::chrono::seconds(2));

    ret = app->stopVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteVoiceStream();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
