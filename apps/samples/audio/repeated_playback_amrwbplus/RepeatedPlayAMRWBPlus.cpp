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
 * This sample application demonstrates, how to configure audio streams for each file and
 * how to define multiple AMR-WB+ files to play repeatedly. The steps are as follows:
 *
 * 1. Get an AudioFactory instance.
 * 2. Get an IAudioPlayer instance from the the AudioFactory.
 * 3. Implement all listener methods from IPlayListListener class.
 * 4. Define parameters to configure audio stream.
 * 5. Define how a given file should be played.
 * 6. Start playing the files.
 * 7. When the use case is over, stop the playback.
 *
 * Usage:
 * # repeated_playback_amrwbplus
 *
 * File /data/prompt1.awbp is played once and file /data/prompt2.awbp is played
 * indefinitely on the local speaker. Files are mono channel in AMR-WB+ format.
 */

#include <errno.h>

#include <cstdio>
#include <chrono>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "RepeatedPlayAMRWBPlus.hpp"

/*
 * Initialize application and get an audio service.
 */
int RepeatedPlayAMRWBPlus::init() {

    telux::common::ErrorCode ec;

    /* Step - 1 */
    auto &audioFactory = telux::audio::AudioFactory::getInstance();

    /* Step - 2 */
    ec = audioFactory.getAudioPlayer(audioPlayer_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get IAudioPlayer" << std::endl;
        return -ENOMEM;
    }
    
    std::cout << "Initialization finished" << std::endl;
    return 0;
}

/*
 * Configure the audio stream, define how to play files and start the playback.
 */
int RepeatedPlayAMRWBPlus::start(
        std::shared_ptr<RepeatedPlayAMRWBPlus> statusListener) {

    bool waitResult = false;
    telux::common::ErrorCode ec;
    telux::audio::StreamConfig sc{};
    telux::audio::AmrwbpParams amrParams1{};
    telux::audio::AmrwbpParams amrParams2{};
    telux::audio::PlaybackConfig pbCfg1{};
    telux::audio::PlaybackConfig pbCfg2{};
    std::vector<telux::audio::PlaybackConfig> pbConfigs;

    /* Step - 4 */
    amrParams1.frameFormat = telux::audio::AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
    pbCfg1.streamConfig.formatParams = &amrParams1;
    pbCfg1.streamConfig.type = telux::audio::StreamType::PLAY;
    pbCfg1.streamConfig.sampleRate = 16000;
    pbCfg1.streamConfig.format = telux::audio::AudioFormat::AMRWB_PLUS;
    pbCfg1.streamConfig.channelTypeMask = telux::audio::ChannelType::LEFT;
    pbCfg1.streamConfig.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);

    amrParams2.frameFormat = telux::audio::AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
    pbCfg2.streamConfig.formatParams = &amrParams2;
    pbCfg2.streamConfig.type = telux::audio::StreamType::PLAY;
    pbCfg2.streamConfig.sampleRate = 16000;
    pbCfg2.streamConfig.format = telux::audio::AudioFormat::AMRWB_PLUS;
    pbCfg2.streamConfig.channelTypeMask = telux::audio::ChannelType::LEFT;
    pbCfg2.streamConfig.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);

    /* Step - 5 */
    /* Play this file only once */
    pbCfg1.absoluteFilePath = "/data/prompt1.awbp";
    pbCfg1.repeatInfo.type = telux::audio::RepeatType::COUNT;
    pbCfg1.repeatInfo.count = 1;
    pbConfigs.push_back(pbCfg1);

    /* Play this file repeatedly */
    pbCfg2.absoluteFilePath = "/data/prompt2.awbp";
    pbCfg2.repeatInfo.type = telux::audio::RepeatType::INDEFINITELY;
    pbConfigs.push_back(pbCfg2);

   {
    /* First acquire the lock then reset predicates */
    std::unique_lock<std::mutex> startPlayLock(playMutex_);

    playStarted_ = false;
    errorOccurred_ = false;

    /* Step - 6 */
    ec = audioPlayer_->startPlayback(pbConfigs, statusListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed start, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    /*
     * Optional:
     * If application requires confirmation that playback has started, wait for
     * the acknowledgement. This wait finishes when any of these condition is met:
     * (a) An error occurred such that playback can't be started
     * (b) Before playback started, application stopped the playback explicitly
     * (c) 5 second timeout occured
     */
    waitResult = playCV_.wait_for(startPlayLock,
        std::chrono::milliseconds(5000),
        [=]{return (playStarted_ || errorOccurred_);});

    if (!waitResult) {
        std::cout << "start timed out" << std::endl;
        return -ETIME;
    }

    return errorOccurred_ ? -EIO : 0;
   }
}

/*
 * Wait for the playback to complete.
 *
 * Optional:
 * Application thread can block waiting for the playback to complete or
 * it can perform other task. In this example, we want to play a file
 * repeatedly for 3 minutes therefore a timed wait is used.
 */
