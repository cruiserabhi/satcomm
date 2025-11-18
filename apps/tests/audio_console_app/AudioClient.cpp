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
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * Audio Client class provides functionality in SDK to create Audio Stream,
 * start/stop Audio on the created Stream and delete the Stream.
 */

#include <chrono>
#include <iostream>
#include <stdio.h>
#include <dirent.h>

#include "AudioClient.hpp"

AudioClient::AudioClient(std::shared_ptr<IAudioManager> audioManager) {
    sampleRate_ = 0;
    channelType_ = 0;
    filePath_ = "";
    audioManager_ = audioManager;
    stream_ = nullptr;
    audioVoiceStream_ = nullptr;
    audioPlayStream_ = nullptr;
    audioCaptureStream_ = nullptr;
    audioLoopbackStream_ = nullptr;
    audioToneStream_ = nullptr;
}

AudioClient::~AudioClient() {
}

void AudioClient::cleanup() {
    std::lock_guard<std::mutex> lock(cleanupMtx_);
    sampleRate_ = 0;
    channelType_ = 0;
    filePath_ = "";
    stream_ = nullptr;
    audioVoiceStream_ = nullptr;
    audioPlayStream_ = nullptr;
    audioCaptureStream_ = nullptr;
    audioLoopbackStream_ = nullptr;
    audioToneStream_ = nullptr;
}

void AudioClient::resolveStreamType(StreamType streamType, SlotId slotId) {
    if (streamType == StreamType::VOICE_CALL) {
        if (slotId == SLOT_ID_1) {
            stream_ = audioVoiceStream_;
        } else if (slotId == SLOT_ID_2) {
            stream_ = audioVoiceStream2_;
        }
    } else if( streamType == StreamType::PLAY) {
        stream_ = audioPlayStream_;
    } else if( streamType == StreamType::CAPTURE) {
        stream_ = audioCaptureStream_;
    } else if( streamType == StreamType::LOOPBACK) {
        stream_ = audioLoopbackStream_;
    } else if( streamType == StreamType::TONE_GENERATOR) {
        stream_ = audioToneStream_;
    } else {
        stream_ = nullptr;
    }
}

