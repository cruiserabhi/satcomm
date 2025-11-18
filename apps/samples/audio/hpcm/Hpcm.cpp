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
 * Steps to receive audio on TX/RX path, modify it and write back to the TX/RX path
 * are as follows:
 *
 *  1. Get an AudioFactory instance.
 *  2. Get an IAudioManager instance from the AudioFactory.
 *  3. Wait for the audio service to become available.
 *  4. Create a voice call stream.
 *  5. Start a voice call stream.
 *  6. Create a playback stream on TX path.
 *  7. Create a capture stream on TX path.
 *  8. Create a playback stream on RX path.
 *  9. Create a capture stream on RX path.
 * 10. Allocate buffers to send and receive audio samples.
 * 11. Create a thread that will receive audio from TX path, modifies it and write back
 *     to the TX path.
 * 12. Create a thread that will receive audio from RX path, modifies it and write back
 *     to the RX path.
 * 13. When the use case is over, delete TX playback stream.
 * 14. When the use case is over, delete TX capture stream.
 * 15. When the use case is over, delete RX playback stream.
 * 16. When the use case is over, delete RX capture stream.
 * 17. Stop voice call stream.
 * 18. Delete voice call stream.
 *
 * Usage:
 * # hpcm_tx_rx_modify
 *
 * A voice call is established, audio spoken on the local mic is heard on the remote end.
 * Voice spoken on the remote end is header on the local speaker.
 *
 * For establishing cellular RF path for voice call, telephony APIs should be used.
 */

#include <errno.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <cstring>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "Hpcm.hpp"

/*
 * Initialize application and get an audio service.
 */
int Hpcm::init() {

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
 *  Step - 4, create a voice call stream with hpcm enabled.
 */
int Hpcm::createVoiceStream() {

    telux::common::ErrorCode ec;
    telux::common::Status status;
    telux::audio::StreamConfig sc{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::VOICE_CALL;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.enableHpcm = true;

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

    std::cout << "Voice stream created" << std::endl;
    return 0;
}

/*
 *  Step - 12, delete voice call stream.
 */
int Hpcm::deleteVoiceStream() {

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

    std::cout << "Voice stream deleted" << std::endl;
    return 0;
}

/*
 *  Step - 5, start voice call stream.
 */
int Hpcm::startVoiceStream() {

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

    std::cout << "Voice stream started" << std::endl;
    return 0;
}

/*
 * Step - 11, stop voice call stream.
 */
int Hpcm::stopVoiceStream() {

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

    std::cout << "Voice stream stopped" << std::endl;
    return 0;
}

int Hpcm::createTXPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::PLAY;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.enableHpcm = true;
    /* Direction::TX indicates voice uplink */
    sc.voicePaths.emplace_back(telux::audio::Direction::TX);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            txPlayStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create tx playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create tx playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "TX playback stream created" << std::endl;
    return 0;
}

int Hpcm::deleteTXPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(txPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete tx playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete tx playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "TX playback stream deleted" << std::endl;
    return 0;
}

int Hpcm::createTXCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    sc.enableHpcm = true;
    /* Direction::TX indicates voice uplink */
    sc.voicePaths.emplace_back(telux::audio::Direction::TX);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            txCaptureStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create tx capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create tx capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "TX capture stream created" << std::endl;
    return 0;
}

int Hpcm::deleteTXCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(txCaptureStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete tx capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete tx capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "TX capture stream deleted" << std::endl;
    return 0;
}

int Hpcm::createRXPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::PLAY;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.enableHpcm = true;
    /* Direction::RX indicates voice downlink */
    sc.voicePaths.emplace_back(telux::audio::Direction::RX);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            rxPlayStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create rx playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create rx playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "RX playback stream created" << std::endl;
    return 0;
}

int Hpcm::deleteRXPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(rxPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete rx playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete rx playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "RX playback stream deleted" << std::endl;
    return 0;
}

int Hpcm::createRXCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    sc.enableHpcm = true;
    /* Direction::RX indicates voice downlink */
    sc.voicePaths.emplace_back(telux::audio::Direction::RX);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            rxCaptureStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create rx capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create rx capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "RX capture stream created" << std::endl;
    return 0;
}

int Hpcm::deleteRXCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(rxCaptureStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete rx capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete rx capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "RX capture stream deleted" << std::endl;
    return 0;
}

