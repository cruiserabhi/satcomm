/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>

#include "HpcmMenu.hpp"


HpcmMenu::HpcmMenu(std::string appName, std::string cursor,
                                            std::shared_ptr<IAudioManager> audioManager)
    : ConsoleApp(appName, cursor), slotId_(DEFAULT_SLOT_ID), hpcmReady_(false),  exitHpcm_(true),
      exitPlayThread_(false), exitRecordThread_(false), audioManager_(audioManager) {
}

HpcmMenu::~HpcmMenu() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        /* exitHpcm_: To identify if HPCM has exited due to the application exit. */
        exitHpcm_ = true;
    }
    cleanup();
}

void HpcmMenu::init() {
    std::shared_ptr<ConsoleAppCommand> startHpcmCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Start HPCM", {},
            std::bind(&HpcmMenu::startHpcmAudio, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopHpcmCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Stop HPCM", {},
            std::bind(&HpcmMenu::stopHpcmAudio, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> hpcmMenuCommandsList = {startHpcmCommand,
        stopHpcmCommand};

    hpcmReady_ = true;
    ConsoleApp::addCommands(hpcmMenuCommandsList);
}

void HpcmMenu::setSystemReady() {
    hpcmReady_ = true;
}

/* Cleanup can be triggered either during SSR or when the application exits */
void HpcmMenu::cleanup() {
    std::lock_guard<std::mutex> lk(mutex_);
    /* hpcmReady_: To determine if the service is ready, for example, during SSR */
    hpcmReady_ = false;
    captureCv_.notify_all();
    bufferReadyCv_.notify_all();

    for (std::thread &th : runningThreads_) {
        if (th.joinable()){
            th.join();
        }
    }

    /* Clear freeCaptureBuffers_ is necessary when startHpcm is invoked after ssr.
     * If this is not done, the second startHpcm will have two buffers in freeCaptureBuffers_
     */
    while (!freeCaptureBuffers_.empty()) {
        freeCaptureBuffers_.pop();
    }

    /* Clear freePlayBuffers_ is necessary when startHpcm is invoked after ssr.
     * If this is not done, the second startHpcm will have two buffers in freePlayBuffers_
     */
    while (!freePlayBuffers_.empty()) {
        freePlayBuffers_.pop();
    }

    readErrorOccurred_ = false;
    writeErrorOccurred_ = false;
    audioCaptureStream_ = nullptr;
    audioPlayStream_ = nullptr;
    voiceSessions_.clear();
    activeSession_ = nullptr;
    /* The record thread can exit due to the following scenarios:
     * (1) SSR occurred
     * (2) An error occurred
     * (3) Recording is finished
     * (4) Stop HPCM request
     * (5) Application exit
     *
     * If HPCM is stopped due to SSR, only `hpcmReady_` should be set. This will indicate that
     * the record thread is joining because of SSR and should not wait for any pending buffer
     * response.
     * To allow the user to start HPCM again after SSR, `exitHpcm_` should be set to true.
     * Note: Do not change the order of the variables to maintain the correct state.
     */
    exitHpcm_ = true;
}

Status HpcmMenu::createVoiceStream(StreamConfig &config) {
    telux::common::Status status = telux::common::Status::FAILED;
    std::cout <<"------------------------------------------------" << std::endl;
    std::cout << "Enter configuration for HPCM Stream" << std::endl;
    std::cout << "Supported sampling rates are 8kHz/16kHz." << std::endl;
    std::cout <<"------------------------------------------------" << std::endl;
    std::vector<telux::audio::Direction> direction{};
    config.slotId = slotId_;
    config.type = StreamType::VOICE_CALL;
    config.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    config.enableHpcm = true;
    getUserSampleRateInput(config.sampleRate);
    takeUserVoicePathInput(direction);
    std::lock_guard<std::mutex> lock(mutex_);
    //Create Voice stream for HPCM usecases
    if (!activeSession_) {
        std::cout << "No running voice session for slotId : " << slotId_
            << ", please create one" << std::endl;
        return Status::FAILED;
    }
    status = activeSession_->createStream(config);

    if (status != Status::SUCCESS) {
        deleteActiveSession(slotId_);
        std::cout << "Voice stream creation failed on slotId : " << slotId_
            << std::endl;
        return status;
    }

    config.voicePaths = direction;
    std::cout << "Voice stream created on slotId : " << slotId_ << std::endl;
    return status;
}

Status HpcmMenu::deleteVoiceStream() {
    telux::common::Status status = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!activeSession_) {
        std::cout << "No running voice session for slotId : " << slotId_
            << ", please create one" << std::endl;
        return Status::FAILED;
    }

    status = activeSession_->deleteStream();
    if (status != Status::SUCCESS) {
        std::cout << "Voice stream deletion failed on slotId : " << slotId_ << std::endl;
        return status;
    }
    deleteActiveSession(slotId_);
    readErrorOccurred_ = false;
    writeErrorOccurred_ = false;
    std::cout << "Voice stream deleted on slotId : " << slotId_ << std::endl;
    return status;
}

Status HpcmMenu::startVoiceStream() {
    telux::common::Status status = telux::common::Status::FAILED;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!activeSession_) {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
            return Status::FAILED;
        }
        status = activeSession_->startAudio();
        if (status != Status::SUCCESS) {
            std::cout << "Failed to start audio on slotId : " << slotId_ << std::endl;
            return status;
        }
        std::cout << "Audio started on slotId : " << slotId_ << std::endl;
        exitHpcm_ = false;
    }

    return status;
}

