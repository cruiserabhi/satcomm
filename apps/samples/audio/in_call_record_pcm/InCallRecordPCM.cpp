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
 * Steps to record audio samples during an active voice call are:
 *
 *  1. Get an AudioFactory instance.
 *  2. Get an IAudioManager instance from AudioFactory.
 *  3. Wait for the audio service to become available.
 *  4. Create a voice call stream (IAudioVoiceStream).
 *  5. Start the voice call stream.
 *  6. Create a capture stream (IAudioCaptureStream).
 *  7. Start reading audio samples from capture stream.
 *  8. When the recording is complete, delete the capture stream.
 *  9. Stop voice call stream.
 * 10. Delete voice call stream.
 *
 * Usage:
 * # in_call_record_pcm <duration> <absolute-file-path>
 *
 * Audio data sent from the remote end is recorded for given <duration> (in seconds)
 * and saved on <absolute-file-path> file. Voice call must be active (answered)
 * between local and far end.
 */

#include <errno.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "InCallRecordPCM.hpp"

/*
 * Initialize application and get an audio service.
 */
int InCallRecordPCM::init() {

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
int InCallRecordPCM::createVoiceStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
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
int InCallRecordPCM::deleteVoiceStream() {

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
int InCallRecordPCM::startVoiceStream() {

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
int InCallRecordPCM::stopVoiceStream() {

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
 * Step - 6, create a incall-record stream.
 * Audio device is not specified. Voice downlink is specified.
 */
int InCallRecordPCM::createIncallRecordStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    telux::audio::StreamConfig sc{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.sampleRate = 48000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

    /* Direction::RX indicates voice downlink */
    sc.voicePaths.emplace_back(telux::audio::Direction::RX);

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
        std::cout << "can't create capture stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create capture stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Capture stream created" << std::endl;
    return 0;
}

/*
 *  Step - 8, delete capture stream.
 */
int InCallRecordPCM::deleteIncallRecordStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

    status = audioManager_->deleteStream(audioCaptureStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete capture stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete capture stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Capture stream deleted" << std::endl;
    return 0;
}

/*
 *  Gets called whenever audio samples are read from the capture stream.
 */
void InCallRecordPCM::readComplete(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    uint32_t bytesRead, bytesWrittenToFile;

    std::lock_guard<std::mutex> lock(captureMutex_);

    if (error != telux::common::ErrorCode::SUCCESS) {
        errorOccurred_ = true;
        std::cout << "read failed, err: " << static_cast<int>(error) << std::endl;
    } else {
        bytesRead = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(), 1, bytesRead, fileToSaveRecording_);
        if (bytesWrittenToFile != bytesRead) {
            std::cout << "can't write to file, " << "written "
            << bytesWrittenToFile << ", read " << bytesRead << std::endl;
        }
    }

    bufferPool_.push(buffer);
    cv_.notify_all();
}

/*
 *  Step - 7, read samples from the capture stream.
 */
void InCallRecordPCM::record() {

    uint32_t bytesToRead = 0;
    bool waitResult = false;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    std::unique_lock<std::mutex> lock(captureMutex_);

    errorOccurred_ = false;

    try {
        recordingDurationMs_ = (std::stoul(recordingDuration_)) * 1000;
    } catch (const std::exception& e) {
        std::cout << "can't interpret time " << recordingDuration_ << std::endl;
        return;
    }

    fileToSaveRecording_ = std::fopen(fileToSaveRecordingPath_, "wb");
    if (!fileToSaveRecording_) {
        std::cout << "can't open file " << fileToSaveRecordingPath_ << std::endl;
        return;
    }

    for (int x = 0; x < BUFFER_POOL_SIZE; x++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            std::fclose(fileToSaveRecording_);
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

    auto readCb = std::bind(&InCallRecordPCM::readComplete, this,
        std::placeholders::_1, std::placeholders::_2);

    std::cout << "recording started" << std::endl;

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

        if (diff >= recordingDurationMs_) {
            /* Let all initiated read complete, buffers saved to file */
            while (bufferPool_.size() != static_cast<uint32_t>(BUFFER_POOL_SIZE)) {
                cv_.wait(lock);
            }
            break;
        }
    }

    std::fflush(fileToSaveRecording_);
    std::fclose(fileToSaveRecording_);

    if (errorOccurred_) {
        std::cout << "recording terminated with error" << std::endl;
        return;
    }
    std::cout << "Recording finished" << std::endl;
}


int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<InCallRecordPCM> app;

    if (argc < 3) {
        std::cout << "Usage: in_call_record_pcm <duration> <absolute-file-path>" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<InCallRecordPCM>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate InCallRecordPCM" << std::endl;
        return -ENOMEM;
    }

    app->recordingDuration_ = argv[1];
    app->fileToSaveRecordingPath_ = argv[2];

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

    ret = app->createIncallRecordStream();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return -EIO;
    }

    std::thread recordWorker(&InCallRecordPCM::record, &(*app));
    recordWorker.join();

    ret = app->deleteIncallRecordStream();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return -EIO;
    }

    ret = app->stopVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return -EIO;
    }

    ret = app->deleteVoiceStream();
    if (ret < 0) {
        return -EIO;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
