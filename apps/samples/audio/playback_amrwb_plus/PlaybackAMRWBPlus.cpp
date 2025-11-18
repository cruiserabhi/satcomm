/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * Steps to play AMR-WB+ audio samples on a audio sink device are:
 *
 * 1. Get an AudioFactory instance.
 * 2. Get an IAudioManager instance from the AudioFactory.
 * 3. Wait for the audio service to become available.
 * 4. Create a playback stream (IAudioPlayStream).
 * 5. Start writing audio samples on the playback stream.
 * 6. When the playback is over, delete the playback stream.
 *
 * Usage:
 * # playback_amrwb_plus /data/audiofile.amrwbp
 *
 * Contents of /data/audiofile.amrwbp file are played on the speaker.
 *
 * Note: The AMR header should have been stripped from the audiofile.amrwbp file.
 */

#include <errno.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "PlaybackAMRWBPlus.hpp"

/*
 * Initialize application and get an audio service.
 */
int PlaybackAMRWBPlus::init() {

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
int PlaybackAMRWBPlus::createPlayStream() {

    telux::common::ErrorCode ec;
    telux::common::Status status;
    telux::audio::StreamConfig sc{};
    telux::audio::AmrwbpParams amrParams{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::PLAY;
    sc.sampleRate = 16000;
    sc.format = telux::audio::AudioFormat::AMRWB_PLUS;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);

    amrParams.frameFormat = telux::audio::AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
    sc.formatParams = &amrParams;

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
        std::cout << "can't request create stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    status = audioPlayStream_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't register listener"  << std::endl;
        return -EIO;
    }

    std::cout << "Stream created" << std::endl;
    return 0;
}

/*
 *  Step - 6, delete playback stream.
 */
int PlaybackAMRWBPlus::deletePlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioPlayStream_->deRegisterListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't deregister listener"  << std::endl;
        return -EIO;
    }

    status = audioManager_->deleteStream(audioPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete stream" << static_cast<int>(status) << std::endl;
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
void PlaybackAMRWBPlus::writeComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    long offset;

    std::lock_guard<std::mutex> lock(playMutex_);

    if (error != telux::common::ErrorCode::SUCCESS) {
        errorOccurred_ = true;
        std::cout << "write failed, err " << static_cast<int>(error) << std::endl;
    } else if (buffer->getDataSize() != bytesWritten) {
        /* Application should wait for onReadyForWrite() invocation */
        offset = (-1) * (static_cast<long>((buffer->getDataSize() - bytesWritten)));
        std::fseek(fileToPlay_, offset, SEEK_CUR);
        frameworkReadyForNextWrite_ = false;
    } else {
        /* Success, send the next buffer to play */
    }

    bufferPool_.push(buffer);
    writeWaitCv_.notify_all();
}

/*
 *  Gets called to indicate, next buffer can be sent now for playback.
 */
void PlaybackAMRWBPlus::onReadyForWrite() {
    std::lock_guard<std::mutex> lock(playMutex_);
    frameworkReadyForNextWrite_ = true;
    writeWaitCv_.notify_all();
}

void PlaybackAMRWBPlus::onPlayStopped() {
    std::cout << "playback stopped" << std::endl;
    playStopCv_.notify_all();
}

/*
 *  Step - 5, write samples on the playback stream.
 */
void PlaybackAMRWBPlus::play() {

    uint32_t size = 0;
    uint32_t numBytes = 0;
    telux::common::ErrorCode ec;
    telux::common::Status status;
    std::promise<telux::common::ErrorCode> p{};
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    std::unique_lock<std::mutex> lock(playMutex_);

    errorOccurred_ = false;
    frameworkReadyForNextWrite_ = true;

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

    auto writeCb = std::bind(&PlaybackAMRWBPlus::writeComplete, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::cout << "playback started" << std::endl;

    while (1) {
        if (frameworkReadyForNextWrite_ && !bufferPool_.empty()) {

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
                std::cout << "can't write, err " << static_cast<int>(status) << std::endl;
                bufferPool_.push(streamBuffer);
                break;
            }

            if (errorOccurred_) {
                break;
            }
        } else {
            writeWaitCv_.wait(lock);
        }
    }

    /* Before closing the file, wait for all responses */
    while (bufferPool_.size() != static_cast<uint32_t>(BUFFER_POOL_SIZE)) {
        writeWaitCv_.wait(lock);
    }

    std::fclose(fileToPlay_);

    if (errorOccurred_) {
        std::cout << "playback terminated with error" << std::endl;
        return;
    }

    /* wait till the very last buffer is played */
    {
      std::unique_lock<std::mutex> stopLock(playStopMutex_);

      status = audioPlayStream_->stopAudio(telux::audio::StopType::STOP_AFTER_PLAY,
              [&p] (telux::common::ErrorCode error) {
          p.set_value(error);
      });

      if (status == telux::common::Status::SUCCESS){
            ec = p.get_future().get();
            if (ec != telux::common::ErrorCode::SUCCESS) {
                std::cout << "can't finish playback, err " << static_cast<int>(ec) << std::endl;
                return;
            }
            playStopCv_.wait(stopLock);
        } else {
            std::cout << "can't stop playback" << std::endl;
        }
    }

    std::cout << "Playback finished" << std::endl;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<PlaybackAMRWBPlus> app;

    if (argc < 2) {
        std::cout << "Need audio file's absolute path" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<PlaybackAMRWBPlus>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate PlaybackAMRWBPlus" << std::endl;
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

    std::thread playWorker(&PlaybackAMRWBPlus::play, &(*app));
    playWorker.join();

    ret = app->deletePlayStream();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