Status HpcmMenu::startHpcm() {
    if (!audioCaptureStream_) {
        std::cout << "Invalid audio capture stream for slotId : " << slotId_
            << std::endl;
        return Status::FAILED;
    }

    if (!audioPlayStream_) {
        std::cout << "Invalid audio play stream for slotId : " << slotId_
            << std::endl;
        return Status::FAILED;
    }

    std::thread recordThread(&HpcmMenu::record, this);
    runningThreads_.emplace_back(std::move(recordThread));

    std::thread playThread(&HpcmMenu::play, this);
    runningThreads_.emplace_back(std::move(playThread));

    return Status::SUCCESS;
}

Status HpcmMenu::stopVoiceStream() {
    telux::common::Status status = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!activeSession_) {
        std::cout << "No running voice session for slotId : " << slotId_
            << ", please create one" << std::endl;
        return Status::FAILED;
    }
    status = activeSession_->stopAudio();
    if (status != Status::SUCCESS) {
        std::cout << "Failed to stop audio on slotId : " << slotId_ << std::endl;
        return status;
    }
    std::cout << "Audio stopped on slotId : " << slotId_ << std::endl;
    exitHpcm_ = true;
    captureCv_.notify_all();
    bufferReadyCv_.notify_all();
    return status;
}

Status HpcmMenu::createHpcmRecordStream(StreamConfig &config) {
    telux::common::Status status = telux::common::Status::FAILED;
    //Create HPCM capture stream
    config.type = StreamType::CAPTURE;
    config.deviceTypes.clear();
    config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    status = createStream(config);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "HPCM capture stream creation failed " << std::endl;
    }
    return status;
}

Status HpcmMenu::deleteHpcmRecordStream() {
    //Delete HPCM capture stream
    if (!audioCaptureStream_){
        std::cout << "Invalid capture stream" << std::endl;
        return Status::FAILED;
    }

    std::promise<bool> promiseCapture;
    telux::common::Status status = telux::common::Status::FAILED;
    status = audioManager_-> deleteStream(
            audioCaptureStream_, [&promiseCapture,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                promiseCapture.set_value(true);
            } else {
                promiseCapture.set_value(false);
            std::cout << "Failed to delete HPCM capture stream" << std::endl;
            }});

    if (status != Status::SUCCESS) {
        std::cout << "Request to delete HPCM capture stream failed"  << std::endl;
        return status;
    }
    std::cout << "Request to delete HPCM capture stream sent" << std::endl;

    if (promiseCapture.get_future().get()) {
        audioCaptureStream_= nullptr;
        std::cout << "Audio HPCM capture stream is Deleted" << std::endl;
        return status;
    }
    std::cout << "Failed to delete HPCM capture stream" << std::endl;
    return Status::FAILED;
}

