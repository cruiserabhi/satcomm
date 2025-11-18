/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef REPEATEDPLAYBACKMENU_HPP
#define REPEATEDPLAYBACKMENU_HPP

#include "ConsoleApp.hpp"
#include "AudioClient.hpp"
#include "../../common/Audio/AudioHelper.hpp"
#include <telux/audio/AudioPlayer.hpp>

class RepeatedPlaybackMenu : public ConsoleApp,
                 public std::enable_shared_from_this<RepeatedPlaybackMenu>,
                 public telux::audio::IPlayListListener {
 public:
    RepeatedPlaybackMenu(std::string appName, std::string cursor);
    ~RepeatedPlaybackMenu();

    bool init();
    void setSystemReady();
    void cleanup();

    void onPlaybackStarted() override;
    void onPlaybackStopped() override;
    void onError(telux::common::ErrorCode error, std::string file) override;
    void onFilePlayed(std::string file) override;
    void onPlaybackFinished() override;

 private:
    bool initAudioPlayerManager();

    void addToPlaylist(std::vector<std::string> userInput);
    void clearPlaylist(std::vector<std::string> userInput);
    void startPlayAudioFiles(std::vector<std::string> userInput);
    void stopPlayAudioFiles(std::vector<std::string> userInput);
    void setVolume(std::vector<std::string> userInput);
    void getVolume(std::vector<std::string> userInput);
    void setMute(std::vector<std::string> userInput);
    void getMute(std::vector<std::string> userInput);
    void setDevice(std::vector<std::string> userInput);
    void getDevice(std::vector<std::string> userInput);

    std::mutex playMutex_;
    std::mutex readyMutex_;
    std::condition_variable playCV_;
    telux::common::ErrorCode playError_;
    bool playFinished_ = false;
    bool errorOccurred_ = false;
    bool playStopped_ = false;
    bool playStarted_ = false;
    bool audioPlayerReady_ = false;
    std::shared_ptr<telux::audio::IAudioPlayer> audioPlayerMgr_;

    std::vector<telux::audio::PlaybackConfig> pbConfigs_;
};

#endif  // REPEATEDPLAYBACKMENU_HPP
