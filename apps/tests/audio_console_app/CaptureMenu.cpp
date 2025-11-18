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
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <chrono>
#include <iostream>
#include <queue>
#include <cstddef>

#include "CaptureMenu.hpp"

#define TOTAL_BUFFERS 2
#define BITS_PER_SAMPLE 16

CaptureMenu::CaptureMenu(std::string appName, std::string cursor,
                                            std::shared_ptr<AudioClient> audioClient)
   : ConsoleApp(appName, cursor),
   audioClient_(audioClient) {
       captureStatus_ = false;
       ready_ = false;
       readFail_ = false;
}

CaptureMenu::~CaptureMenu() {
    audioClient_ = nullptr;
    captureStatus_ = false;
    readFail_ = false;

    for(std::thread &th : runningThreads_) {
        if(th.joinable()){
            th.join();
        }
    }
}

void CaptureMenu::init() {
    std::shared_ptr<ConsoleAppCommand> createStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Create Stream",
         {}, std::bind(&CaptureMenu::createStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Delete Stream",
         {}, std::bind(&CaptureMenu::deleteStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get Device",
         {}, std::bind(&CaptureMenu::getDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set Device",
         {}, std::bind(&CaptureMenu::setDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Get Volume",
         {}, std::bind(&CaptureMenu::getVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Set Volume",
         {}, std::bind(&CaptureMenu::setVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get Mute Status",
         {}, std::bind(&CaptureMenu::getMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Set Mute",
         {}, std::bind(&CaptureMenu::setMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startCaptureCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Start Capture",
         {}, std::bind(&CaptureMenu::startCapture, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopCaptureCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Stop Capture",
         {}, std::bind(&CaptureMenu::stopCapture, this, std::placeholders::_1)));


     std::vector<std::shared_ptr<ConsoleAppCommand>> captureMenuCommandsList
      = {createStreamCommand,
         deleteStreamCommand,
         getDeviceCommand,
         setDeviceCommand,
         getVolumeCommand,
         setVolumeCommand,
         getMuteCommand,
         setMuteCommand,
         startCaptureCommand,
         stopCaptureCommand};
   if(audioClient_){
        ready_ = true;
        audioCaptureStream_ = std::dynamic_pointer_cast<IAudioCaptureStream>(
           audioClient_->getStream(StreamType::CAPTURE));
        ConsoleApp::addCommands(captureMenuCommandsList);
   } else {
       std::cout << "AudioClient not initialized " << std::endl;
   }
}

void CaptureMenu::cleanup() {
    ready_ = false;
    captureStatus_ = false;
    readFail_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cv_.notify_all();
    }
    for (std::thread &th : runningThreads_) {
        if (th.joinable()){
            th.join();
        }
    }
    bufferRecordedTillNow_ = 0;
    audioCaptureStream_ = nullptr;
}

void CaptureMenu::setSystemReady() {
    ready_ = true;
}


void CaptureMenu::createStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if (ready_) {
        if(!audioCaptureStream_) {
            status = audioClient_->createStream(telux::audio::StreamType::CAPTURE);
            if(status == telux::common::Status::SUCCESS) {
                audioCaptureStream_ = std::dynamic_pointer_cast<IAudioCaptureStream>(
                    audioClient_->getStream(StreamType::CAPTURE));
            }
        } else {
             std::cout << "Stream exist please delete first" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void CaptureMenu::deleteStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if(audioCaptureStream_) {
        captureStatus_ = false;
        readFail_ = false;
        for(std::thread &th : runningThreads_) {
            if(th.joinable()){
                th.join();
            }
        }
        status = audioClient_->deleteStream(StreamType::CAPTURE);
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }

    if(status == telux::common::Status::SUCCESS) {
        audioCaptureStream_ = nullptr;
    }
}

void CaptureMenu::getDevice(std::vector<std::string> userInput) {
    if(audioCaptureStream_) {
        audioClient_->getStreamDevice(telux::audio::StreamType::CAPTURE);
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }
}

void CaptureMenu::setDevice(std::vector<std::string> userInput) {
    if(audioCaptureStream_) {
        audioClient_->setStreamDevice(telux::audio::StreamType::CAPTURE);
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }
}

void CaptureMenu::getVolume(std::vector<std::string> userInput) {
    if(audioCaptureStream_) {
        audioClient_->getVolume(telux::audio::StreamType::CAPTURE);
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }
}

void CaptureMenu::setVolume(std::vector<std::string> userInput) {
    if(audioCaptureStream_) {
        audioClient_->setVolume(telux::audio::StreamType::CAPTURE);
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }
}

void CaptureMenu::getMute(std::vector<std::string> userInput) {
    if(audioCaptureStream_) {
        audioClient_->getMute(telux::audio::StreamType::CAPTURE);
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }
}

void CaptureMenu::setMute(std::vector<std::string> userInput) {
    if(audioCaptureStream_) {
        audioClient_->setMute(telux::audio::StreamType::CAPTURE);
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }
}

void CaptureMenu::startCapture(std::vector<std::string> userInput) {
    if(audioCaptureStream_) {
        if(!captureStatus_) {
            std::string userInput ="";
            std::cout << "Enter File name with path : "  ;
            std::getline(std::cin, filePath_);

            std::thread recordThread(&CaptureMenu::record, this);
            runningThreads_.emplace_back(std::move(recordThread));
        } else {
            std::cout << "Capturing audio already in process" << std::endl;
        }
    } else {
        std::cout << "No running Capture session please create one" << std::endl;
    }
}

void CaptureMenu::stopCapture(std::vector<std::string> userInput) {
    captureStatus_ = false;
    readFail_ = false;
}


void CaptureMenu::record() {
    bufferRecordedTillNow_ = 0;
    uint32_t bytesToRead = 0;
    std::unique_lock<std::mutex> lock(mutex_);

    while(!freeBuffers_.empty()) {
        freeBuffers_.pop();
    }
    // Creating two buffers to fetch the data, to have ping pong behaviour
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    for(int i = 0; i < TOTAL_BUFFERS; i++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if(streamBuffer != nullptr) {
            // Setting the bytesToRead (bytes to be readed from stream) as minimum size
            // required by stream. In any case if size returned is 0, using the Maximum Buffer
            // Size, any buffer size between min and max can be used
            bytesToRead = streamBuffer->getMinSize();
            if(bytesToRead == 0) {
                bytesToRead = streamBuffer->getMaxSize();
            }
            freeBuffers_.push(streamBuffer);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
            return;
        }
    }
    // numChannels here stores num of channels;
    uint32_t sampleRate;
    uint32_t channelType;
    audioClient_->getCaptureConfig(sampleRate, channelType);
    int numChannels = (channelType == 3) ?  2 : 1;
    file_ = fopen(filePath_.c_str(),"w");
    if(file_) {
            fseek(file_, 0, SEEK_SET);
    } else {
        std::cout << "Unable to Create File " <<std::endl;
        return;
    }

    captureStatus_ = true;
    std::cout << "Audio Capture Started" << std::endl;
    auto readCb =  std::bind(&CaptureMenu::readCallback, this,
                  std::placeholders::_1, std::placeholders::_2);
    while (captureStatus_)
    {
        if(!freeBuffers_.empty()) {
            streamBuffer = freeBuffers_.front();
            freeBuffers_.pop();
            telux::common::Status status = audioCaptureStream_->read(streamBuffer,
                    bytesToRead, readCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "read() failed with error" << static_cast<unsigned int>(status)
                <<std::endl;
                streamBuffer->reset();
                freeBuffers_.push(streamBuffer);
                readFail_ = true;
                break;
            }
        } else {
            cv_.wait(lock);
            if(readFail_) {
                break;
            }
        }
    }

    if(readFail_) {
        while(freeBuffers_.size() != TOTAL_BUFFERS && ready_) {
            cv_.wait(lock);
        }
        std::cout << "File Recording Failed" <<std::endl;
    } else {
    /* WaitTime is time required to receive buffer for the last read request.
       We calculate the total time by converting max buffer size in bytes to bits, then we divide
       this by frame size which is numChannel(mono/setero)*16(2 byte per analog sample) and
       samplerate to get time.*/
        int waitTime = (8*(streamBuffer->getMaxSize())*1000)/
                        (sampleRate*numChannels*BITS_PER_SAMPLE);
        waitTime = waitTime+100;
        while(freeBuffers_.size() != TOTAL_BUFFERS && ready_) {
            cv_.wait_for(lock, std::chrono::milliseconds(waitTime));
        }
        std::cout << "File Recorded SuccessFully" <<std::endl;
    }

    fflush(file_);
    fclose(file_);
    captureStatus_ = false;
    readFail_ = false;
}

void CaptureMenu::readCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
           telux::common::ErrorCode error)
{
    uint32_t bytesWrittenToFile = 0;
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        readFail_ = true;
    } else {
        uint32_t size = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(),1,size,file_);
        if(bytesWrittenToFile != size) {
            std::cout << "Write Size mismatch while writing to file" << std::endl;
        }
        bufferRecordedTillNow_ = bufferRecordedTillNow_ + size;
    }
    buffer->reset();
    freeBuffers_.push(buffer);
    {
        /*This thread will be able to acquire this lock once the waiting thread is in wait state by
          releasing the lock, ready to receive the wake up notification.*/
        std::lock_guard<std::mutex> lock(mutex_);
        cv_.notify_all();
    }
}
