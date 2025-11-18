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
 * Steps to play audio samples during an active voice call are:
 *
 *  1. Get an AudioFactory instance.
 *  2. Get an IAudioManager instance from the AudioFactory.
 *  3. Wait for the audio service to become available.
 *  4. Create a voice call stream (IAudioVoiceStream).
 *  5. Start voice call stream.
 *  6. Create a playback stream (IAudioPlayStream).
 *  7. Start writing audio samples on the playback stream.
 *  8. When the playback is over, delete the playback stream.
 *  9. Stop voice call stream.
 * 10. Delete voice call stream.
 *
 * Usage:
 * # in_call_playback_pcm /data/musicfile.raw
 *
 * Contents of /data/musicfile.raw file played on the device is heard on the far end.
 * Voice call must be active (answered) between local end and far end.
 */

#include <errno.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "InCallPlaybackPCM.hpp"

/*
 * Initialize application and get an audio service.
 */
int InCallPlaybackPCM::init() {

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
int InCallPlaybackPCM::createVoiceStream() {

    telux::common::ErrorCode ec;
    telux::common::Status status;
    telux::audio::StreamConfig sc{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::VOICE_CALL;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

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

    std::cout << "Voice call stream created" << std::endl;
    return 0;
}

/*
 *  Step - 10, delete voice call stream.
 */
int InCallPlaybackPCM::deleteVoiceStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

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

    std::cout << "Voice call stream deleted" << std::endl;
    return 0;
}

/*
 *  Step - 5, start voice call stream.
 */
int InCallPlaybackPCM::startVoiceStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

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

    std::cout << "Voice call stream started" << std::endl;
    return 0;
}

/*
 * Step - 9, stop voice call stream.
 */
int InCallPlaybackPCM::stopVoiceStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

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

    std::cout << "Voice call stream stopped" << std::endl;
    return 0;
}

/*
 * Step - 6, create a incall-playback stream.
 * Audio device is not specified. Voice uplink is specified.
 */
int InCallPlaybackPCM::createIncallPlayStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    telux::audio::StreamConfig sc{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::PLAY;
    sc.sampleRate = 48000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

    /* Direction::TX indicates voice uplink playback */
    sc.voicePaths.emplace_back(telux::audio::Direction::TX);

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
        std::cout << "failed create playback stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Playback stream created" << std::endl;
    return 0;
}

/*
 *  Step - 8, delete playback stream.
 */
int InCallPlaybackPCM::deleteIncallPlayStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

    status = audioManager_->deleteStream(audioPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete playback stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete playback stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Playback stream deleted" << std::endl;
    return 0;
}

/*
 *  Gets called to confirm how many bytes were actually written to the playback stream.
 */
void InCallPlaybackPCM::writeComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    long offset;

    std::lock_guard<std::mutex> lock(playMutex_);

    if (error != telux::common::ErrorCode::SUCCESS) {
        errorOccurred_ = true;
        std::cout << "write failed, err " << static_cast<int>(error) << std::endl;
    } else if (buffer->getDataSize() != bytesWritten) {
        /* Whole buffer can't be played successfully.
         * It is an application owner's decision, what to do if an error occurs
         * in writing; resend buffer for playback or terminate the playback.
         * In this example we are resending. */
        offset = (-1) * (static_cast<long>((buffer->getDataSize() - bytesWritten)));
        std::fseek(fileToPlay_, offset, SEEK_CUR);
    } else {
        /* Success, send the next buffer to play */
    }

    bufferPool_.push(buffer);
    cv_.notify_all();
}

/*
 *  Step - 7, write samples on the playback stream.
 */
void InCallPlaybackPCM::play() {

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

    auto writeCb = std::bind(&InCallPlaybackPCM::writeComplete, this,
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
    std::cout << "playback finished" << std::endl;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<InCallPlaybackPCM> app;

    if (argc < 2) {
        std::cout << "Need audio file's absolute path" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<InCallPlaybackPCM>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate InCallPlaybackPCM" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    app->fileToPlayPath_ = argv[1];

    ret = app->createVoiceStream();
    if (ret < 0) {
        return ret;
    }

    ret = app->startVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->createIncallPlayStream();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    std::thread playWorker(&InCallPlaybackPCM::play, &(*app));
    playWorker.join();

    ret = app->deleteIncallPlayStream();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

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
