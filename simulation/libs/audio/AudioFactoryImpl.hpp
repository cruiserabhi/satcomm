/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOFACTORYIMPL_HPP
#define AUDIOFACTORYIMPL_HPP

#include <memory>

#include <telux/audio/AudioFactory.hpp>

#include "common/FactoryHelper.hpp"

#include "AudioPlayerImpl.hpp"
#include "AudioManagerImpl.hpp"

namespace telux {
namespace audio {

class AudioFactoryImpl : public AudioFactory, public telux::common::FactoryHelper {

 public:
    static AudioFactory &getInstance();

    std::shared_ptr<IAudioManager> getAudioManager(
        telux::common::InitResponseCb callback = nullptr) override;

    telux::common::ErrorCode getAudioPlayer(std::shared_ptr<IAudioPlayer> &audioPlayer) override;

    void managerInitResult(telux::common::ServiceStatus status);

 private:
    /* Time to wait in seconds for the audio service availability */
    const int INIT_WAIT_TIME = 10;

    bool serviceStatusReady_;
    std::mutex audioFactoryGuard_;
    std::mutex managerInitGuard_;
    telux::common::ServiceStatus currentServiceStatus_;
    std::condition_variable serviceStatusAvailable_;
    std::vector<telux::common::InitResponseCb> initCompleteCallbacks_;
    std::weak_ptr<IAudioManager> audioMgr_;

    AudioFactoryImpl();
    ~AudioFactoryImpl();
};

}  // End of namespace audio
}  // End of namespace telux

#endif  // AUDIOFACTORYIMPL_HPP