Status HpcmMenu::createHpcmPlayStream(StreamConfig &config) {
    telux::common::Status status = telux::common::Status::FAILED;
    //Create HPCM play stream
    config.type = StreamType::PLAY;
    config.deviceTypes.clear();
    config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    status = createStream(config);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "HPCM play stream creation failed " << std::endl;
    }
    return status;
}

Status HpcmMenu::deleteHpcmPlayStream() {
    //Delete HPCM play stream
    if (!audioPlayStream_){
        std::cout << "Invalid play stream" << std::endl;
        return Status::FAILED;
    }

    std::promise<bool> promisePlay;
    telux::common::Status status = telux::common::Status::FAILED;
    status = audioManager_-> deleteStream(
            audioPlayStream_, [&promisePlay,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                promisePlay.set_value(true);
            } else {
                promisePlay.set_value(false);
                std::cout << "Failed to delete HPCM play stream" << std::endl;
            }});

    if (status != Status::SUCCESS) {
        std::cout << "Request to delete HPCM play stream failed"  << std::endl;
        return status;
    }
    std::cout << "Request to delete HPCM play stream sent" << std::endl;

    if (promisePlay.get_future().get()) {
        audioPlayStream_= nullptr;
        std::cout << "Audio HPCM play stream is Deleted" << std::endl;
        /* Let the play thread know about play stream deleted/stopped */
        return status;
    }
    std::cout << "Failed to delete HPCM play stream" << std::endl;
    return Status::FAILED;
}

void HpcmMenu::startHpcmAudio(std::vector<std::string> userInput) {
    StreamConfig config;
    telux::common::Status status = telux::common::Status::FAILED;
    if (!hpcmReady_) {
        std::cout << "HPCM is not initialized" << std::endl;
        return;
    }

    if (!exitHpcm_) {
        std::cout << "HPCM is already started" << std::endl;
        return;
    }

    if (readErrorOccurred_ || writeErrorOccurred_) {
        std::cout << "Please stop the HPCM first because of the previous error"
                  << std::endl;
        return;
    }

    getUserSlotIdInput(slotId_);

    if (createActiveSession(slotId_) != Status::SUCCESS) {
        std::cout << "No running voice session for slotId : " << slotId_
            << ", please create one" << std::endl;
        return;
    }

    status = createVoiceStream(config);
    if (status != Status::SUCCESS) {
        return;
    }

    status = startVoiceStream();
    if (status != Status::SUCCESS) {
        deleteVoiceStream();
        return;
    }

    status = createHpcmRecordStream(config);
    if (status != Status::SUCCESS) {
        exitHpcm_ = true;
        stopVoiceStream();
        deleteVoiceStream();
        return;
    }

    status = createHpcmPlayStream(config);
    if (status != Status::SUCCESS) {
        exitHpcm_ = true;
        deleteHpcmRecordStream();
        stopVoiceStream();
        deleteVoiceStream();
        return;
    }

    status = startHpcm();
    if (status != Status::SUCCESS) {
        exitHpcm_ = true;
        return;
    }
}

