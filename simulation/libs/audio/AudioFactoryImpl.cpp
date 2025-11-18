/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "AudioFactoryImpl.hpp"

namespace telux {
namespace audio {

AudioFactoryImpl::AudioFactoryImpl() {
}

AudioFactoryImpl::~AudioFactoryImpl() {
}

AudioFactory::AudioFactory() {
}

AudioFactory::~AudioFactory() {
}

AudioFactory &AudioFactoryImpl::getInstance() {
    static AudioFactoryImpl instance;
    return instance;
}

AudioFactory &AudioFactory::getInstance() {
    return AudioFactoryImpl::getInstance();
}

/*
 * Gives an instance of the AudioManagerImpl to the application.
 */
std::shared_ptr<IAudioManager> AudioFactoryImpl::getAudioManager(
    telux::common::InitResponseCb initCallback) {

    std::function<std::shared_ptr<IAudioManager>(telux::common::InitResponseCb)> createAndInit
        = [](telux::common::InitResponseCb initCb) -> std::shared_ptr<IAudioManager> {
        try {
            std::shared_ptr<AudioManagerImpl> manager = std::make_shared<AudioManagerImpl>();
            if (manager->init(initCb) != telux::common::Status::SUCCESS) {
                return nullptr;
            }
            return manager;
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create AudioManagerImpl");
        }
        return nullptr;
    };

    std::string type = std::string("Audio manager");
    LOG(DEBUG, __FUNCTION__, " requesting ", type.c_str(), ", cb ", &initCompleteCallbacks_);

    auto mgrAudio = telux::common::FactoryHelper::getManager<IAudioManager>(
        type, audioMgr_, initCompleteCallbacks_, initCallback, createAndInit);

    return mgrAudio;
}

void AudioFactoryImpl::managerInitResult(telux::common::ServiceStatus status) {
    {
        std::lock_guard<std::mutex> initLock(managerInitGuard_);

        currentServiceStatus_ = status;
        serviceStatusReady_   = true;
        serviceStatusAvailable_.notify_all();
    }
}

/*
 * Provides AudioPlayerImpl instance to the application.
 */
telux::common::ErrorCode AudioFactoryImpl::getAudioPlayer(
    std::shared_ptr<IAudioPlayer> &audioPlayer) {

    bool waitResult = false;
    std::shared_ptr<IAudioPlayer> player;
    std::shared_ptr<IAudioManager> audioManager;

    auto initCb = std::bind(&AudioFactoryImpl::managerInitResult, this, std::placeholders::_1);

    {
        std::unique_lock<std::mutex> initLock(managerInitGuard_);

        serviceStatusReady_ = false;
        audioManager        = getAudioManager(initCb);
        if (!audioManager) {
            LOG(ERROR, __FUNCTION__, " can't get IAudioManager");
            return telux::common::ErrorCode::NO_MEMORY;
        }

        /*
         * If the audio service doesn't become available within INIT_WAIT_TIME
         * seconds, timeout and let the application retry.
         */
        waitResult = serviceStatusAvailable_.wait_for(
            initLock, std::chrono::seconds(INIT_WAIT_TIME), [=] { return serviceStatusReady_; });

        if ((!waitResult)
            || (currentServiceStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
            LOG(ERROR, __FUNCTION__, " audio service timedout/unavailable");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }
    }

    try {
        player      = std::make_shared<AudioPlayerImpl>(audioManager);
        audioPlayer = player;
    } catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioPlayerImpl");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    return telux::common::ErrorCode::SUCCESS;
}

}  // end namespace audio
}  // end namespace telux