void AudioClient::takeUserSlotIdInput(SlotId &slotId) {
    std::string userInput = "";
    int input = INVALID_SLOT_ID;
    while(1) {
        std::cout << "Enter Slot Id: ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> input) {
                slotId = static_cast<SlotId>(input);
                break;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void AudioClient::takeUserSampleRateInput(uint32_t &userSampleRate) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Sample Rate (16000 32000 48000) :" ;
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> userSampleRate) {
                break;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void AudioClient::takeUserChannelInput(telux::audio::ChannelTypeMask &channelType) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter channel mask (1 for left, 2 for right, 3 for both): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> command) {
                if(command == 1 || command == 2 || command == 3){
                    if(command == 1) {
                        channelType = ChannelType::LEFT;
                    } else if(command == 2) {
                        channelType = ChannelType::RIGHT;
                    } else {
                        channelType = (ChannelType::LEFT | ChannelType::RIGHT);
                    }
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

void AudioClient::takeUserDeviceInput(std::vector<telux::audio::DeviceType> &devices,
    StreamType &streamType) {

    std::string userInput = "";
    int command = -1;
    int numDevices=0;
    if (streamType == StreamType::LOOPBACK) {
        std::cout << "Note: This Stream requires two devices" << std::endl;
        std::cout << "Please provide first device as RX and second as TX" << std::endl;
    }
    while(1) {
        std::cout << "Enter no. of devices : " ;
        if(std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> numDevices){
                break;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        }
    }

    while(numDevices) {
        std::cout << "Enter device type (1 for speaker, 257 for microphone): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                devices.emplace_back(static_cast<telux::audio::DeviceType>(command));
                numDevices--;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void AudioClient::takeUserVoicePathInput(std::vector<telux::audio::Direction> &direction) {
    std::string userInput = "";
    int command = -1;
    int numDir=0;
    while(1) {
        std::cout << "Enter voice path type (0 for None, 1 for RX, 2 for TX, 3 for BOTH): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                if (command >=0 && command <=3) {
                    if (command == 1 || command == 3) {
                        direction.emplace_back(telux::audio::Direction::RX);
                        numDir++;
                    }
                    if (command == 2 || command == 3) {
                        direction.emplace_back(telux::audio::Direction::TX);
                        numDir++;
                    }
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

void AudioClient::takeVolumeValueInput(float &vol) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Volume :";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> vol) {
                break;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void AudioClient::takeUserVolumeInput(StreamVolume &streamVolume) {

    ChannelTypeMask channelType;
    ChannelVolume channelVolume;
    float vol;

    takeUserDirectionInput(streamVolume.dir);
    takeUserChannelInput(channelType);

    if(channelType != ChannelType::LEFT){
        std::cout << "For Right Channel " << std::endl;
        takeVolumeValueInput(vol);
        channelVolume.channelType = ChannelType::RIGHT;
        channelVolume.vol = vol;
        streamVolume.volume.emplace_back(channelVolume);
    }

    if(channelType != ChannelType::RIGHT){
        std::cout << "For Left Channel " << std::endl;
        takeVolumeValueInput(vol);
        channelVolume.channelType = ChannelType::LEFT;
        channelVolume.vol = vol;
        streamVolume.volume.emplace_back(channelVolume);
    }
}

void AudioClient::takeAudioFormatInput(AudioFormat &audioFormat) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Please Select Audio Format : 1->PCM, 2->AMRWB+, 3->AMRNB, 4->AMRWB :";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                if (command == 1 || command == 2 || command == 3 || command == 4) {
                    if (command == 1) {
                        audioFormat = AudioFormat::PCM_16BIT_SIGNED;
                    } else if (command == 2) {
                        audioFormat = AudioFormat::AMRWB_PLUS;
                    } else if (command == 3) {
                        audioFormat = AudioFormat::AMRNB;
                    } else if (command == 4) {
                        audioFormat = AudioFormat::AMRWB;
                    }
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

void AudioClient::takeUserCreateStreamInput(telux::audio::StreamConfig &config)
{
    config.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    // For Voice Call the slot Id is provided from Voice Menu. Voice Menu by default uses the
    // DEFAULT_SLOT_ID, if user switches sub then corresponding slotId is used.
    if (config.type != StreamType::VOICE_CALL) {
        takeUserSlotIdInput(config.slotId);
    }
    takeUserSampleRateInput(config.sampleRate);
    takeUserChannelInput(config.channelTypeMask);
    sampleRate_ = config.sampleRate;
    channelType_ = config.channelTypeMask;

    if (config.type == telux::audio::StreamType::PLAY) {
        takeAudioFormatInput(config.format);
        FILE * file;
        while(1) {
            std::cout << "Enter File name with path :" ;
            std::getline(std::cin, filePath_);
            DIR* directory = opendir(filePath_.c_str());
            if (directory != NULL) {
                std::cout << "Please enter valid file path" << std::endl;
            } else {
                file = fopen(filePath_.c_str(),"r");
                if(file) {
                    fseek(file, 0 , SEEK_SET);
                    break;
                } else {
                    perror("Error : ");
                }
            }
        }
        fclose(file);
    }
    takeUserDeviceInput(config.deviceTypes, config.type);
    if ((config.type == telux::audio::StreamType::PLAY) ||
        (config.type == telux::audio::StreamType::CAPTURE)) {
      takeUserVoicePathInput(config.voicePaths);
    }
}

void AudioClient::takeUserDirectionInput(StreamDirection &direction) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter direction of stream: (0 for TX, 1 for RX) ";
        if(std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> command){
                if(command == 0 || command == 1) {
                    break;
                } else {
                    std::cout << "Invalid Input" << std::endl;
                }
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        }
    }

    if(command == 0) {
        direction = telux::audio::StreamDirection::TX;
    } else if (command == 1) {
        direction = telux::audio::StreamDirection::RX;
    }
}

std::shared_ptr<IAudioStream> AudioClient::getStream(StreamType streamtype, SlotId slotId) {
    resolveStreamType(streamtype, slotId);
    return stream_;
}

void AudioClient::getCaptureConfig(uint32_t &sampleRate, uint32_t &channelType) {
    sampleRate = sampleRate_;
    channelType = channelType_;
}

void AudioClient::getPlayConfig(std::string &filePath, AudioFormat &playFormat) {
    filePath = filePath_;
    playFormat = playFormat_;
}

Status AudioClient::createStream(StreamType streamType, SlotId slotId) {

    std::promise<bool> p;
    StreamConfig streamConfig;
    // Initialising the Configuration of stream
    streamConfig.type = streamType;
    if (streamType == StreamType::VOICE_CALL) {
        streamConfig.slotId = slotId;
    }
    takeUserCreateStreamInput(streamConfig);
    AmrwbpParams formatParams{};
    if (streamConfig.format == AudioFormat::AMRWB_PLUS) {
        formatParams.bitWidth = 16;
        formatParams.frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
        streamConfig.formatParams = static_cast<FormatParams*>(&formatParams);
        playFormat_ = AudioFormat::AMRWB_PLUS;
    } else if (streamConfig.format == AudioFormat::PCM_16BIT_SIGNED) {
        playFormat_ = AudioFormat::PCM_16BIT_SIGNED;
        streamConfig.formatParams = nullptr;
    } else if (streamConfig.format == AudioFormat::AMRWB) {
        playFormat_ = AudioFormat::AMRWB;
        streamConfig.formatParams = nullptr;
    } else if (streamConfig.format == AudioFormat::AMRNB) {
        playFormat_ = AudioFormat::AMRNB;
        streamConfig.formatParams = nullptr;
    }

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
        std::cout<< "Audio Stream is Created" << std::endl;
        if(myAudioStream->getType() == StreamType::VOICE_CALL) {
            if (streamConfig.slotId == SLOT_ID_1) {
                audioVoiceStream_ = std::dynamic_pointer_cast<
                            telux::audio::IAudioVoiceStream>(myAudioStream);
            }
            if (streamConfig.slotId == SLOT_ID_2) {
                audioVoiceStream2_ = std::dynamic_pointer_cast<
                            telux::audio::IAudioVoiceStream>(myAudioStream);
            }
            std::cout<< "Voice Stream is Created on slot id "<< streamConfig.slotId << std::endl;

        } else if(myAudioStream->getType() == StreamType::PLAY) {
            audioPlayStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioPlayStream>(myAudioStream);
            std::cout<< "Audio Play Stream is Created" << std::endl;
        } else if(myAudioStream->getType() == StreamType::CAPTURE) {
            audioCaptureStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioCaptureStream>(myAudioStream);
            std::cout<< "Audio Capture Stream is Created" << std::endl;
        } else if(myAudioStream->getType() == StreamType::LOOPBACK) {
            audioLoopbackStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioLoopbackStream>(myAudioStream);
            std::cout<< "Audio loopback Stream is Created" << std::endl;
        } else if (myAudioStream->getType() == StreamType::TONE_GENERATOR) {
            audioToneStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioToneGeneratorStream>(myAudioStream);
            std::cout<< "Audio tone generator Stream is Created" << std::endl;
        } else {
            std::cout << "Unknown Stream type is generated" << std::endl;
        }
    } else {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status AudioClient::deleteStream(StreamType streamType, SlotId slotId) {
    resolveStreamType(streamType, slotId);
    std::promise<bool> p;
    telux::common::Status deleteStreamStatus = audioManager_-> deleteStream(
    stream_, [&p,this](telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
        p.set_value(true);
        } else {
        p.set_value(false);
        std::cout << "Failed to delete a stream" << std::endl;
        }
    });
    if(deleteStreamStatus == Status::SUCCESS) {
        std::cout << "request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
        return Status::FAILED;
    }
    if (p.get_future().get()) {
        if(streamType == StreamType::VOICE_CALL) {
            if (slotId == SLOT_ID_1) {
                audioVoiceStream_= nullptr;
            }
            if (slotId == SLOT_ID_2) {
                audioVoiceStream2_ = nullptr;
            }
        } else if(streamType == StreamType::PLAY) {
            audioPlayStream_= nullptr;
        } else if(streamType == StreamType::CAPTURE) {
            audioCaptureStream_= nullptr;
        } else if(streamType == StreamType::LOOPBACK) {
            audioLoopbackStream_= nullptr;
        } else if(streamType == StreamType::TONE_GENERATOR) {
            audioToneStream_= nullptr;
        } else {
            std::cout << " Unknown Stream Type " << std::endl;
        }
        std::cout << "Audio Stream is Deleted" << std::endl;
    } else {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

void AudioClient::getStreamDevice(StreamType streamType, SlotId slotId) {
    resolveStreamType(streamType, slotId);
    std::promise<bool> p;
    std::vector<telux::audio::DeviceType> devices_;

    cleanupMtx_.lock();
    if(stream_) {
        telux::common::Status status = stream_->getDevice(
            [&p, &devices_, this](std::vector<telux::audio::DeviceType> devices,
                     telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                devices_ = devices;
                p.set_value(true);
            } else {
                std::cout << "Failed to get stream device" << std::endl;
                p.set_value(false);
            }
        });
        cleanupMtx_.unlock();
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to get device sent" << std::endl;
        } else {
            std::cout << "Request to get device failed" << std::endl;
            return;
        }

        if (p.get_future().get()) {
            for (auto deviceType : devices_) {
                std::string deviceName;
                std::cout << "Device type: "  << (static_cast<uint32_t>(deviceType)) << std::endl;
                std::cout.flush();
            }
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
        cleanupMtx_.unlock();
    }
}

void AudioClient::setStreamDevice(StreamType streamType, SlotId slotId) {
    resolveStreamType(streamType, slotId);
    std::promise<bool> p;
    std::vector<telux::audio::DeviceType> devices;
    takeUserDeviceInput(devices, streamType);

    cleanupMtx_.lock();
    if(stream_) {
        telux::common::Status status = stream_->setDevice(devices,
           [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to set stream device" << std::endl;
            }
        });
        cleanupMtx_.unlock();
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to set device sent" << std::endl;
        } else {
            std::cout << "Request to set device failed" << std::endl;
            return;
        }
        if (p.get_future().get()) {
             std::cout << "set stream device succeeded." << std::endl;
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
        cleanupMtx_.unlock();
    }
}

void AudioClient::setVolume(StreamType streamType, SlotId slotId) {
    resolveStreamType(streamType, slotId);
    std::promise<bool> p;
    telux::audio::StreamVolume streamVol;
    takeUserVolumeInput(streamVol);

    cleanupMtx_.lock();
    if(stream_) {
        telux::common::Status status = stream_->setVolume(streamVol,
              [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to set stream volume" << std::endl;
            }
        });
        cleanupMtx_.unlock();
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to set volume sent" << std::endl;
        } else {
            std::cout << "Request to set volume failed" << std::endl;
            return;
        }
        if (p.get_future().get()) {
            std::cout << "setStreamVolume() succeeded." << std::endl;
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
        cleanupMtx_.unlock();
    }
}

void AudioClient::getVolume(StreamType streamType, SlotId slotId) {
    resolveStreamType(streamType, slotId);
    std::promise<bool> p;
    telux::audio::StreamVolume vol;
    telux::audio::StreamDirection dir;
    takeUserDirectionInput(dir);

    cleanupMtx_.lock();
    if(stream_) {
        telux::common::Status status = stream_->getVolume(
        dir,  [&p,&vol,this](telux::audio::StreamVolume volume, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                vol = volume;
                p.set_value(true);
            } else {
                std::cout << "Failed to get stream volume" << std::endl;
                p.set_value(false);
            }
        });
        cleanupMtx_.unlock();
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to get volume sent" << std::endl;
        } else {
            std::cout << "Request to get volume failed" << std::endl;
            return;
        }

        if (p.get_future().get()) {
            for (auto channelVolume : vol.volume) {
                std::cout << "Volume: "<< channelVolume.vol << std::endl;
                std::cout.flush();
            }
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
        cleanupMtx_.unlock();
    }

}

void AudioClient::setMute(StreamType streamType, SlotId slotId) {
    resolveStreamType(streamType, slotId);
    StreamMute mute;
    std::promise<bool> p;

    takeUserDirectionInput(mute.dir);

    std::string userInput = "";
    int muteStatus;
    while(1) {
        std::cout << " Enter 0 to Unmute and 1 to Mute" ;
        if(std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> muteStatus) {
                if(muteStatus == 0 || muteStatus == 1) {
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

    cleanupMtx_.lock();
    if(stream_) {
        if(muteStatus == 0) {
            mute.enable = false;
        } else if (muteStatus == 1) {
            mute.enable = true;
        }
        telux::common::Status status = stream_->setMute(mute,
               [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to set mute" << std::endl;
            }
        });
        cleanupMtx_.unlock();
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to set mute sent " << std::endl;
        } else {
            std::cout << "Request to set mute failed" << std::endl;
            return;
        }
        if (p.get_future().get()) {
            if (mute.enable) {
                std::cout << "Stream Muted" << std::endl;
            } else {
                std::cout << "Stream Unmuted" << std::endl;
            }
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
        cleanupMtx_.unlock();
    }
}

void AudioClient::getMute(StreamType streamType, SlotId slotId) {
    resolveStreamType(streamType, slotId);
    std::promise<bool> p;
    telux::audio::StreamMute mute_;
    telux::audio::StreamDirection dir;
    std::string input;
    takeUserDirectionInput(dir);

    cleanupMtx_.lock();
    if (stream_) {
        telux::common::Status status = stream_->getMute(
               dir,  [&p,&mute_,this](telux::audio::StreamMute mute,
               telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                mute_ = mute;
                p.set_value(true);
            } else {
                std::cout << "Failed to get mute" << std::endl;
                p.set_value(false);
            }
        });
        cleanupMtx_.unlock();
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to get mute sent" << std::endl;
        } else {
            std::cout << "Request to get mute failed" << std::endl;
            return;
        }
        if (p.get_future().get()) {
            std::string muteStatus;
            if(mute_.enable) {
                muteStatus = "Muted";
            } else {
                muteStatus = "Unmuted";
            }
            std::cout << "Mute status: " << muteStatus << std::endl;
            std::cout.flush();
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
        cleanupMtx_.unlock();
    }
}
