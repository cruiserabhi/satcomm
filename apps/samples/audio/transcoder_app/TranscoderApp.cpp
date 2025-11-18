/*
 *  Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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


#include <future>
#include <iostream>
#include <condition_variable>

#include <telux/audio/AudioFactory.hpp>
#include "TranscoderApp.hpp"

using std::promise;
using namespace telux::common;
using namespace telux::audio;

const uint32_t SAMPLE_RATE = 48000;
const uint32_t CHANNEL_MASK = 3;
const uint32_t GAURD_FOR_WAITING = 100;


// Below constant represents number of buffers allocated to pass Bitstream. Additional number of
// buffers would provide flexibility in copying Bitsream from source and write to Stream Interface
// in two parallel threaded operations.
const int TOTAL_READ_BUFFERS = 1;
const int TOTAL_WRITE_BUFFERS = 1;
const int EOF_REACHED = 1;
const int EOF_NOT_REACHED = 0;

TranscoderApp::TranscoderApp() {
    pipeLineEmpty_ = true;
}

TranscoderApp::~TranscoderApp() {
    for(std::thread &th : runningThreads_) {
        if(th.joinable()){
            th.join();
        }
    }
}

Status TranscoderApp::init() {
   // Get the AudioFactory and AudioManager instances.
    std::promise<ServiceStatus> prom{};
    auto &audioFactory = AudioFactory::getInstance();
    audioManager_ = audioFactory.getAudioManager([&prom](ServiceStatus status) {
        if (status == ServiceStatus::SERVICE_AVAILABLE) {
            prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
        } else {
            prom.set_value(ServiceStatus::SERVICE_FAILED);
        }
    });
    if (!audioManager_) {
        std::cout << "Failed to get AudioManager object" << std::endl;
        return Status::FAILED;
    }
    //  Check if audio subsystem is ready
    //  If audio subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = audioManager_->getServiceStatus();
    if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }

    if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsystem is ready." << std::endl;
    } else {
        std::cout << "Audio Subsystem is NOT ready." << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

void TranscoderApp::transcode() {
    createTranscoder();
    if (transcoder_) {
        std::thread writeThread(&TranscoderApp::write, this);
        runningThreads_.emplace_back(std::move(writeThread));
        std::thread readThread(&TranscoderApp::read, this);
        runningThreads_.emplace_back(std::move(readThread));
    } else {
        std::cout << "Transcoder not avaialble !!" << std::endl;
    }
}

void TranscoderApp::createTranscoder() {
    std::promise<bool> p;

    AmrwbpParams inputParams{};
    inputConfig_.sampleRate = SAMPLE_RATE;
    inputConfig_.mask = CHANNEL_MASK;
    inputConfig_.format = AudioFormat::AMRWB_PLUS;
    inputParams.bitWidth = 16;
    inputParams.frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
    inputConfig_.params = &inputParams;

    inputConfig_.sampleRate = SAMPLE_RATE;
    outputConfig_.mask = CHANNEL_MASK;
    outputConfig_.format = AudioFormat::PCM_16BIT_SIGNED;
    outputConfig_.params = nullptr;

    audioManager_->createTranscoder(inputConfig_, outputConfig_,
    [&p,this](std::shared_ptr<telux::audio::ITranscoder> &transcoder,
        ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            transcoder_ = transcoder;
            registerListener();
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "failed to create transcoder" <<std::endl;
        }
    });
    if (p.get_future().get()) {
        std::cout<< "Transcoder Created" << std::endl;
    }
}

void TranscoderApp::read() {
    uint32_t bytesToRead = 0;
    std::unique_lock<std::mutex> lock(mutex_);

    while (!readBuffers_.empty()) {
        readBuffers_.pop();
    }
    std::shared_ptr<telux::audio::IAudioBuffer> audioBuffer;
    for (int i = 0; i < TOTAL_READ_BUFFERS; i++) {
        audioBuffer = transcoder_->getReadBuffer();
        if (audioBuffer != nullptr) {
            // Setting the bytesToRead (bytes to be readed from stream) as minimum size
            // required by stream. In any case if size returned is 0, using the Maximum Buffer
            // Size, any buffer size between min and max can be used
            bytesToRead = audioBuffer->getMinSize();
            if (bytesToRead == 0) {
                bytesToRead = audioBuffer->getMaxSize();
            }
            std::cout << "Bytes to read" << bytesToRead << std::endl;
            readBuffers_.push(audioBuffer);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
            return;
        }
    }
    if (!readBuffers_.empty()) {
        audioBuffer = readBuffers_.front();
        readBuffers_.pop();
        auto readCb =  std::bind(&TranscoderApp::readCallback, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        Status status = transcoder_->read(audioBuffer, bytesToRead, readCb);
        if (status != Status::SUCCESS) {
            std::cout << "read() failed with error" << static_cast<unsigned int>(status)
            <<std::endl;
        }
    } else {
        cv_.wait(lock);
    }
    // waitTime is time required to receive last remaining buffers when all
    std::cout << "Transcoding Successful" <<std::endl;
}

void TranscoderApp::readCallback(std::shared_ptr<telux::audio::IAudioBuffer> buffer,
         uint32_t isLastBuffer, ErrorCode error) {

    if (isLastBuffer) {
        // Stop reading from now onwards as this is the last transcoded buffers
    }
    if (error != ErrorCode::SUCCESS) {
        std::cout << "read() returned with error " << static_cast<unsigned int>(error) << std::endl;
    } else {
        // uint32_t size = buffer->getDataSize();
    }
    buffer->reset();
    readBuffers_.push(buffer);
    cv_.notify_all();
    return;
}

void TranscoderApp::teardown() {
    std::promise<bool> p;
    auto status = transcoder_->tearDown([&p](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to tear down" << std::endl;
        }
        });
    if (status == Status::SUCCESS) {
        std::cout << "Request to Teardown transcoder sent" << std::endl;
    } else {
        std::cout << "Request to Teardown transcoder failed" << std::endl;
    }
    if (p.get_future().get()) {
        transcoder_ = nullptr;
        std::cout << "Tear Down successful !!" << std::endl;
    }
}

// Callback to provide response to the write request
void TranscoderApp::writeCallback(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t bytes, ErrorCode error) {
    std::cout << "Bytes Written : " << bytes << std::endl;
    if (error != ErrorCode::SUCCESS || buffer->getDataSize() != bytes) {
        // Application needs to resend the Bitstream buffer from leftover position if bytes
        // consumed are not equal to requested number of bytes to be written.
        pipeLineEmpty_ = false;
    }
    buffer->reset();
    writeBuffers_.push(buffer);
    cv_.notify_all();
    return;
}

// This event is received in case of compressed audio format playback, it is received when the
// buffer pipeline is ready to accept new buffers.
void TranscoderApp::onReadyForWrite() {
    pipeLineEmpty_ = true;
    cv_.notify_all();
}

void TranscoderApp::write() {
    while (!writeBuffers_.empty()) {
        writeBuffers_.pop();
    }

    std::unique_lock<std::mutex> lock(mutex_);
    uint32_t size = 0;
    uint32_t numBytes = 0;
    std::shared_ptr<telux::audio::IAudioBuffer> audioBuffer;
    for(int i = 0; i < TOTAL_WRITE_BUFFERS; i++) {
        audioBuffer = transcoder_->getWriteBuffer();
        if (audioBuffer != nullptr) {
            size = audioBuffer->getMinSize();
            if(size == 0) {
                size =  audioBuffer->getMaxSize();
            }
            writeBuffers_.push(audioBuffer);
        } else {
            std::cout << "Failed to get Buffers for Write operation " << std::endl;
            return;
        }
    }
    pipeLineEmpty_ = true;
    if (!writeBuffers_.empty() && (pipeLineEmpty_)) {
        audioBuffer = writeBuffers_.front();
        writeBuffers_.pop();
        // Here Bitstream content passed as zero for representation only. Actually valid Bitstream
        // of "size" bytes need to be passed, except during end of operation where "size"
        // represents last leftover Bitstream.
        memset(audioBuffer->getRawBuffer(), 0, size);
        audioBuffer->setDataSize(numBytes);
        auto writeCb = std::bind(&TranscoderApp::writeCallback, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3);
        Status status = Status::FAILED;
        if (EOF_REACHED) {
            status = transcoder_->write(audioBuffer, EOF_REACHED,  writeCb);
        } else {
            status = transcoder_->write(audioBuffer, EOF_NOT_REACHED,  writeCb);
        }
        if (status != Status::SUCCESS) {
            std::cout << "write() failed with error" << static_cast<unsigned int>(status)
            <<std::endl;
        } else {
            std::cout << "Request to transcode buffers sent " << std::endl;
        }
    } else {
        cv_.wait(lock);
    }
}

void TranscoderApp::registerListener() {
    Status status = transcoder_ ->registerListener(shared_from_this());
    if (status == Status::SUCCESS) {
        std::cout << "Request to register Transcode Listener Sent" << std::endl;
    }
}

void TranscoderApp::deRegisterListener() {
    Status status = transcoder_ ->deRegisterListener(shared_from_this());
    if (status == Status::SUCCESS) {
        std::cout << "Request to deregister Transcode Listener Sent" << std::endl;
    }
}

int main(int argc, char ** argv) {
    // Creating an instance of application
    std::shared_ptr<TranscoderApp> app = std::make_shared<TranscoderApp>();
    // Initialing the object
    app->init();

    // Starting the transcoding operation.
    app->transcode();

    // Tearing down the transcoder as it can not be used for multiple transcoding operation
    app->teardown();

    return EXIT_SUCCESS;
}
