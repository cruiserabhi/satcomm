/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       AudioConsoleApp.cpp
 *
 * @brief      This is entry class for console application for audio,
 *             It allows one to interactively invoke most of the public APIs in audio.
 */

#include <iostream>
#include <memory>

extern "C" {
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
}

#include <telux/audio/AudioFactory.hpp>

#include "VoiceMenu.hpp"
#include "PlayMenu.hpp"
#include "CaptureMenu.hpp"
#include "LoopbackMenu.hpp"
#include "ToneMenu.hpp"
#include "TransCodeMenu.hpp"
#include "HpcmMenu.hpp"
#include "RepeatedPlaybackMenu.hpp"

#include "AudioConsoleApp.hpp"
#include "../../common/utils/Utils.hpp"
#include <telux/common/Version.hpp>

AudioConsoleApp::AudioConsoleApp(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
    ready_ = false;
}

AudioConsoleApp::~AudioConsoleApp() {
    voiceMenu_ = nullptr;
    playMenu_ = nullptr;
    captureMenu_ = nullptr;
    loopbackMenu_ = nullptr;
    toneMenu_ = nullptr;
    transCodeMenu_ = nullptr;
    audioClient_ = nullptr;
}

void AudioConsoleApp::init() {
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    //  Get the AudioFactory and AudioManager instances.
    auto &audioFactory = telux::audio::AudioFactory::getInstance();
    audioManager_ = audioFactory.getAudioManager([&prom](telux::common::ServiceStatus status) {
        if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            prom.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        } else {
            prom.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
        }
    });
    if (!audioManager_) {
        std::cout << "Failed to get AudioManager object" << std::endl;
        return;
    }

    //  Check if audio subsystem is ready
    //  If audio subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = audioManager_->getServiceStatus();
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }

    //  Exit the application, if SDK is unable to initialize audio subsystems
    if (managerStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Audio Subsystems to ready : " << elapsedTime.count() << "s"
                << std::endl;
        ready_ = true;
    } else {
        std::cout << " *** ERROR - Unable to initialize audio subsystem" << std::endl;
        return;
    }
    audioClient_ = std::make_shared<AudioClient>(audioManager_);

    auto status = audioManager_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Audio Listener Registeration failed" <<std::endl;
    }
    initConsole();
}