void HpcmMenu::stopHpcmAudio(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if (!hpcmReady_) {
        std::cout << "HPCM is not initialized" << std::endl;
        return;
    }

    if (exitHpcm_) {
        std::cout << "HPCM is already stopped" << std::endl;
        return;
    }

    exitPlayThread_ = true;
    exitRecordThread_ = true;
    captureCv_.notify_all();
    bufferReadyCv_.notify_all();

    for (std::thread &th : runningThreads_) {
        if (th.joinable()){
            th.join();
        }
    }

    writeErrorOccurred_ = false;
    readErrorOccurred_ = false;
    exitPlayThread_ = false;
    exitRecordThread_ = false;

    /* Clear freeCaptureBuffers_ is necessary when startHpcm is invoked after stopHpcm.
     * If this is not done, the second startHpcm will have two buffers in freeCaptureBuffers_
     */
    while (!freeCaptureBuffers_.empty()) {
        freeCaptureBuffers_.pop();
    }

    /* Clear freePlayBuffers_ is necessary when startHpcm is invoked after stopHpcm.
     * If this is not done, the second startHpcm will have two buffers in freePlayBuffers_
     */
    while (!freePlayBuffers_.empty()) {
        freePlayBuffers_.pop();
    }

    if (setActiveSession(slotId_) != Status::SUCCESS) {
        std::cout << "No running voice session for slotId : " << slotId_
            << ", please create one" << std::endl;
        return;
    }

    status = deleteHpcmRecordStream();
    if (status != Status::SUCCESS) {
        return;
    }

    status = deleteHpcmPlayStream();
    if (status != Status::SUCCESS) {
        return;
    }

    status = stopVoiceStream();
    if (status != Status::SUCCESS) {
        return;
    }

    /* Before deleting voice stream, capture and play stream must delete */
    status = deleteVoiceStream();
    if (status != Status::SUCCESS) {
        return;
    }
}

Status HpcmMenu::createActiveSession(SlotId slotId) {
    if (setActiveSession(slotId) != Status::SUCCESS) {
        std::lock_guard<std::mutex> lk(mutex_);
        try {
            voiceSessions_[slotId] = std::make_shared<VoiceSession>();
            activeSession_ = voiceSessions_[slotId];
        } catch (std::bad_alloc &e) {
            std::cout << "Error: Create active session failed! NOMEMORY!" << std::endl;
            return Status::NOMEMORY;
        }
    }
    return Status::SUCCESS;
}

void HpcmMenu::deleteActiveSession(SlotId slotId) {
    voiceSessions_.erase(slotId);
    activeSession_ = nullptr;
    std::cout << "Voice session deleted on slotId : " << slotId_ << std::endl;
}

Status HpcmMenu::setActiveSession(SlotId slotId) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (voiceSessions_.count(slotId) && (voiceSessions_[slotId])) {
        activeSession_ = voiceSessions_[slotId];
        return Status::SUCCESS;
    } else {
        activeSession_ = nullptr;
    }
    return Status::NOSUCH;
}

/*
 *  Gets called whenever audio samples are read from the hpcm capture stream. The captured buffer is
 *  then passed to hpcm playback stream.
 */
void HpcmMenu::readCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {
    uint32_t bytesRead;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer = audioCaptureStream_->getStreamBuffer();

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read failed, err: " << static_cast<int>(error) << std::endl;
        readErrorOccurred_ = true;
    } else {
        bytesRead = buffer->getDataSize();
        streamBuffer->setDataSize(bytesRead);
        std::cout << "bytes read: " << bytesRead << std::endl;
        memcpy(streamBuffer->getRawBuffer(), buffer->getRawBuffer(), bytesRead);
        freePlayBuffers_.push(streamBuffer);
        bufferReadyCv_.notify_all();
    }
    buffer->reset();
    freeCaptureBuffers_.push(buffer);
    captureCv_.notify_all();
}


/*
 *  Read samples from the hpcm capture stream.
 */