int RepeatedPlayAMRWBPlus::wait() {

    bool waitResult = false;

    std::unique_lock<std::mutex> waitPlayLock(playMutex_);

    if (playFinished_) {
        std::cout << "playback already finished" << std::endl;
        return 0;
    }

    playFinished_ = false;
    playStopped_ = false;
    errorOccurred_ = false;

    waitResult = playCV_.wait_for(waitPlayLock,
        std::chrono::minutes(3),
        [=]{return (playFinished_ || errorOccurred_ || playStopped_);});

    if (!waitResult) {
        /* 3 minutes elapsed */
        std::cout << "wait complete" << std::endl;
        return 0;
    }

    return errorOccurred_ ? -EIO : 0;
}



/*
 * When the use case is over, stop the playback.
 */
int RepeatedPlayAMRWBPlus::stop() {

    telux::common::ErrorCode ec;
    bool waitResult = false;

    std::unique_lock<std::mutex> stopPlayLock(playMutex_);

    if (playFinished_ || playStopped_) {
        std::cout << "playback already stopped/completed" << std::endl;
        return 0;
    }

    playStopped_ = false;
    errorOccurred_ = false;

    /* Step - 7 */
    ec = audioPlayer_->stopPlayback();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        if (ec == telux::common::ErrorCode::INVALID_STATE) {
            std::cout << "no playback in progress" << std::endl;
            return 0;
        }
        std::cout << "failed stop, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    /*
     * Optional:
     * After calling stopPlayback(), playback will stop and invoke onPlaybackStopped().
     * Application thread can perform other tasks or can wait for onPlaybackStopped()
     * invocation. In this example we are waiting for 5 seconds. This wait finishes
     * when any of these condition is met:
     * (a) An error occured during playback
     * (b) Playback stopped
     * (c) 5 second timeout occurred
     */
    waitResult = playCV_.wait_for(stopPlayLock,
        std::chrono::milliseconds(5000),
        [=]{return (playStopped_ || errorOccurred_);});

    if (!waitResult) {
        std::cout << "stop timed out" << std::endl;
        return -ETIME;
    }

    return errorOccurred_ ? -EIO : 0;
}

/*
 * Step - 3
 * Invoked as a response to IAudioPlayer::startPlaying(). Marks playback started.
 */
void RepeatedPlayAMRWBPlus::onPlaybackStarted() {

    std::cout << "playback started" << std::endl;

    std::lock_guard<std::mutex> startPlayLock(playMutex_);
    playStarted_ = true;
    playCV_.notify_all();
}

/*
 * Step - 3
 * Invoked as a response to IAudioPlayer::stopPlaying(). Marks playback is terminated.
 */
void RepeatedPlayAMRWBPlus::onPlaybackStopped() {

    std::cout << "playback stopped" << std::endl;

    std::lock_guard<std::mutex> stoplock(playMutex_);
    playStopped_ = true;
    playCV_.notify_all();
}

/*
 * Step - 3
 * Invoked whenever error occurs during playback.
 */
void RepeatedPlayAMRWBPlus::onError(telux::common::ErrorCode error, std::string file) {

    std::string fileName = "";
    if (!file.empty()) {
        fileName += ", file: ";
        fileName += file;
    }

    std::cout << "error encountered: " << static_cast<int>(error) << fileName << std::endl;

    /* stop playback on error */
    std::lock_guard<std::mutex> errorLock(playMutex_);
    errorOccurred_ = true;
    playCV_.notify_all();
}

/*
 * Step - 3
 * Invoked whenever a file is played successfully.
 */
void RepeatedPlayAMRWBPlus::onFilePlayed(std::string file) {

    std::cout << "played " << file << std::endl;
}

/*
 * Step - 3
 * Invoked whenever playback finished completely.
 */
void RepeatedPlayAMRWBPlus::onPlaybackFinished() {

    std::cout << "playback finished" << std::endl;

    std::lock_guard<std::mutex> finishLock(playMutex_);
    playFinished_ = true;
    playCV_.notify_all();
}

/*
 * Application entry.
 */
int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<RepeatedPlayAMRWBPlus> repeatPlay;

    try {
        repeatPlay = std::make_shared<RepeatedPlayAMRWBPlus>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate RepeatedPlayAMRWBPlus" << std::endl;
        return -ENOMEM;
    }

    ret = repeatPlay->init();
    if (ret < 0) {
        return ret;
    }

    ret = repeatPlay->start(repeatPlay);
    if (ret < 0) {
        return ret;
    }

    ret = repeatPlay->wait();
    if (ret < 0) {
        return ret;
    }

    ret = repeatPlay->stop();
    if (ret < 0) {
        return ret;
    }

    std::cout << "repeat playback done" << std::endl;
    return 0;
}
