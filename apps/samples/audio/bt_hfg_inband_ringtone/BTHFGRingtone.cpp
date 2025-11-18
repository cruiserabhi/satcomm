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
 * Steps to play an inband ringtone on a Bluetooth device are:
 *
 * 1. Get an AudioFactory instance.
 * 2. Get an IAudioManager instance from the AudioFactory.
 * 3. Wait for the audio service to become available.
 * 4. Create a playback stream (IAudioPlayStream).
 * 5. Start writing audio samples on the playback stream.
 * 6. When the playback is over, delete the playback stream.
 *
 * Usage:
 * # bt_hfg_inband_ringtone /data/ringtone.raw
 *
 * Contents of /data/ringtone.raw raw PCM file are played on the Bluetooth
 * headset connect to the device.
 */

#include <errno.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "BTHFGRingtone.hpp"

/*
 * Initialize application and get an audio service.
 */
int BTHFGRingtone::init() {

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
 * Step - 4, create a playback stream.
 */
int BTHFGRingtone::createPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::PLAY;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_BT_SCO_SPEAKER);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            audioPlayStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't create playback stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream created" << std::endl;
    return 0;
}

/*
 *  Step - 6, delete playback stream.
 */
int BTHFGRingtone::deletePlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(audioPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete play stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream deleted" << std::endl;
    return 0;
}

/*
 *  Gets called to confirm how many bytes were actually written to the playback stream.
 */
void BTHFGRingtone::writeComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    long offset;

    std::lock_guard<std::mutex> lock(playMutex_);

    if (error != telux::common::ErrorCode::SUCCESS) {
        /* Error occurred during playback, terminate the playback thread */
        errorOccurred_ = true;
        std::cout << "write failed, err " << static_cast<int>(error) << std::endl;
    } else if (buffer->getDataSize() != bytesWritten) {
        /* Whole buffer can't be played successfully */
        offset = (-1) * (static_cast<long>((buffer->getDataSize() - bytesWritten)));
        std::fseek(fileToPlay_, offset, SEEK_CUR);
    } else {
        /* Success, send the next buffer to play */
    }

    bufferPool_.push(buffer);
    cv_.notify_all();
}

/*
 *  Step - 5, write samples on the playback stream.
 */
void BTHFGRingtone::play() {

    uint32_t size = 0;
    uint32_t numBytes = 0;
    bool waitResult = false;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    std::unique_lock<std::mutex> lock(playMutex_);

    errorOccurred_ = false;

    fileToPlay_ = std::fopen(fileToPlayPath_, "rb");
    if (!fileToPlay_) {
        std::cout << "can't open file " << fileToPlayPath_ << std::endl;
        return;
    }

    /* Allocate buffers as per pool size */
    for (int x = 0; x < BUFFER_POOL_SIZE; x++) {
        streamBuffer = audioPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            std::fclose(fileToPlay_);
            bufferPool_ = {};
            return;
        }
        bufferPool_.push(streamBuffer);

        size = streamBuffer->getMinSize();
        if (!size) {
            size =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(size);
    }

    auto writeCb = std::bind(&BTHFGRingtone::writeComplete, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::cout << "playback started" << std::endl;

    while (1) {
        streamBuffer = bufferPool_.front();
        bufferPool_.pop();

        numBytes = std::fread(streamBuffer->getRawBuffer(), 1, size, fileToPlay_);
        if (numBytes == 0 && feof(fileToPlay_)) {
            bufferPool_.push(streamBuffer);
            break;
        }
        if (numBytes != size && !feof(fileToPlay_)) {
            std::cout << "can't read required bytes, read " << numBytes << std::endl;
            bufferPool_.push(streamBuffer);
            break;
        }

        streamBuffer->setDataSize(numBytes);

        status = audioPlayStream_->write(streamBuffer, writeCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "can't write, err " << static_cast<unsigned int>(status) << std::endl;
            break;
        }

        waitResult = false;
        waitResult = cv_.wait_for(lock,
            std::chrono::seconds(TIME_10_SECONDS),
            [=] { return (!bufferPool_.empty() || errorOccurred_); });

        if (!waitResult) {
            std::cout << "timedout " << std::endl;
            break;
        }
        if (errorOccurred_) {
            break;
        }
    }

    /* Before closing the file, wait for all responses */
    while (bufferPool_.size() != static_cast<uint32_t>(BUFFER_POOL_SIZE)) {
        cv_.wait(lock);
    }

    std::fclose(fileToPlay_);

    if (errorOccurred_) {
        std::cout << "playback terminated with error" << std::endl;
        return;
    }
    std::cout << "Playback finished" << std::endl;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<BTHFGRingtone> app;

    if (argc < 2) {
        std::cout << "Need audio file absolute path" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<BTHFGRingtone>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate BTHFGRingtone" << std::endl;
        return -ENOMEM;
    }

    app->fileToPlayPath_ = argv[1];

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->createPlayStream();
    if (ret < 0) {
        return ret;
    }

    std::thread playWorker(&BTHFGRingtone::play, &(*app));
    playWorker.join();

    ret = app->deletePlayStream();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