void HpcmMenu::record() {

    uint32_t bytesToRead = 0;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::unique_lock<std::mutex> lock(captureMutex_);
    readErrorOccurred_ = false;

    for (int x = 0; x < 1; x++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            return;
        }
        freeCaptureBuffers_.push(streamBuffer);

        bytesToRead = streamBuffer->getMinSize();
        if (!bytesToRead) {
            bytesToRead =  streamBuffer->getMaxSize();
        }
        streamBuffer->setDataSize(bytesToRead);
    }

    auto readCb = std::bind(&HpcmMenu::readCompletion, this,
        std::placeholders::_1, std::placeholders::_2);

    std::cout << "HPCM recording started" << std::endl;

    while(!exitHpcm_ && !exitRecordThread_) {
        /* Wait for readCompletion(`captureCv_`) until all three of the following conditions are
         * satisfied:
         * (1) When there is no buffer in `freeCaptureBuffers_`.
         * (2) When the class is not started destructing (exitHpcm_ is false) at this point in time.
         * (3) When the user has not requested to stopHpcm (`exitRecordThread_` is false).
         * Do not required to wait if any of the following conditions satisfied:
         * (1) When `freeCaptureBuffers_` is non-empty, go ahead for read operation.
         * (2) When the class starts destruction (`exitHpcm_` is true) or user is requested to
         * stopHpcm(`exitRecordThread_` is true), it is not necessary to wait for readCompletion
         * since we are already waiting for the buffer to return before this thread is destructed.
         */
        if(freeCaptureBuffers_.empty() && !exitHpcm_ && !exitRecordThread_) {
            captureCv_.wait(lock);
        }

        /* It is safe to break from the loop when we know there is a buffer in `freeCaptureBuffers_`
         * and there is either a read or write error.
         */
        if (readErrorOccurred_ || writeErrorOccurred_) {
            /* error occurred during recording, terminate the thread */
            break;
        }

        /* Go ahead for read request:
         * (1) When there is atleast 1 `freeCaptureBuffers_`.
         */
        if (!freeCaptureBuffers_.empty()) {
            streamBuffer = freeCaptureBuffers_.front();
            freeCaptureBuffers_.pop();
            /* Once it is poped from `freeCaptureBuffers_`, always invoke read request to the server
             * under any condition(even if the class is requested to destruct, stopHpcm is requested
             * by the user), This ensures the buffer is returned to `freeCaptureBuffers_`.
             */
            if (streamBuffer && audioCaptureStream_) {
                status = audioCaptureStream_->read(streamBuffer, bytesToRead, readCb);
                if(status != telux::common::Status::SUCCESS) {
                    std::cout << "can't read, err " << static_cast<int>(status) << std::endl;
                    readErrorOccurred_ = true;
                    break;
                }
            }
        }
    }

    if (readErrorOccurred_ || writeErrorOccurred_) {
        std::cout << "recording finished with error" << std::endl;
    } else {
        std::cout << "recording finished" << std::endl;
    }
    /* If read operation returns with error then record thread will exit but play thread
    will be waiting for buffer. To avoid that notify play thread to exit. */
    bufferReadyCv_.notify_all();

    /* Wait for the pending buffer only in the following scenarios:
     * There might be a pending buffer response from the server, ensure that the pending response
     * is handled appropriately before joining the thread.
     * (1) When HPCM is exiting without invoking stopHpcm request, for example, destructor called.
     * (2) When the service is running and recording has finished, regardless of whether an error
     * occurred, but the last buffer has not yet arrived; a read/write error might occur if the
     * voice call is terminated, not yet started, or if reading from PAL failed.
     * In all these cases, wait for the buffer to arrive from the server.
     * Do not wait for the pending buffer in the following scenarios:
     * (1) When SSR occurs, the server is unable to send the pending buffer back to the application.
     */
    while((freeCaptureBuffers_.size()!= 1) ){
        if (exitHpcm_ || exitRecordThread_ || exitPlayThread_) {
            captureCv_.wait_for(lock, std::chrono::milliseconds(5000));
        } else {
            break;
        }
    }

    exitRecordThread_ = false;
}

/*
 *  Gets called to confirm how many bytes were actually written to stream.
 */
void HpcmMenu::writeCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    std::cout << "bytes played: " << bytesWritten << std::endl;
    if (!buffer) { std::cout << "Invalid buffer" << std::endl;  return; }

    if ((error != telux::common::ErrorCode::SUCCESS) ||
            (buffer->getDataSize() != bytesWritten)) {
        std::cout << "error in writting" << std::endl;
        writeErrorOccurred_ = true;
    }
}

