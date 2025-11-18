/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "RepeatedPlaybackMenu.hpp"
#include "../../common/utils/Utils.hpp"


RepeatedPlaybackMenu::RepeatedPlaybackMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor),
      playFinished_(false),
      errorOccurred_(false),
      playStopped_(false),
      playStarted_(false),
      audioPlayerReady_(false) {
      }

RepeatedPlaybackMenu::~RepeatedPlaybackMenu() {
    cleanup();
}

void RepeatedPlaybackMenu::setSystemReady() {
    std::lock_guard<std::mutex> lock(readyMutex_);
    audioPlayerReady_ = true;
}

void RepeatedPlaybackMenu::cleanup() {
    std::string enableLogs = "0";
    std::vector<std::string> input = {"0"};
    input.push_back(enableLogs);
    stopPlayAudioFiles(input);
    clearPlaylist(input);
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        audioPlayerReady_ = false;
    }
    {
        std::lock_guard<std::mutex> lock(playMutex_);
        playStopped_ = false;
        playStarted_ = false;
        playFinished_ = false;
    }

}

bool RepeatedPlaybackMenu::init() {
    bool initStatus = initAudioPlayerManager();

    if (!initStatus) {
        std::cout << "can't initialize audio player"<< std::endl;
        return false;
    }

    std::shared_ptr<ConsoleAppCommand> addToPlaylistCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Add Files to Playlist", {},
            std::bind(&RepeatedPlaybackMenu::addToPlaylist, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> clearPlaylistCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Clear Playlist", {},
            std::bind(&RepeatedPlaybackMenu::clearPlaylist, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startPlayCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Start Play", {},
            std::bind(&RepeatedPlaybackMenu::startPlayAudioFiles, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopPlayCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Stop Play", {},
            std::bind(&RepeatedPlaybackMenu::stopPlayAudioFiles, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Set Volume", {},
            std::bind(&RepeatedPlaybackMenu::setVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Get Volume", {},
            std::bind(&RepeatedPlaybackMenu::getVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Set Mute", {},
            std::bind(&RepeatedPlaybackMenu::setMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Get Mute", {},
            std::bind(&RepeatedPlaybackMenu::getMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Set Device", {},
            std::bind(&RepeatedPlaybackMenu::setDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Get Device", {},
            std::bind(&RepeatedPlaybackMenu::getDevice, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> repeatedPlaybackMenuCommandsList =
    { addToPlaylistCommand, clearPlaylistCommand, startPlayCommand, stopPlayCommand,
      setVolumeCommand, getVolumeCommand, setMuteCommand, getMuteCommand, setDeviceCommand,
      getDeviceCommand };

    {
        std::lock_guard<std::mutex> startPlayLock(readyMutex_);
        audioPlayerReady_ = initStatus;
    }

    ConsoleApp::addCommands(repeatedPlaybackMenuCommandsList);

    return true;
}

bool RepeatedPlaybackMenu::initAudioPlayerManager() {

    auto &audioFactory = telux::audio::AudioFactory::getInstance();
    auto ec = audioFactory.getAudioPlayer(audioPlayerMgr_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get IAudioPlayer" << std::endl;
        return false;
    }

    std::cout << "\n" << " AudioPlayer is ready" << std::endl;
    return true;
}

void RepeatedPlaybackMenu::addToPlaylist(std::vector<std::string> userInput) {

    unsigned int audioFormat = 0;
    unsigned int option = 0;
    unsigned int repeatCount = 0;
    unsigned int sr = 0;
    unsigned int deviceType = 0;
    unsigned int channelType = 0;
    bool addMore = false;
    bool onlyOnce = false;
    telux::audio::ChannelTypeMask channelMask;
    telux::audio::DeviceType devType;
    std::string filePath;

    {
        std::lock_guard<std::mutex> addPlaylistLock(playMutex_);

        do {
            telux::audio::StreamConfig streamConfig;
            telux::audio::PlaybackConfig pbCfg;

            streamConfig.type = telux::audio::StreamType::PLAY;

            std::cout << "Enter file to play (absolute path): ";
            std::cin >> filePath;
            Utils::validateInput(filePath);

            pbCfg.absoluteFilePath = filePath;

            std::cout << std::endl;
            std::cout << "Enter how many times to play this file "
                << "(1-skip, 2-play indefinitely, 3-play certain numer of times): ";
            std::cin >> option;
            Utils::validateInput<unsigned int>(option, {1, 2, 3});

            if (option == 1) {
                pbCfg.repeatInfo.type = telux::audio::RepeatType::SKIP;
            } else if (option == 2) {
                pbCfg.repeatInfo.type = telux::audio::RepeatType::INDEFINITELY;
            } else {
                std::cout << std::endl;
                std::cout << "Enter count: ";
                std::cin >> repeatCount;
                pbCfg.repeatInfo.type = telux::audio::RepeatType::COUNT;
                pbCfg.repeatInfo.count = repeatCount;
            }

            std::cout << std::endl;
            std::cout << "Enter how stream should be created to play this file: "
                << "(1-PCM_16BIT_SIGNED, 2-AMRNB, 3-AMRWB, 4-AMRWB_PLUS): ";
            std::cin >> audioFormat;

            Utils::validateInput<unsigned int>(audioFormat, {1, 2, 3, 4});
            if (audioFormat == 1) {
                streamConfig.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
                std::cout << std::endl;
                std::cout << "Enter sampling rate :(for ex; 8k/16k/32k/48k): ";
                std::cin >> sr;
                Utils::validateInput(sr);
                streamConfig.sampleRate = sr;
            } else if(audioFormat == 2) {
                streamConfig.format = telux::audio::AudioFormat::AMRNB;
            } else if(audioFormat ==3) {
                streamConfig.format = telux::audio::AudioFormat::AMRWB;
            } else {
                streamConfig.format = telux::audio::AudioFormat::AMRWB_PLUS;
            }

            if (!onlyOnce) {
                std::cout << std::endl;
                std::cout << "Enter sink device :(for ex; 1-DEVICE_TYPE_SPEAKER): ";
                std::cin >> deviceType;
                Utils::validateInput<unsigned int>(deviceType, {1, 2, 3});
                devType = static_cast<telux::audio::DeviceType>(deviceType);
                onlyOnce = true;
            }

            std::cout << std::endl;
            std::cout << "Enter channel type :(0-LEFT, 1-RIGHT, 2-BOTH): ";
            std::cin >> channelType;
            Utils::validateInput<unsigned int>(channelType, {0, 1, 2});
            channelMask = (1 << channelType);
            if (channelType == 2) {
                channelMask = (1 << 0) | (1 << 1);
            }
            streamConfig.channelTypeMask = channelMask;

            streamConfig.deviceTypes.emplace_back(devType);

            pbCfg.streamConfig = streamConfig;
            pbConfigs_.push_back(pbCfg);

            std::cout << std::endl;
            std::cout << "Do you want to add more files :(0-NO, 1-YES): ";
            std::cin >> addMore;
            std::cout << std::endl;
        } while(addMore);
    }

    std::cout << "playlist added"<< std::endl;
}

void RepeatedPlaybackMenu::clearPlaylist(std::vector<std::string> userInput) {
    {
        std::lock_guard<std::mutex> clearPlaylistLock(playMutex_);
        pbConfigs_.clear();
    }


    if (!userInput.empty() && userInput[0] == "0") {
        return;
    }

    std::cout << "playlist cleared"<< std::endl;
}

void RepeatedPlaybackMenu::startPlayAudioFiles(std::vector<std::string> userInput) {

    telux::common::ErrorCode ec;
    {
        std::unique_lock<std::mutex> startPlayLock(playMutex_);

        if (playStarted_) {
            std::cout << "playback already started"<< std::endl;
            return;
        }

        playStarted_ = false;
        errorOccurred_ = false;

        ec = audioPlayerMgr_->startPlayback(pbConfigs_, shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "failed start, err " << static_cast<int>(ec) << std::endl;
            return;
        }

        /*
         * Optional:
         * If application requires confirmation that playback has started, wait for
         * the acknowledgement. This wait finishes when any of these condition is met:
         * (a) An error occurred such that playback can't be started
         * (b) Before playback started, application stopped the playback explicitly
         * (c) 5 second timeout occured
         */
        auto waitResult = playCV_.wait_for(startPlayLock,
                std::chrono::milliseconds(5000),
                [=]{return (playStarted_ || errorOccurred_);});

        if (!waitResult) {
            std::cout << "start timed out" << std::endl;
            return;
        }

        errorOccurred_ ? (ec = playError_) : (ec = telux::common::ErrorCode::SUCCESS);
    }
}

void RepeatedPlaybackMenu::stopPlayAudioFiles(std::vector<std::string> userInput) {

    telux::common::ErrorCode ec;
    bool waitResult = false;
    bool enableLogs = true;

    if (!userInput.empty() && userInput[0] == "0") {
        enableLogs = false;
    }

    std::unique_lock<std::mutex> stopPlayLock(playMutex_);

    if (playStopped_ && enableLogs) {
        std::cout << "playback already stopped" << std::endl;
        return;
    }

    playStopped_ = false;
    errorOccurred_ = false;

    ec = audioPlayerMgr_->stopPlayback();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        if (ec == telux::common::ErrorCode::INVALID_STATE && enableLogs) {
            std::cout << "no playback in progress" << std::endl;
            return;
        }

        if (enableLogs) {
            std::cout << "failed stop, err " << static_cast<int>(ec) << std::endl;
        }
        return;
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
        return;
    }

    errorOccurred_ ? (ec = playError_) : (ec = telux::common::ErrorCode::SUCCESS);
}

void RepeatedPlaybackMenu::setVolume(std::vector<std::string> userInput) {

    float volume = 0;

    do {
        std::cout << "Enter Volume (VALID: 0.1 to 1.0):" << std::endl;
        std::cin >> volume;
    } while((volume < 0.0f) || (volume > 1.0f));

    auto ec = audioPlayerMgr_->setVolume(volume);

    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "volume set failed, ec: " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "volume set succeed" << std::endl;
}

void RepeatedPlaybackMenu::getVolume(std::vector<std::string> userInput) {

    float volume = 0;

    auto ec = audioPlayerMgr_->getVolume(volume);

    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "volume get failed, ec: " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "volume: " << volume << std::endl;
}

void RepeatedPlaybackMenu::setMute(std::vector<std::string> userInput) {

    bool mute;

    std::cout << "Enter mute (0-UNMUTE, 1-MUTE): " << std::endl;
    std::cin >> mute;
    Utils::validateInput<bool>(mute, {0, 1});

    auto ec = audioPlayerMgr_->setMute(mute);

    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "set mute failed, ec: " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "set mute succeed" << std::endl;
}

void RepeatedPlaybackMenu::getMute(std::vector<std::string> userInput) {

    bool mute;

    auto ec = audioPlayerMgr_->getMute(mute);

    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "get mute failed, ec: " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "current mute status: " << mute << std::endl;
}

void RepeatedPlaybackMenu::setDevice(std::vector<std::string> userInput) {

    bool addMore = false;
    unsigned int deviceType;
    std::vector<DeviceType> devices;

    do {
        std::cout << "Enter device (for ex; 1-DEVICE_TYPE_SPEAKER): " << std::endl;
        std::cin >> deviceType;
        Utils::validateInput<unsigned int>(deviceType, {1, 2, 3});

        devices.emplace_back(static_cast<telux::audio::DeviceType>(deviceType));

        std::cout << "Add more devices ?: (0-NO, 1-YES): " << std::endl;
        std::cin >> addMore;
    } while(addMore);

    auto ec = audioPlayerMgr_->setDevice(devices);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "set device failed, ec: " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "set device succeed" << std::endl;
}

void RepeatedPlaybackMenu::getDevice(std::vector<std::string> userInput) {
    std::vector<DeviceType> devices;

    auto ec = audioPlayerMgr_->getDevice(devices);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "get device failed, ec: " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Devices:";

    for (auto device : devices) {
        std::cout << ", "  << static_cast<int>(device);
    }
    std::cout << std::endl;
}

void RepeatedPlaybackMenu::onPlaybackStarted() {

    std::cout << "playback started" << std::endl;

    std::lock_guard<std::mutex> startPlayLock(playMutex_);
    playStarted_ = true;
    playStopped_ = false;
    playFinished_ = false;
    playCV_.notify_all();
}

/*
 * Invoked as a response to IAudioPlayer::stopPlaying(). Marks playback is terminated.
 */
void RepeatedPlaybackMenu::onPlaybackStopped() {

    std::cout << "playback stopped" << std::endl;

    std::lock_guard<std::mutex> stoplock(playMutex_);
    playStopped_ = true;
    playStarted_ = false;
    playFinished_ = false;
    playCV_.notify_all();
}

void RepeatedPlaybackMenu::onError(telux::common::ErrorCode error, std::string file) {

    std::string fileName = "";
    if (!file.empty()) {
        fileName += ", file: ";
        fileName += file;
    }

    std::cout << "error encountered: " << static_cast<int>(error) << fileName << std::endl;

    /* stop playback on error */
    std::lock_guard<std::mutex> errorLock(playMutex_);
    playError_ = error;
    errorOccurred_ = true;
    playCV_.notify_all();
}

/*
 * Invoked whenever a file is played successfully.
 */
void RepeatedPlaybackMenu::onFilePlayed(std::string file) {

    std::cout << "played " << file << std::endl;
}

/*
 * Invoked whenever playback finished completely.
 */
void RepeatedPlaybackMenu::onPlaybackFinished() {

    std::cout << "playback finished" << std::endl;

    std::lock_guard<std::mutex> finishLock(playMutex_);
    playFinished_ = true;
    playStopped_ = false;
    playStarted_ = false;
    playCV_.notify_all();
}

