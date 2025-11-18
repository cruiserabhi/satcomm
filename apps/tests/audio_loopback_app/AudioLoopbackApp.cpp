/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include<iostream>
#include<chrono>
#include<getopt.h>
#include <csignal>

#include "AudioLoopbackApp.hpp"

#define SAMPLE_RATE 48000

static std::mutex mutex;
static std::condition_variable cv;

AudioLoopbackApp::AudioLoopbackApp() {
    inputDevice_ = DeviceType::DEVICE_TYPE_MIC;
    outputDevice_ = DeviceType::DEVICE_TYPE_SPEAKER;
}

AudioLoopbackApp::~AudioLoopbackApp() {

}

void AudioLoopbackApp::changeInputDevice(int inputDevice) {
    std::cout << "Input Device is " << inputDevice << std::endl;
    inputDevice_ = static_cast<DeviceType>(inputDevice);
}

void AudioLoopbackApp::changeOutputDevice(int outputDevice) {
    std::cout << "Output Device is " << outputDevice << std::endl;
    outputDevice_ = static_cast<DeviceType>(outputDevice);
}

static void signalHandler( int signum ) {
    std::unique_lock<std::mutex> lock(mutex);
    std::cout << "Interrupt signal (" << signum << ") received.." << std::endl;
    cv.notify_all();
}

Status AudioLoopbackApp::init() {
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<ServiceStatus> prom;
    //  Get the AudioFactory and AudioManager instances.
    auto &audioFactory = telux::audio::AudioFactory::getInstance();
    audioManager_ = audioFactory.getAudioManager([&prom](telux::common::ServiceStatus status) {
        prom.set_value(status);
    });
    if (!audioManager_) {
        std::cout << "Failed to get AudioManager object" << std::endl;
        return Status::FAILED;
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
    } else {
        std::cout << "ERROR - Unable to initialize audio subsystem" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status AudioLoopbackApp::createLoopbackStream() {
    StreamConfig config;
    config.type = StreamType::LOOPBACK;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = SAMPLE_RATE;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    // here both channel selected, this can be selected according to requirement
    config.channelTypeMask = (ChannelType::LEFT | ChannelType::RIGHT);
    config.deviceTypes.emplace_back(outputDevice_);
    config.deviceTypes.emplace_back(inputDevice_);
    std::promise<bool> p;
    auto status = audioManager_->createStream(config,
        [&p,this](std::shared_ptr<IAudioStream> &audioStream, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                audioLoopbackStream_ = std::dynamic_pointer_cast<IAudioLoopbackStream>(audioStream);
                p.set_value(true);
            } else {
                p.set_value(false);
            }
        });
    if (status == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to create stream failed"  << std::endl;
        return Status::FAILED;
    }

    if (p.get_future().get()) {
        std::cout<< "Loopback Stream is Created" << std::endl;
    } else {
        std::cout<< "Loopback Stream Creation Failed !!" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status AudioLoopbackApp::startLoopback() {
    createLoopbackStream();
    if (audioLoopbackStream_) {
        std::promise<bool> p;
        Status status =
            audioLoopbackStream_->startLoopback( [&p,this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
            }
            });
        if (status == Status::SUCCESS){
            std::cout << "Request to start loopback sent" << std::endl;
        } else {
            std::cout << "Request to start loopback Failed" << std::endl;
            return Status::FAILED;
        }

        if (p.get_future().get()) {
            std::cout << "Audio loopback is Started" << std::endl;
            loopbackStarted_ = true;
        } else {
            std::cout << "Failed to start loopback" << std::endl;
            return Status::FAILED;
        }
    }
    return Status::SUCCESS;
}

Status AudioLoopbackApp::deleteLoopbackStream() {
    std::promise<bool> p;
    if (audioLoopbackStream_) {
        Status status = audioManager_-> deleteStream(audioLoopbackStream_,
            [&p,this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
            }});
        if (status == Status::SUCCESS) {
            std::cout << "request to delete stream sent" << std::endl;
        } else {
            std::cout << "Request to delete stream failed"  << std::endl;
            return Status::FAILED;
        }
        if (p.get_future().get()) {
            audioLoopbackStream_= nullptr;
            std::cout << "Audio Stream is Deleted" << std::endl;
        } else {
            std::cout << "Failed to delete stream" << std::endl;
            return Status::FAILED;
        }
    }
    return Status::SUCCESS;
}

Status AudioLoopbackApp::stopLoopback() {
    std::promise<bool> p;
    if (audioLoopbackStream_ && loopbackStarted_) {
        Status status = audioLoopbackStream_->stopLoopback(
            [&p,this](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
        });
        if (status == Status::SUCCESS){
            std::cout << "Request to stop loopback sent" << std::endl;
        } else {
            std::cout << "Request to stop loopback Failed" << std::endl;
            return Status::FAILED;
        }

        if (p.get_future().get()) {
            std::cout << "Audio loopback is Stopped" << std::endl;
        } else {
            std::cout << "Failed to stop loopback" << std::endl;
            return Status::FAILED;
        }
    }
    auto status = deleteLoopbackStream();
    return status;
}

void AudioLoopbackApp::printHelp() {
    std::cout << "             Audio Loopback App\n"
    << "-------------------------------------------------------------\n"
    << "-i <device>           set input device, '-i 257' for mic.\n"
    << "-o <device>           set output device '-o 1' for speaker \n"
    << "-h                    help\n" << std::endl;
}

Status AudioLoopbackApp::parseArgs(int argc, char **argv) {
    int c;
    static struct option long_options[] = {
        {"change input device",        required_argument, 0, 'i'},
        {"change output device",       required_argument, 0, 'o'},
        {"help",                       no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    c = getopt_long(argc, argv, "i:o:h", long_options, &option_index);

    // if no option is entered the loopback starts with default options
    do {
        switch (c) {
            case 'i':
                changeInputDevice(atoi(optarg));
                break;
            case 'o':
                changeOutputDevice(atoi(optarg));
                break;
            case 'h':
                printHelp();
                exit(0);
        }
        c = getopt_long(argc, argv, "i:o:h", long_options, &option_index);

    } while (c != -1);
    return Status::SUCCESS;
}

int main(int argc, char ** argv)
{
    signal(SIGINT, signalHandler);

    std::shared_ptr<AudioLoopbackApp> app;
    try {
        app = std::make_shared<AudioLoopbackApp>();
    } catch (std::bad_alloc & e) {
        std::cout << " Failed to instantiate audio loopback app " << std::endl;
        return 0;
    }

    app->parseArgs(argc, argv);

    app->init();

    app->startLoopback();

    std::cout <<  " Press CTRL+C to exit" << std::endl;
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock);

    app->stopLoopback();

    return 0;
}
