/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <chrono>
#include <iostream>

#include "LoopbackMenu.hpp"

LoopbackMenu::LoopbackMenu(
    std::string appName, std::string cursor, std::shared_ptr<AudioClient> audioClient)
   : ConsoleApp(appName, cursor)
   , audioClient_(audioClient) {
    ready_ = false;
    loopbackStarted_ = false;
}

LoopbackMenu::~LoopbackMenu() {
    audioClient_ = nullptr;
}

void LoopbackMenu::init() {
    std::shared_ptr<ConsoleAppCommand> createStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Create Stream", {},
            std::bind(&LoopbackMenu::createStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Delete Stream", {},
            std::bind(&LoopbackMenu::deleteStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startLoopbackCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Start Loopback", {},
            std::bind(&LoopbackMenu::startLoopback, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopLoopbackCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Stop Loopback", {},
            std::bind(&LoopbackMenu::stopLoopback, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> loopbackMenuCommandsList
        = {createStreamCommand, deleteStreamCommand, startLoopbackCommand, stopLoopbackCommand};

    if (audioClient_) {
        ready_ = true;
        audioLoopbackStream_ = std::dynamic_pointer_cast<IAudioLoopbackStream>(
            audioClient_->getStream(StreamType::LOOPBACK));
        ConsoleApp::addCommands(loopbackMenuCommandsList);
    } else {
        std::cout << "AudioClient not initialized " << std::endl;
    }
}

void LoopbackMenu::cleanup() {
    ready_ = false;
    loopbackStarted_ = false;
    audioLoopbackStream_ = nullptr;
}

void LoopbackMenu::setSystemReady() {
    ready_ = true;
}

void LoopbackMenu::createStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if (ready_) {
        if (!audioLoopbackStream_) {
            status = audioClient_->createStream(telux::audio::StreamType::LOOPBACK);
            if (status == telux::common::Status::SUCCESS) {
                audioLoopbackStream_ = std::dynamic_pointer_cast<IAudioLoopbackStream>(
                    audioClient_->getStream(StreamType::LOOPBACK));
            }
        } else {
            std::cout << "Stream exist please delete first" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void LoopbackMenu::deleteStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if (audioLoopbackStream_) {
        status = audioClient_->deleteStream(StreamType::LOOPBACK);
    } else {
        std::cout << "No running loopback session please create one" << std::endl;
    }

    if (status == telux::common::Status::SUCCESS) {
        loopbackStarted_ = false;
        audioLoopbackStream_ = nullptr;
    }
}

void LoopbackMenu::startLoopback(std::vector<std::string> userInput) {
    if (audioLoopbackStream_) {
        if (!loopbackStarted_) {
            std::promise<bool> p;
            Status status
                = audioLoopbackStream_->startLoopback([&p, this](telux::common::ErrorCode error) {
                      if (error == telux::common::ErrorCode::SUCCESS) {
                          p.set_value(true);
                      } else {
                          p.set_value(false);
                          std::cout << "Failed to start loopback" << std::endl;
                      }
                  });
            if (status == Status::SUCCESS) {
                std::cout << "Request to start loopback sent" << std::endl;
            } else {
                std::cout << "Request to start loopback Failed" << std::endl;
            }

            if (p.get_future().get()) {
                loopbackStarted_ = true;
                std::cout << "Audio loopback is Started" << std::endl;
            }
        } else {
            std::cout << "Loopback already started" << std::endl;
        }
    } else {
        std::cout << "No running loopback session please create one" << std::endl;
    }
}

void LoopbackMenu::stopLoopback(std::vector<std::string> userInput) {
    if (audioLoopbackStream_) {
        if (loopbackStarted_) {
            std::promise<bool> p;
            Status status
                = audioLoopbackStream_->stopLoopback([&p, this](telux::common::ErrorCode error) {
                      if (error == telux::common::ErrorCode::SUCCESS) {
                          p.set_value(true);
                      } else {
                          p.set_value(false);
                          std::cout << "Failed to stop loopback" << std::endl;
                      }
                  });
            if (status == Status::SUCCESS) {
                std::cout << "Request to stop loopback sent" << std::endl;
            } else {
                std::cout << "Request to stop loopback Failed" << std::endl;
            }

            if (p.get_future().get()) {
                std::cout << "Audio loopback is Stopped" << std::endl;
                loopbackStarted_ = false;
            }
        } else {
            std::cout << "loopback not started yet" << std::endl;
        }
    } else {
        std::cout << "No running loopback session please create one" << std::endl;
    }
}
