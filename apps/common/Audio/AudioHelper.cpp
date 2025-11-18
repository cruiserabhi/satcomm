/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       AudioHelper.cpp
 *
 * @brief      AudioHelper class provides utilities for taking user parameters for audio.
 *
 */

#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <sstream>

#include "AudioHelper.hpp"

void AudioHelper::getUserSlotIdInput(SlotId &slotId) {
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

void AudioHelper::getUserSampleRateInput(uint32_t &sampleRate) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Sample Rate (16000 32000 48000) :" ;
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

void AudioHelper::getUserChannelInput(ChannelTypeMask &channels) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter channel mask (1 for left, 2 for right, 3 for both): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                if (command == 1 || command == 2 || command == 3){
                    if (command == 1) {
                        channels = ChannelType::LEFT;
                    } else if (command == 2) {
                        channels = ChannelType::RIGHT;
                    } else {
                        channels = (ChannelType::LEFT | ChannelType::RIGHT);
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

void AudioHelper::getUserDeviceInput(std::vector<DeviceType> &devices) {

    std::string userInput = "";
    int command = -1;
    int numDevices=0;
    while(1) {
        std::cout << "Enter no. of devices : " ;
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> numDevices){
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
                devices.emplace_back(static_cast<DeviceType>(command));
                numDevices--;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void AudioHelper::getAudioFormatInput(AudioFormat &audioFormat) {
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

void AudioHelper::getUserCreateStreamInput(StreamConfig &config)
{
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    // For Voice Call the slot Id is provided from Voice Menu. Voice Menu by default uses the
    // DEFAULT_SLOT_ID, if user switches sub then corresponding slotId is used.
    if (config.type != StreamType::VOICE_CALL) {
        getUserSlotIdInput(config.slotId);
    }
    getUserSampleRateInput(config.sampleRate);
    getUserChannelInput(config.channelTypeMask);
    getUserDeviceInput(config.deviceTypes);
}

void AudioHelper::getUserVoicePathInput(std::vector<Direction> &direction) {
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
                        direction.emplace_back(Direction::RX);
                        numDir++;
                    }
                    if (command == 2 || command == 3) {
                        direction.emplace_back(Direction::TX);
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

void AudioHelper::getVolumeValueInput(float &vol) {
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

void AudioHelper::getUserVolumeInput(StreamVolume &streamVolume) {

    ChannelTypeMask channelType;
    ChannelVolume channelVolume;
    float vol;

    getUserDirectionInput(streamVolume.dir);
    getUserChannelInput(channelType);

    if(channelType != ChannelType::LEFT){
        std::cout << "For Right Channel " << std::endl;
        getVolumeValueInput(vol);
        channelVolume.channelType = ChannelType::RIGHT;
        channelVolume.vol = vol;
        streamVolume.volume.emplace_back(channelVolume);
    }

    if(channelType != ChannelType::RIGHT){
        std::cout << "For Left Channel " << std::endl;
        getVolumeValueInput(vol);
        channelVolume.channelType = ChannelType::LEFT;
        channelVolume.vol = vol;
        streamVolume.volume.emplace_back(channelVolume);
    }
}

void AudioHelper::getUserDirectionInput(StreamDirection &direction) {
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
        direction = StreamDirection::TX;
    } else if (command == 1) {
        direction = StreamDirection::RX;
    }
}

void AudioHelper::getUserMuteStatusInput(StreamMute &mute) {
    getUserDirectionInput(mute.dir);
    std::string userInput = "";
    int muteStatus;
    while(1) {
        std::cout << "Enter 0 to Unmute and 1 to Mute: " ;
        if(std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> muteStatus) {
                if(muteStatus == 0 || muteStatus == 1) {
                    mute.enable = muteStatus;
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

Status AudioHelper::getUserDtmfInput(DtmfTone &tone, uint32_t &duration, uint16_t &gain) {
    std::string userInput = "";
    std::cout << "Enter the Gain : ";
    if (std::getline(std::cin, userInput)) {
        std::stringstream inputStream(userInput);
        if (!(inputStream >> gain)) {
            std::cout << "Invalid Input" << std::endl;
            return Status::FAILED;
        }
    } else {
        std::cout << "Invalid Input" << std::endl;
        return Status::FAILED;
    }

    uint32_t lowFreq = 0;
    std::cout << "Enter the Low Frequency (697, 770, 852, 941) : ";
    if (std::getline(std::cin, userInput)) {
        std::stringstream inputStream(userInput);
        if(!(inputStream >> lowFreq)) {
            std::cout << "Invalid Input" << std::endl;
            return Status::FAILED;
        }
    } else {
        std::cout << "Invalid Input" << std::endl;
        return Status::FAILED;
    }

    uint32_t highFreq = 0;
    std::cout << "Enter the High Frequency (1209 1336 1477 1633) : ";
    if (std::getline(std::cin, userInput)) {
        std::stringstream inputStream(userInput);
        if (!(inputStream >> highFreq)) {
            std::cout << "Invalid Input" << std::endl;
            return Status::FAILED;
        }
    } else {
        std::cout << "Invalid Input" << std::endl;
        return Status::FAILED;
    }

    std::cout << "Enter the duration (in ms (0-65534) and 65535 for infinite): ";
    if (std::getline(std::cin, userInput)) {
        std::stringstream inputStream(userInput);
        if (!(inputStream >> duration)) {
            std::cout << "Invalid Input" << std::endl;
            return Status::FAILED;
        }
    } else {
        std::cout << "Invalid Input" << std::endl;
        return Status::FAILED;
    }

    Status lowFreqValid = lowFrequencyHelper(lowFreq, tone.lowFreq);
    Status highFreqValid = highFrequencyHelper(highFreq, tone.highFreq);
    if (lowFreqValid == Status::SUCCESS &&highFreqValid == Status::SUCCESS ) {
        return Status::SUCCESS;
    } else {
        return Status::FAILED;
    }
}

void AudioHelper::getUserEcnrModeInput(EcnrMode &ecnrMode) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter 0 to disable and 1 to enable ECNR mode: " ;
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                if (command == 0 || command == 1) {
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
    if (command == 0) {
        ecnrMode = EcnrMode::DISABLE;
    } else if (command == 1) {
        ecnrMode = EcnrMode::ENABLE;
    }
}


Status AudioHelper::lowFrequencyHelper(uint32_t lowFreq,
                             DtmfLowFreq &lowFrequency) {
    switch(lowFreq) {
        case Freq::Freq_697:
        lowFrequency = DtmfLowFreq::FREQ_697;
        return Status::SUCCESS;
        case Freq::Freq_770:
        lowFrequency = DtmfLowFreq::FREQ_770;
        return Status::SUCCESS;
        case Freq::Freq_852:
        lowFrequency = DtmfLowFreq::FREQ_852;
        return Status::SUCCESS;
        case Freq::Freq_941:
        lowFrequency = DtmfLowFreq::FREQ_941;
        return Status::SUCCESS;
        default:
        std::cout << "unsupported Dtmf Frequency " << std::endl;
        return Status::FAILED;
    }
}

Status AudioHelper::highFrequencyHelper(uint32_t highFreq,
                             DtmfHighFreq &highFrequency) {
    switch(highFreq) {
        case Freq::Freq_1209:
        highFrequency = DtmfHighFreq::FREQ_1209;
        return Status::SUCCESS;
        case Freq::Freq_1336:
        highFrequency = DtmfHighFreq::FREQ_1336;
        return Status::SUCCESS;
        case Freq::Freq_1477:
        highFrequency = DtmfHighFreq::FREQ_1477;
        return Status::SUCCESS;
        case Freq::Freq_1633:
        highFrequency = DtmfHighFreq::FREQ_1633;
        return Status::SUCCESS;
        default:
        std::cout << "unsupported Frequency " << std::endl;
        return Status::FAILED;
    }
}