void AudioConsoleApp::initConsole() {
    std::shared_ptr<ConsoleAppCommand> voiceMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Voice Call", {},
        std::bind(&AudioConsoleApp::voiceMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> playMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Playback", {},
        std::bind(&AudioConsoleApp::playMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> captureMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Capture", {},
        std::bind(&AudioConsoleApp::captureMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> loopbackMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Loopback", {},
        std::bind(&AudioConsoleApp::loopbackMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> toneMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Tone", {},
        std::bind(&AudioConsoleApp::toneMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> transCodeMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "TransCode", {},
        std::bind(&AudioConsoleApp::transCodeMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> getCalStatusCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get Calibration Status", {},
        std::bind(&AudioConsoleApp::getCalStatus, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> getSupportedStreamsCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Get Supported Streams", {},
        std::bind(&AudioConsoleApp::getSupportedStreams, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> getSupportedDevicesCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Get Supported Devices", {},
        std::bind(&AudioConsoleApp::getSupportedDevices, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> hpcmMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Hpcm", {},
        std::bind(&AudioConsoleApp::hpcmMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> repeatedPlaybackMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "Repeated Playback", {},
        std::bind(&AudioConsoleApp::repeatedPlaybackMenu, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
    = {voiceMenuCommand, playMenuCommand, captureMenuCommand, loopbackMenuCommand,
        toneMenuCommand, transCodeMenuCommand, getCalStatusCommand,
        getSupportedStreamsCommand, getSupportedDevicesCommand , hpcmMenuCommand,
        repeatedPlaybackMenuCommand};

    voiceMenu_ = std::make_shared<VoiceMenu>("Voice Menu", "voice> ");
    voiceMenu_->init();
    playMenu_ = std::make_shared<PlayMenu>("Play Menu", "play> ", audioClient_);
    playMenu_->init();
    captureMenu_ = std::make_shared<CaptureMenu>("Capture Menu", "capture> ", audioClient_);
    captureMenu_->init();
    loopbackMenu_ = std::make_shared<LoopbackMenu>("Loopback Menu", "loopback> ", audioClient_);
    loopbackMenu_->init();
    toneMenu_ = std::make_shared<ToneMenu>("Tone menu", "tone> ", audioClient_);
    toneMenu_->init();
    transCodeMenu_ = std::make_shared<TransCodeMenu>("TransCode menu", "transCode> ");
    transCodeMenu_->init();
    hpcmMenu_ = std::make_shared<HpcmMenu>("Hpcm menu", "hpcm> ", audioManager_);
    hpcmMenu_->init();
    repeatedPlaybackMenu_ = std::make_shared<RepeatedPlaybackMenu>("RepeatedPlayback menu",
            "repeatedPlayback> ");
    repeatedPlaybackMenu_->init();

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void AudioConsoleApp::voiceMenu(std::vector<std::string> userInput) {
    voiceMenu_->displayMenu();
    voiceMenu_->mainLoop();
}

void AudioConsoleApp::playMenu(std::vector<std::string> userInput) {
    playMenu_->displayMenu();
    playMenu_->mainLoop();
}

void AudioConsoleApp::captureMenu(std::vector<std::string> userInput) {
    captureMenu_->displayMenu();
    captureMenu_->mainLoop();
}

void AudioConsoleApp::loopbackMenu(std::vector<std::string> userInput) {
    loopbackMenu_->displayMenu();
    loopbackMenu_->mainLoop();
}

void AudioConsoleApp::toneMenu(std::vector<std::string> userInput) {
    toneMenu_->displayMenu();
    toneMenu_->mainLoop();
}

void AudioConsoleApp::transCodeMenu(std::vector<std::string> userInput) {
    transCodeMenu_->displayMenu();
    transCodeMenu_->mainLoop();
}

void AudioConsoleApp::hpcmMenu(std::vector<std::string> userInput) {
    hpcmMenu_->displayMenu();
    hpcmMenu_->mainLoop();
}

void AudioConsoleApp::repeatedPlaybackMenu(std::vector<std::string> userInput) {
    repeatedPlaybackMenu_->displayMenu();
    repeatedPlaybackMenu_->mainLoop();
}

void AudioConsoleApp::getCalStatus(std::vector<std::string> userInput) {
    if (!ready_) {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
        return;
    }

    if (audioManager_) {
        std::promise<bool> p;
        auto status = audioManager_->getCalibrationInitStatus(
            [&p](CalibrationInitStatus calStatus, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                if (calStatus == CalibrationInitStatus::INIT_SUCCESS) {
                    std::cout << "Calibration initialized successfully" << std::endl;
                } else if (calStatus == CalibrationInitStatus::INIT_FAILED) {
                    std::cout << "Calibration init failed" << std::endl;
                } else {
                    std::cout << "Calibration Status Unknown" << std::endl;
                }
                p.set_value(true);
            } else if(error == telux::common::ErrorCode::NOT_SUPPORTED) {
                p.set_value(false);
                std::cout << "API not supported" << std::endl;
            } else {
                p.set_value(false);
                std::cout << "failed to get cal init status" << std::endl;
            }
            });
        if (status == telux::common::Status::SUCCESS){
            std::cout << "Request to get cal init status sent" << std::endl;
        } else {
            std::cout << "Request to get cal init status failed" << std::endl;
        }
        p.get_future().get();
    } else {
        std::cout << "Invalid Audio Manager" << std::endl;
    }
}

void AudioConsoleApp::getSupportedDevices(std::vector<std::string> userInput) {
    if (!ready_) {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
        return;
    }

    if (audioManager_) {
        std::promise<bool> p;
        auto status = audioManager_->getDevices([&p, this](
            std::vector<std::shared_ptr<IAudioDevice>> devices, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                for (auto &it : devices) {
                    if (it != nullptr) {
                        std::cout << "DeviceType: " << static_cast<int>(it->getType()) << std::endl;
                        if (it->getDirection() == DeviceDirection::TX) {
                            std::cout << "Direction : TX " << std::endl;
                        } else if (it->getDirection() == DeviceDirection::RX) {
                            std::cout << "Direction : RX " << std::endl;
                        } else {
                            std::cout << "Direction : NONE" << std::endl;
                        }
                    }
                }
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "failed to get supported devices" << std::endl;
            }
        });
        if (status == telux::common::Status::SUCCESS){
            std::cout << "Request to get supported devices sent" << std::endl;
        } else {
            std::cout << "Request to get supported devices failed" << std::endl;
        }
        p.get_future().get();
    } else {
        std::cout << "Invalid Audio Manager" << std::endl;
    }
}

void AudioConsoleApp::getSupportedStreams(std::vector<std::string> userInput) {
    if (!ready_) {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
        return;
    }

    if (audioManager_) {
        std::promise<bool> p;
        auto status = audioManager_->getStreamTypes(
            [&p, this](std::vector<StreamType> streamTypes, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                for (auto it : streamTypes) {
                    auto streamName = getStreamName(it);
                    std::cout << "Stream Type : " << streamName << std::endl;
                }
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "failed to get supported stream types" << std::endl;
            }
        });
        if (status == telux::common::Status::SUCCESS){
            std::cout << "Request to get supported stream sent" << std::endl;
        } else {
            std::cout << "Request to get supported stream failed" << std::endl;
        }
        p.get_future().get();
    } else {
        std::cout << "Invalid Audio Manager" << std::endl;
    }
}

void AudioConsoleApp::cleanup() {
    ready_ = false;
    audioClient_->cleanup();
    voiceMenu_->cleanup();
    playMenu_->cleanup();
    captureMenu_->cleanup();
    loopbackMenu_->cleanup();
    toneMenu_->cleanup();
    transCodeMenu_->cleanup();
    hpcmMenu_->cleanup();
    repeatedPlaybackMenu_->cleanup();
}

void AudioConsoleApp::setSystemReady() {
    ready_ = true;
    if (voiceMenu_) {
        voiceMenu_->setSystemReady();
    }
    if (playMenu_) {
        playMenu_->setSystemReady();
    }
    if (captureMenu_) {
        captureMenu_->setSystemReady();
    }
    if (loopbackMenu_) {
        loopbackMenu_->setSystemReady();
    }
    if (toneMenu_) {
        toneMenu_->setSystemReady();
    }
    if (transCodeMenu_) {
        transCodeMenu_->setSystemReady();
    }
    if (hpcmMenu_) {
        hpcmMenu_->setSystemReady();
    }
    if (repeatedPlaybackMenu_) {
        repeatedPlaybackMenu_->setSystemReady();
    }
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "Audio console app - SDK v" + std::to_string(sdkVersion.major) + "."
                            + std::to_string(sdkVersion.minor) + "."
                            + std::to_string(sdkVersion.patch) + "\n" +
                            "Release name: " + sdkReleaseName;
    auto audioConsoleApp = std::make_shared<AudioConsoleApp>(appName, "audio> ");

    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    audioConsoleApp->init();  // initialize commands and display

    return audioConsoleApp->mainLoop();  // Main loop to continuously read and execute commands

}

void AudioConsoleApp::onServiceStatusChange(telux::common::ServiceStatus status) {
    if (status == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
        cleanup();
    }
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Service AVAILABLE" << std::endl;
        setSystemReady();
    }
}

std::string AudioConsoleApp::getStreamName(StreamType type) {
    switch (type){
        case StreamType::VOICE_CALL:
        return "VOICE_CALL";
        case StreamType::PLAY:
        return "PLAY";
        case StreamType::CAPTURE:
        return "CAPTURE";
        case StreamType::LOOPBACK:
        return "LOOPBACK";
        case StreamType::TONE_GENERATOR:
        return "TONE_GENERATOR";
        default:
        return "NONE";
    }
}