int Hpcm::allocateBuffers() {

    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    txReadSize_ = 0;
    rxReadSize_ = 0;

    for (int32_t x = 0; x < BUFFER_COUNT; x++) {
        streamBuffer = txCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get tx capture stream buffer" << std::endl;
            txReadBuffers_ = {};
            return -ENOMEM;
        }

        txReadSize_ = streamBuffer->getMinSize();
        if (!txReadSize_) {
            txReadSize_ =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(txReadSize_);
        txReadBuffers_.push(streamBuffer);
    }

    for (int32_t x = 0; x < BUFFER_COUNT; x++) {
        streamBuffer = txPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get tx play stream buffer" << std::endl;
            txReadBuffers_ = {};
            txWriteBuffers_ = {};
            return -ENOMEM;
        }

        streamBuffer->setDataSize(txReadSize_);
        txWriteBuffers_.push(streamBuffer);
    }

    for (int32_t x = 0; x < BUFFER_COUNT; x++) {
        streamBuffer = rxCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get rx capture stream buffer" << std::endl;
            rxReadBuffers_ = {};
            txReadBuffers_ = {};
            txWriteBuffers_ = {};
            return -ENOMEM;
        }

        rxReadSize_ = streamBuffer->getMinSize();
        if (!rxReadSize_) {
            rxReadSize_ =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(rxReadSize_);
        rxReadBuffers_.push(streamBuffer);
    }

    for (int32_t x = 0; x < BUFFER_COUNT; x++) {
        streamBuffer = rxPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get rx play stream buffer" << std::endl;
            rxReadBuffers_ = {};
            rxWriteBuffers_ = {};
            txReadBuffers_ = {};
            txWriteBuffers_ = {};
            return -ENOMEM;
        }

        streamBuffer->setDataSize(rxReadSize_);
        rxWriteBuffers_.push(streamBuffer);
    }

    return 0;
}

void Hpcm::writeCompleteTX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "write tx err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> txReadLock(txReadMutex_);
            keepRunning_ = false;
            txReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> txReadLock(txReadMutex_);

        txWriteBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++txWritePossible_;
        }

        txReadWaiterCv_.notify_all();
    }
}

void Hpcm::readCompleteTX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read tx err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> txReadLock(txReadMutex_);
            keepRunning_ = false;
            txReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> txReadLock(txReadMutex_);

        readyForTxWriteBuffers_.push(buffer);
        txReadBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++txReadPossible_;
            if (txReadDone_ < BUFFER_COUNT) {
                ++txReadDone_;
            }
        }

        txReadWaiterCv_.notify_all();
    }
}

void Hpcm::readFromTXwriteOnTX() {

    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::shared_ptr<telux::audio::IStreamBuffer> tmpBufferPtr;

    auto txReadCompleteCb = std::bind(&Hpcm::readCompleteTX, this,
        std::placeholders::_1, std::placeholders::_2);

    auto txWriteCompleteCb = std::bind(&Hpcm::writeCompleteTX, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::unique_lock<std::mutex> txReadLock(txReadMutex_);

    txReadDone_ = 0;
    txReadPossible_ = BUFFER_COUNT;
    txWritePossible_ = BUFFER_COUNT;

    std::cout << "readFromTXwriteOnTX started" << std::endl;

    while (keepRunning_) {
        if (txReadDone_ && txWritePossible_) {

            tmpBufferPtr = readyForTxWriteBuffers_.front();
            readyForTxWriteBuffers_.pop();

            streamBuffer = txWriteBuffers_.front();
            txWriteBuffers_.pop();

            txReadDone_--;

            /*
             * In this example whatever audio samples are read we write them back without
             * modification. If required, an application can modify it and then write back.
             */
            std::memcpy(streamBuffer->getRawBuffer(),
                tmpBufferPtr->getRawBuffer(), txReadSize_);

            status = txPlayStream_->write(streamBuffer, txWriteCompleteCb);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "tx write err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                rxReadWaiterCv_.notify_all();
                break;
            }

            txWritePossible_--;
        }

        if (txReadPossible_) {
            streamBuffer = txReadBuffers_.front();
            txReadBuffers_.pop();

            status = txCaptureStream_->read(streamBuffer, txReadSize_, txReadCompleteCb);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "tx read err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                rxReadWaiterCv_.notify_all();
                break;
            }

            txReadPossible_--;
        }

        txReadWaiterCv_.wait(txReadLock, [this] {
            return ((txReadPossible_ ||
                (txReadDone_ && txWritePossible_)) ? true : false);
        });
    }

    while ((txReadBuffers_.size() != static_cast<uint32_t>(BUFFER_COUNT)) &&
            (txWriteBuffers_.size() != static_cast<uint32_t>(BUFFER_COUNT))) {
        txReadWaiterCv_.wait(txReadLock);
    }

    std::cout << "readFromTXwriteOnTX completed" << std::endl;
}

void Hpcm::writeCompleteRX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "write rx err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> rxReadLock(rxReadMutex_);
            keepRunning_ = false;
            rxReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> rxReadLock(rxReadMutex_);

        rxWriteBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++rxWritePossible_;
        }

        rxReadWaiterCv_.notify_all();
    }
}

