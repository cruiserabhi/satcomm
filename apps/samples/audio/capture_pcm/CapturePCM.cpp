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
 * Steps to capture audio samples from an audio source are:
 *
 * 1. Get an AudioFactory instance.
 * 2. Get an IAudioManager instance from the AudioFactory.
 * 3. Wait for the audio service to become available.
 * 4. Create a capture stream (IAudioCaptureStream).
 * 5. Start reading audio samples from capture stream.
 * 6. When required samples have been captured, delete the capture stream.
 *
 * Usage:
 * # capture_pcm <duration> <absolute-file-path>
 *
 * Raw audio samples are captured for the given <duration> (in seconds) and saved
 * on <absolute-file-path> file.
 */

#include <errno.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

#include <telux/audio/AudioFactory.hpp>

#include "CapturePCM.hpp"

/*
 * Initialize application and get an audio service.
 */
int CapturePCM::init() {

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
 * Step - 4, create a capture stream.
 */
int CapturePCM::createCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.sampleRate = 48000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            audioCaptureStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(audioStream);
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

    std::cout << "Stream created" << std::endl;
    return 0;
}

/*
 *  Step - 6, delete capture stream.
 */
int CapturePCM::deleteCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(audioCaptureStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete stream"  << std::endl;
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
 *  Gets called whenever audio samples are read from the capture stream.
 */
void CapturePCM::readComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    uint32_t bytesRead, bytesWrittenToFile;

    std::lock_guard<std::mutex> lock(captureMutex_);

    if (error != telux::common::ErrorCode::SUCCESS) {
        errorOccurred_ = true;
        std::cout << "read failed, err: " << static_cast<int>(error) << std::endl;
    } else {
        bytesRead = buffer->getDataSize();
        bytesWrittenToFile = std::fwrite(buffer->getRawBuffer(), 1, bytesRead, fileToSaveSamples_);
        if (bytesWrittenToFile != bytesRead) {
            std::cout << "can't write to file, " << "written "
            << bytesWrittenToFile << ", read " << bytesRead << std::endl;
        }
    }

    bufferPool_.push(buffer);
    cv_.notify_all();
}

/*
 *  Step - 5, read samples from the capture stream.
 */
void CapturePCM::capture() {

    uint32_t bytesToRead = 0;
    bool waitResult = false;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    std::unique_lock<std::mutex> lock(captureMutex_);

    errorOccurred_ = false;

    fileToSaveSamples_ = std::fopen(fileToSaveSamplesPath_, "wb");
    if (!fileToSaveSamples_) {
        std::cout << "can't open file " << fileToSaveSamplesPath_ << std::endl;
        return;
    }

    for (int x = 0; x < BUFFER_POOL_SIZE; x++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            std::fclose(fileToSaveSamples_);
            bufferPool_ = {};
            return;
        }

        bufferPool_.push(streamBuffer);

        bytesToRead = streamBuffer->getMinSize();
        if (!bytesToRead) {
            bytesToRead =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(bytesToRead);
    }

    auto readCb = std::bind(&CapturePCM::readComplete, this,
        std::placeholders::_1, std::placeholders::_2);

    std::cout << "capture started" << std::endl;

    auto startTime = std::chrono::steady_clock::now();

    while (1) {
        streamBuffer = bufferPool_.front();
        bufferPool_.pop();

        status = audioCaptureStream_->read(streamBuffer, bytesToRead, readCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "can't read, err " << static_cast<int>(status) << std::endl;
            bufferPool_.push(streamBuffer);
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

        auto currentTime = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime).count();

        if (diff >= captureDurationMs_) {
            /* Let all initiated read complete, buffers saved to file */
            while (bufferPool_.size() != static_cast<uint32_t>(BUFFER_POOL_SIZE)) {
                cv_.wait(lock);
            }
            break;
        }
    }

    std::fflush(fileToSaveSamples_);
    std::fclose(fileToSaveSamples_);

    if (errorOccurred_) {
        std::cout << "capture terminated with error" << std::endl;
        return;
    }
    std::cout << "Capture finished" << std::endl;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<CapturePCM> app;

    if (argc < 3) {
        std::cout << "Usage: capture_pcm <duration> <absolute-file-path>" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<CapturePCM>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate CapturePCM" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    try {
        app->captureDurationMs_ = std::stoul(argv[1]) * 1000;
    } catch (const std::exception& e) {
        std::cout << "can't interpret duration from " << argv[1] << std::endl;
        return -ERANGE;
    }

    app->fileToSaveSamplesPath_ = argv[2];

    ret = app->createCaptureStream();
    if (ret < 0) {
        return ret;
    }

    std::thread captureWorker(&CapturePCM::capture, &(*app));
    captureWorker.join();

    ret = app->deleteCaptureStream();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
