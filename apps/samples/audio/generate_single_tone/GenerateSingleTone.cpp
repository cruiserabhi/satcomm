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
 * Steps to create a tone stream and generate single tone on the local
 * speaker are as follows:
 *
 * 1. Get an AudioFactory instance.
 * 2. Get an IAudioManager instance from the AudioFactory.
 * 3. Wait for the audio service to become available.
 * 4. Create a tone stream (IAudioToneGeneratorStream).
 * 5. Configure parameters for the tone and generate it.
 * 6. When the use-case is complete, stop the tone.
 * 7. Delete tone stream.
 *
 * Usage:
 * # generate_single_tone
 */

#include <errno.h>

#include <cstdio>
#include <iostream>
#include <thread>

#include <telux/audio/AudioFactory.hpp>

#include "GenerateSingleTone.hpp"

/*
 * Initialize application and get an audio service.
 */
int GenerateSingleTone::init() {

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
 *  Step - 4, create a tone stream.
 */
int GenerateSingleTone::createToneStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::TONE_GENERATOR;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            audioToneStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioToneGeneratorStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't create tone stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create tone stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream created" << std::endl;
    return 0;
}

/*
 *  Step - 7, delete tone stream.
 */
int GenerateSingleTone::deleteToneStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(audioToneStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete tone stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete tone stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream deleted" << std::endl;
    return 0;
}

/*
 *  Step - 5, generate tone.
 */
int GenerateSingleTone::generateSingleTone() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    std::vector<uint16_t> frequency{1244};
    uint16_t duration = 12000;
    uint16_t gain = 5000;

    status = audioToneStream_->playTone(frequency, duration, gain,
        [&p] (telux::common::ErrorCode result) {
            p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't play tone, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed play tone, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Tone generation started" << std::endl;
    return 0;
}

/*
 * Step - 6, stop tone.
 */
int GenerateSingleTone::stopGeneratingTone() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioToneStream_->stopTone([&p] (telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't stop tone, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed stop tone, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Tone generation stopped" << std::endl;
    return 0;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<GenerateSingleTone> app;

    try {
        app = std::make_shared<GenerateSingleTone>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate GenerateSingleTone" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->createToneStream();
    if (ret < 0) {
        return ret;
    }

    ret = app->generateSingleTone();
    if (ret < 0) {
        app->deleteToneStream();
        return ret;
    }

    /* Application's business logic goes here. We are sleeping just as an example */
    std::this_thread::sleep_for(std::chrono::seconds(2));

    ret = app->stopGeneratingTone();
    if (ret < 0) {
        app->deleteToneStream();
        return ret;
    }

    ret = app->deleteToneStream();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