void Hpcm::readCompleteRX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read rx err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> rxReadLock(rxReadMutex_);
            keepRunning_ = false;
            rxReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> rxReadLock(rxReadMutex_);

        readyForRxWriteBuffers_.push(buffer);
        rxReadBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++rxReadPossible_;
            if (rxReadDone_ < BUFFER_COUNT) {
                ++rxReadDone_;
            }
        }

        rxReadWaiterCv_.notify_all();
    }
}

void Hpcm::readFromRXwriteOnRX() {

    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::shared_ptr<telux::audio::IStreamBuffer> tmpBufferPtr;

    auto rxReadCompleteCb = std::bind(&Hpcm::readCompleteRX, this,
        std::placeholders::_1, std::placeholders::_2);

    auto rxWriteCompleteCb = std::bind(&Hpcm::writeCompleteRX, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::unique_lock<std::mutex> rxReadLock(rxReadMutex_);

    rxReadDone_ = 0;
    rxReadPossible_ = BUFFER_COUNT;
    rxWritePossible_ = BUFFER_COUNT;

    std::cout << "readFromRXwriteOnRX started" << std::endl;

    while (keepRunning_) {
        if (rxReadDone_ && rxWritePossible_) {

            tmpBufferPtr = readyForRxWriteBuffers_.front();
            readyForRxWriteBuffers_.pop();

            streamBuffer = rxWriteBuffers_.front();
            rxWriteBuffers_.pop();

            rxReadDone_--;

            /*
             * In this example whatever audio samples are read we write them back without
             * modification. If required, an application can modify it and then write back.
             */
            std::memcpy(streamBuffer->getRawBuffer(),
                tmpBufferPtr->getRawBuffer(), rxReadSize_);

            status = rxPlayStream_->write(streamBuffer, rxWriteCompleteCb);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "rx write err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                txReadWaiterCv_.notify_all();
                break;
            }

            rxWritePossible_--;
        }

        if (rxReadPossible_) {
            streamBuffer = rxReadBuffers_.front();
            rxReadBuffers_.pop();

            status = rxCaptureStream_->read(
                streamBuffer, rxReadSize_, rxReadCompleteCb);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "rx read err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                txReadWaiterCv_.notify_all();
                break;
            }

            rxReadPossible_--;
        }

        rxReadWaiterCv_.wait(rxReadLock, [this] {
            return ((rxReadPossible_ ||
                (rxReadDone_ && rxWritePossible_)) ? true : false);
        });
    }

    while ((rxReadBuffers_.size() != static_cast<uint32_t>(BUFFER_COUNT)) &&
            (rxWriteBuffers_.size() != static_cast<uint32_t>(BUFFER_COUNT))) {
        rxReadWaiterCv_.wait(rxReadLock);
    }

    std::cout << "readFromRXwriteOnRX completed" << std::endl;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<Hpcm> app;

    try {
        app = std::make_shared<Hpcm>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate Hpcm" << std::endl;
        return -ENOMEM;
    }

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

    ret = app->createTXPlayStream();
    if (ret < 0) {
        return ret;
    }

    ret = app->createTXCaptureStream();
    if (ret < 0) {
        app->deleteTXPlayStream();
        return ret;
    }

    ret = app->createRXPlayStream();
    if (ret < 0) {
        app->deleteTXPlayStream();
        app->deleteTXCaptureStream();
        return ret;
    }

    ret = app->createRXCaptureStream();
    if (ret < 0) {
        app->deleteTXPlayStream();
        app->deleteTXCaptureStream();
        app->deleteRXPlayStream();
        return ret;
    }

    ret = app->allocateBuffers();
    if (ret < 0) {
        app->deleteRXPlayStream();
        app->deleteRXCaptureStream();
        app->deleteTXPlayStream();
        app->deleteTXCaptureStream();
        return ret;
    }

    std::thread txAudioModifier(&Hpcm::readFromRXwriteOnRX, &(*app));
    std::thread rxAudioModifier(&Hpcm::readFromTXwriteOnTX, &(*app));

    /* Run the use case for 5 minutes as an example */
    std::this_thread::sleep_for(std::chrono::minutes(2));
    app->keepRunning_ = false;
    {
        std::lock_guard<std::mutex> rxReadLock(app->rxReadMutex_);
        app->rxReadWaiterCv_.notify_all();
    }
    {
        std::lock_guard<std::mutex> txReadLock(app->txReadMutex_);
        app->txReadWaiterCv_.notify_all();
    }

    txAudioModifier.join();
    rxAudioModifier.join();

    ret = app->deleteTXPlayStream();
    if (ret < 0) {
        app->deleteTXCaptureStream();
        app->deleteRXPlayStream();
        app->deleteRXCaptureStream();
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteTXCaptureStream();
    if (ret < 0) {
        app->deleteRXPlayStream();
        app->deleteRXCaptureStream();
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteRXPlayStream();
    if (ret < 0) {
        app->deleteRXCaptureStream();
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteRXCaptureStream();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->stopVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteVoiceStream();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