/*
 *  This function waits for buffer to be read from hpcm capture stream and write samples
 *  on playback stream.
 */
void HpcmMenu::play() {

    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    writeErrorOccurred_ = false;

    auto writeCb = std::bind(&HpcmMenu::writeCompletion, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::cout << "HPCM playback started" << std::endl;

    while(!exitHpcm_ && !exitPlayThread_) {
        //waiting for hpcm read buffer to be ready
        std::unique_lock<std::mutex> lck(bufferReadyMutex_);
        bufferReadyCv_.wait(lck);

        if(writeErrorOccurred_ || readErrorOccurred_) {
            break;
        }

        if (!freePlayBuffers_.empty()) {
            streamBuffer = freePlayBuffers_.front();
            freePlayBuffers_.pop();
            if (!exitHpcm_ && !exitPlayThread_ && streamBuffer && audioPlayStream_) {
                status = audioPlayStream_->write(streamBuffer, writeCb);
                if(status != telux::common::Status::SUCCESS) {
                    std::cout << "can't write, err "<< static_cast<unsigned int>(status)
                        << std::endl;
                    writeErrorOccurred_ = true;
                    break;
                }
            }
        }
    }

    if (writeErrorOccurred_ || readErrorOccurred_) {
        std::cout << "Playback finished with error" << std::endl;
    } else {
        std::cout << "Playback finished" << std::endl;
    }

    /* If write operation returns with error then play thread will exit but record thread
    will be waiting for buffer. To avoid that notify record thread to exit. */
    captureCv_.notify_all();

    exitPlayThread_ = false;
}

void HpcmMenu::takeUserVoicePathInput(std::vector<telux::audio::Direction> &direction) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter voice path type (1 for RX, 2 for TX): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                if (command == 1) {
                    direction.emplace_back(telux::audio::Direction::RX);
                    break;
                }
                else if (command == 2) {
                    direction.emplace_back(telux::audio::Direction::TX);
                    break;
                }
                else {
                    std::cout << "Invalid Input" << std::endl;
                }
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void HpcmMenu::getUserSampleRateInput(uint32_t &sampleRate) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Sample Rate (8000 16000) :" ;
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> sampleRate) {
                break;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void HpcmMenu::getUserSlotIdInput(SlotId &slotId){
    std::string userInput = "";
    int slotIdInput = 0;
    while(1) {
        std::cout << "Enter the slotId : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> slotIdInput) {
                if(slotIdInput == 1){
                    slotId = SLOT_ID_1;
                    break;
                } else if(slotIdInput == 2){
                    slotId = SLOT_ID_2;
                    break;
                } else {
                    std::cout << "Invalid Input" << std::endl;
                }
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }

    }
}

Status HpcmMenu::createStream(StreamConfig &streamConfig){
    std::promise<bool> p;
    std::shared_ptr<telux::audio::IAudioStream> myAudioStream;

    //Sending a request to create audio stream
    Status audioStatus = audioManager_->createStream(streamConfig,
        [&p,&myAudioStream,this](std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                myAudioStream = audioStream;
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "failed to Create a stream" <<std::endl;
            }
        });
    if(audioStatus == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to create stream failed"  << std::endl;
        return Status::FAILED;
    }

    if (p.get_future().get()) {
        if(myAudioStream->getType() == StreamType::PLAY) {
            audioPlayStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioPlayStream>(myAudioStream);
            std::cout<< "Audio HPCM Play Stream is Created" << std::endl;
        } else if(myAudioStream->getType() == StreamType::CAPTURE) {
            audioCaptureStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioCaptureStream>(myAudioStream);
            std::cout<< "Audio HPCM Capture Stream is Created" << std::endl;
        } else {
            std::cout << "Unknown Stream type is generated" << std::endl;
        }
    } else {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}
