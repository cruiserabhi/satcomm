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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <thread>
#include <chrono>
#include <iostream>

#include "PlayMenu.hpp"

#define TOTAL_BUFFERS 2

PlayMenu::PlayMenu(std::string appName, std::string cursor,
                                            std::shared_ptr<AudioClient> audioClient)
   : ConsoleApp(appName, cursor),
   audioClient_(audioClient) {
    pipeLineEmpty_ = true;
    ready_ = false;
    playInProgress_ = false;
    writeFail_ = false;
}

PlayMenu::~PlayMenu() {
   cleanup();
}

void PlayMenu::init() {
    std::shared_ptr<ConsoleAppCommand> createStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Create Stream",
         {}, std::bind(&PlayMenu::createStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Delete Stream",
         {}, std::bind(&PlayMenu::deleteStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get Device",
         {}, std::bind(&PlayMenu::getDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set Device",
         {}, std::bind(&PlayMenu::setDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Get Volume",
         {}, std::bind(&PlayMenu::getVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Set Volume",
         {}, std::bind(&PlayMenu::setVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get Mute Status",
         {}, std::bind(&PlayMenu::getMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Set Mute",
         {}, std::bind(&PlayMenu::setMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startPlayCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Start Play",
         {}, std::bind(&PlayMenu::startPlay, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopPlayCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Stop Play",
         {}, std::bind(&PlayMenu::stopPlay, this, std::placeholders::_1)));


    std::vector<std::shared_ptr<ConsoleAppCommand>> playMenuCommandsList
      = {createStreamCommand,
         deleteStreamCommand,
         getDeviceCommand,
         setDeviceCommand,
         getVolumeCommand,
         setVolumeCommand,
         getMuteCommand,
         setMuteCommand,
         startPlayCommand,
         stopPlayCommand};
    if (audioClient_) {
            ready_ = true;
            audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(
            audioClient_->getStream(StreamType::PLAY));
            ConsoleApp::addCommands(playMenuCommandsList);
    } else {
        std::cout << "AudioClient not initialized " << std::endl;
    }
}

void PlayMenu::cleanup() {
    ready_ = false;
    playInProgress_ = false;
    playStatus_ = false;
    playStopcv_.notify_all();
    cv_.notify_all();
    for (std::thread &th : runningThreads_) {
        if(th.joinable()){
            th.join();
        }
    }
    pipeLineEmpty_ = true;
    audioPlayStream_ = nullptr;
    writeFail_ = false;
}

void PlayMenu::setSystemReady() {
    ready_ = true;
}


void PlayMenu::closeFile() {
    fclose(file_);
}

void PlayMenu::createStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if(ready_) {
        if(!audioPlayStream_) {
            status = audioClient_->createStream(StreamType::PLAY);
            if(status == telux::common::Status::SUCCESS) {
                audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(
                audioClient_->getStream(StreamType::PLAY));
                registerListener();
            }
        } else {
            std::cout << "Stream exist please delete first" << std::endl;
        }
    } else {
       std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void PlayMenu::deleteStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if(audioPlayStream_) {
        playStatus_ = false;
        writeFail_ = false;
        for(std::thread &th : runningThreads_) {
            if(th.joinable()){
                th.join();
            }
        }
        status = audioClient_->deleteStream(StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }

    if(status == telux::common::Status::SUCCESS) {
        deRegisterListener();
        audioPlayStream_ = nullptr;
    }
}

void PlayMenu::getDevice(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->getStreamDevice(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::setDevice(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->setStreamDevice(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::getVolume(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->getVolume(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::setVolume(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->setVolume(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::getMute(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->getMute(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::setMute(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->setMute(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::startPlay(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        // Check if a file is already being played.
        if(!playInProgress_) {
            audioClient_->getPlayConfig(filePath_, playFormat_);
            std::thread playThread(&PlayMenu::play, this);
            runningThreads_.emplace_back(std::move(playThread));
            playInProgress_ = true;
        } else {
            std::cout << "File Play in progress please wait" << std::endl;
        }
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::stopPlay(std::vector<std::string> userInput) {
    playStatus_ = false;
    writeFail_ = false;
    if (audioPlayStream_) {
        if (isAMR()){
            std::promise<bool> p;
            auto status = audioPlayStream_->stopAudio(
                StopType::FORCE_STOP, [&p](telux::common::ErrorCode error) {
                if (error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(true);
                } else {
                    p.set_value(false);
                    std::cout << "Failed to force stop" << std::endl;
                }
                });
            if(status == telux::common::Status::SUCCESS){
                std::cout << "Request to force stop Sent" << std::endl;
                if (p.get_future().get()) {
                    std::cout << "Force Stop successful" << std::endl;
                }
            } else {
                std::cout << "Request to force stop failed" << std::endl;
            }
        }
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::writeCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytes,
                telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS){
        if (isAMR()) {
            pipeLineEmpty_ = false;
        }
        writeFail_ = true;
    }

    if(buffer->getDataSize() != bytes){
        if (isAMR()) {
            pipeLineEmpty_ = false;
            std::cout << " pipeline not ready " << std::endl;
        }
        std::cout <<
            "Bytes Requested " << buffer->getDataSize() << " Bytes Written " << bytes << std::endl;
        // We are seeking back so that left over buffer can be resent again.
        long offset = -1 * (static_cast<long>((buffer->getDataSize() - bytes)));
        fseek(file_, offset, SEEK_CUR);

    }

    buffer->reset();
    freeBuffers_.push(buffer);
    cv_.notify_all();
    return;
}

void PlayMenu::play() {

    bool firstPlay = true;
    uint32_t size = 0;
    std::string userInput ="";
    uint32_t numBytes =0;
    std::promise<bool> p;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    while(!freeBuffers_.empty()) {
        freeBuffers_.pop();
    }

    file_ = fopen(filePath_.c_str(),"r");
    if(file_) {
        fseek(file_, 0, SEEK_SET);
    } else {
        std::cout <<"Unable to read file" << std::endl;
        return;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    for(int i = 0; i < TOTAL_BUFFERS; i++) {
        streamBuffer = audioPlayStream_->getStreamBuffer();
        if(streamBuffer != nullptr) {
            size = streamBuffer->getMinSize();
            if(size == 0) {
                size =  streamBuffer->getMaxSize();
            }
            streamBuffer->setDataSize(size);
            std::cout << "Buffer no. " << i << " buffer size "<< streamBuffer->getDataSize()
                    << std::endl;
            freeBuffers_.push(streamBuffer);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
            return;
        }
    }

    playStatus_ = true;
    pipeLineEmpty_ = true;
    writeFail_ = false;

    auto writeCb = std::bind(&PlayMenu::writeCallback, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);

    std::cout << "Audio play started" << std::endl;

    while (playStatus_)
    {
        if (!firstPlay) {
            if(feof(file_) && freeBuffers_.empty()) {
                cv_.wait(lock);
            }
            if(feof(file_) && pipeLineEmpty_ && (freeBuffers_.size() == TOTAL_BUFFERS)) {
                break;
            }
        }
        firstPlay = false;

        if(!freeBuffers_.empty() && (pipeLineEmpty_)) {
            streamBuffer = freeBuffers_.front();
            freeBuffers_.pop();
            numBytes = fread(streamBuffer->getRawBuffer(),1,size,file_);
            if (numBytes == 0 && feof(file_)) {
                streamBuffer->reset();
                freeBuffers_.push(streamBuffer);
                playStatus_ = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if(numBytes != size && !feof(file_)) {
                std::cout << "Unable to read specified bytes, bytes read: " << numBytes<< std::endl;
                streamBuffer->reset();
                freeBuffers_.push(streamBuffer);
                playStatus_ = false;
                writeFail_ = true;
                break;
            }
            streamBuffer->setDataSize(numBytes);
            if (ready_) {
                telux::common::Status status = audioPlayStream_->write(streamBuffer,writeCb);
                if(status != telux::common::Status::SUCCESS) {
                    std::cout << "write() failed with error" << static_cast<unsigned int>(status)
                    <<std::endl;
                    streamBuffer->reset();
                    freeBuffers_.push(streamBuffer);
                    playStatus_ = false;
                    writeFail_ = true;
                    break;
                }
            } else {
                std::cout << "Audio Service UNAVAILABLE" << std::endl;
            }
        } else {
            cv_.wait(lock);
            if(writeFail_) {
                break;
            }
        }
    }

    if (ready_) {
        if(isAMR()) {
            std::unique_lock<std::mutex> lck(playStopMutex_);

            auto status = audioPlayStream_->stopAudio(StopType::STOP_AFTER_PLAY,
                [&p](telux::common::ErrorCode error) {
                    if (error == telux::common::ErrorCode::SUCCESS) {
                        p.set_value(true);
                    } else {
                        p.set_value(false);
                        std::cout << "Failed to stop after playing buffers" << std::endl;
                    }
            });

            if(status == telux::common::Status::SUCCESS) {
                std::cout << "Request to stop playback after pending buffers Sent" << std::endl;
                if (p.get_future().get()) {
                    std::cout << "Pending buffers played successfully" << std::endl;
                    playStopcv_.wait(lck);
                }
            } else {
                std::cout << "Request to stop playback after pending buffers failed" << std::endl;
            }
        } else {
            while(freeBuffers_.size() != TOTAL_BUFFERS) {
                cv_.wait(lock);
            }
        }
    }


    if(writeFail_) {
        std::cout << "Play Failed" << std::endl;
    } else {
        if(playStatus_) {
            std::cout << "File played SuccessFully" <<std::endl;
        } else {
            std::cout << "Play Stopped" << std::endl;
        }
    }

    playStatus_ = false;
    playInProgress_ = false;
    closeFile();
}

bool PlayMenu::isAMR() {

    switch (playFormat_) {
        case AudioFormat::AMRWB_PLUS:
        case AudioFormat::AMRWB:
        case AudioFormat::AMRNB:
            return true;
        default:
            return false;
    }

    return false;
}

void PlayMenu::onReadyForWrite() {
    // This event is received in case of compressed audio format playback, it is received when the
    // buffer pipeline is ready to accept new buffers.
    std::cout << "Write Indication Received" << std::endl;
    pipeLineEmpty_ = true;
    cv_.notify_all();
}

void PlayMenu::onPlayStopped() {
    {
        std::lock_guard<std::mutex> lk(playStopMutex_);
        playStopcv_.notify_all();
        std::cout << "Playback Stopped after playing pending buffers" << std::endl;
    }
}

void PlayMenu::registerListener() {
    telux::common::Status status = audioPlayStream_ ->registerListener(shared_from_this());
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request to register Play Listener Sent" << std::endl;
    }
}

void PlayMenu::deRegisterListener() {
    telux::common::Status status = audioPlayStream_ ->deRegisterListener(shared_from_this());
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request to deregister Play Listener Sent" << std::endl;
    }
}
