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

#include "VoiceMenu.hpp"

VoiceMenu::VoiceMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor)
   , ready_(false)
   , slotId_(DEFAULT_SLOT_ID) {
}

VoiceMenu::~VoiceMenu() {
}

void VoiceMenu::init() {
    std::shared_ptr<ConsoleAppCommand> createStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Create Stream", {},
            std::bind(&VoiceMenu::createStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Delete Stream", {},
            std::bind(&VoiceMenu::deleteStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "3", "Get Device", {}, std::bind(&VoiceMenu::getDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "Set Device", {}, std::bind(&VoiceMenu::setDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "Get Volume", {}, std::bind(&VoiceMenu::getVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "Set Volume", {}, std::bind(&VoiceMenu::setVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get Mute Status", {},
            std::bind(&VoiceMenu::getMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "8", "Set Mute", {}, std::bind(&VoiceMenu::setMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startAudioCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Start Audio", {},
            std::bind(&VoiceMenu::startAudio, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopAudioCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "10", "Stop Audio", {}, std::bind(&VoiceMenu::stopAudio, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startDtmfCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "Start Dtmf Tone", {},
            std::bind(&VoiceMenu::startDtmf, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopDtmfCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12", "Stop Dtmf Tone", {},
            std::bind(&VoiceMenu::stopDtmf, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> regListenerCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13", "Register Listener", {},
            std::bind(&VoiceMenu::registerListener, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deregListenerCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("14", "Deregister Listener", {},
            std::bind(&VoiceMenu::deRegisterListener, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> changeSlotIdCmd = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("15", "Switch Slot ID", {}, std::bind(&VoiceMenu::changeSlotId, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> voiceMenuCommandsList = {createStreamCommand,
        deleteStreamCommand, getDeviceCommand, setDeviceCommand, getVolumeCommand, setVolumeCommand,
        getMuteCommand, setMuteCommand, startAudioCommand, stopAudioCommand, startDtmfCommand,
        stopDtmfCommand, regListenerCmd, deregListenerCmd, changeSlotIdCmd};
    ready_ = true;
    ConsoleApp::addCommands(voiceMenuCommandsList);
}

void VoiceMenu::setSystemReady() {
    ready_ = true;
}

void VoiceMenu::cleanup() {
    std::lock_guard<std::mutex> lk(mutex_);
    ready_ = false;
    voiceSessions_.clear();
    activeSession_ = nullptr;
}

void VoiceMenu::createStream(std::vector<std::string> userInput) {
    if (ready_) {
        if (createActiveSession(slotId_) == Status::SUCCESS) {
            StreamConfig config;
            config.slotId = slotId_;
            config.type = StreamType::VOICE_CALL;
            AudioHelper::getUserCreateStreamInput(config);
            AudioHelper::getUserEcnrModeInput(config.ecnrMode);
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->createStream(config);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Stream created on slotId : " << slotId_ << std::endl;
                } else if(status == Status::ALREADY) {
                    std::cout << "Stream exist please delete first" << std::endl;
                } else {
                    deleteActiveSession(slotId_);
                    std::cout << "Stream creation failed on slotId : " << slotId_ << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::deleteStream(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->deleteStream();
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    deleteActiveSession(slotId_);
                    std::cout << "Voice stream deleted on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Voice stream deletion failed on slotId : " << slotId_ << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::getDevice(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            std::vector<DeviceType> devices;
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->getStreamDevice(devices);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    for (auto deviceType : devices) {
                        std::string deviceName;
                        std::cout << "Device Type" << (static_cast<uint32_t>(deviceType)) << std::endl;
                    }
                } else {
                    std::cout << "Get Device Request Failed." << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::setDevice(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            std::vector<DeviceType> devices;
            AudioHelper::getUserDeviceInput(devices);
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->setStreamDevice(devices);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Device set successfully." << std::endl;
                } else {
                    std::cout << "Device set failed." << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::getVolume(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            StreamVolume volume;
            AudioHelper::getUserDirectionInput(volume.dir);
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->getVolume(volume);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    for (auto channelVolume : volume.volume) {
                        std::cout << "volume: " << channelVolume.vol << std::endl;
                    }
                } else {
                    std::cout << "Get Volume Failed." << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                        << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::setVolume(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            StreamVolume volume;
            AudioHelper::getUserVolumeInput(volume);
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->setVolume(volume);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Set Volume succeeded" << std::endl;
                } else {
                    std::cout << "Set Volume Failed" << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::getMute(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            StreamMute muteStatus;
            AudioHelper::getUserDirectionInput(muteStatus.dir);
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->getMute(muteStatus);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Mute Status is : " << muteStatus.enable << std::endl;
                } else {
                    std::cout << "Get Mute Failed" << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::setMute(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            StreamMute muteStatus;
            AudioHelper::getUserMuteStatusInput(muteStatus);
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->setMute(muteStatus);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    if (muteStatus.enable) {
                        std::cout << "Stream Muted" << std::endl;
                    } else {
                        std::cout << "Stream Unmuted" << std::endl;
                    }
                } else {
                    std::cout << "Mute Operation Failed" << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::startAudio(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                Status status = activeSession_->startAudio();
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Audio started on slotId : " << slotId_ << std::endl;
                } else if(status == Status::ALREADY) {
                    std::cout << "Audio already started on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Failed to start audio on slotId : " << slotId_ << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::stopAudio(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                Status status = activeSession_->stopAudio();
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Audio stopped on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Failed to stop audio on slotId : " << slotId_ << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::startDtmf(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            DtmfTone tone;
            tone.direction = StreamDirection::RX;
            uint32_t duration = 0;
            uint16_t gain = 0;
            auto status = AudioHelper::getUserDtmfInput(tone, duration, gain);
            if (status != Status::SUCCESS) {
                return;
            }
            mutex_.lock();
            if(activeSession_) {
                status = activeSession_->startDtmf(tone, duration, gain);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Dtmf Tone Started on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Start Dtmf Tone Failed on slotId : " << slotId_ << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::stopDtmf(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->stopDtmf();
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Dtmf Tone Stopped on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Stop Dtmf Tone Failed on slotId_" << slotId_ << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::registerListener(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->registerListener(shared_from_this());
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Voice listener registered" << std::endl;
                } else {
                    std::cout << "Listener registration failed" << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::deRegisterListener(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                auto status = activeSession_->deRegisterListener(shared_from_this());
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Voice listener deregistered" << std::endl;
                } else {
                    std::cout << "Listener deregistration failed" << std::endl;
                }
            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void VoiceMenu::changeSlotId() {
    // User can switch slots using this method. It assumes two slots are supported.
    std::cout << "Current Slot Id is " << slotId_ << std::endl;
    if (slotId_ == SLOT_ID_1) {
        slotId_ = SLOT_ID_2;
    } else if (slotId_ == SLOT_ID_2) {
        slotId_ = SLOT_ID_1;
    }
    std::cout << "After switch Slot Id is Changed to " << slotId_ << std::endl;
    setActiveSession(slotId_);
}

Status VoiceMenu::createActiveSession(SlotId slotId) {
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

void VoiceMenu::deleteActiveSession(SlotId slotId) {
    std::lock_guard<std::mutex> lk(mutex_);
    voiceSessions_.erase(slotId);
    activeSession_ = nullptr;
    std::cout << "Voice session deleted on slotId : " << slotId_ << std::endl;
}

Status VoiceMenu::setActiveSession(SlotId slotId) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (voiceSessions_.count(slotId) && (voiceSessions_[slotId])) {
        activeSession_ = voiceSessions_[slotId];
        return Status::SUCCESS;
    } else {
        activeSession_ = nullptr;
    }
    return Status::NOSUCH;
}

void VoiceMenu::onDtmfToneDetection(DtmfTone dtmfTone) {
    std::cout << "Dtmf Tone Detected" << std::endl;
    std::cout << "Direction is " << static_cast<uint32_t>(dtmfTone.direction) << std::endl;
    std::cout << "Low Frequency is " << static_cast<uint32_t>(dtmfTone.lowFreq) << std::endl;
    std::cout << "High Frequency is " << static_cast<uint32_t>(dtmfTone.highFreq) << std::endl;
}
