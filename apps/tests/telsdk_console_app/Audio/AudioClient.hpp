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
 *
 *  Copyright (c) 2023,2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOCLIENT_HPP
#define AUDIOCLIENT_HPP

#include<map>
#include <memory>

#include <telux/audio/AudioManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "Audio/VoiceSession.hpp"
#include "Audio/AudioHelper.hpp"
#include "ConfigParser.hpp"

using namespace telux::common;
using namespace telux::audio;

/** AudioClient class provides methods to start and stop a voice session */
class AudioClient: public telux::audio::IAudioListener,
                   public std::enable_shared_from_this<AudioClient> {
public:
    static std::shared_ptr<AudioClient> getInstance();

    /**
     * Initialize audio subsystem
     */
    Status init();

    /**
     * @returns status of the Audio subsystem.
     */
    bool isReady();

    /**
     * This function setups the audio path for voice call.
     *
     * @param [in] slotId    Represents slotId for the voice call.
     */
    void startVoiceSession(SlotId slotId);

    /**
     * This function helps in tearing down the audio path for voice call.
     *
     * @param [in] slotId    Represents slotId for the voice call.
     */
    void stopVoiceSession(SlotId slotId);

    /**
     * This function helps in muting the audio path for voice call for both RX and TX path.
     *
     * @param [in] slotId    Represents slotId for the voice call.
     * @param [in] status    mute status.
     *
     */
    void setMuteStatus(SlotId slotId, bool muteStatus);

    ~AudioClient();

    void onServiceStatusChange(telux::common::ServiceStatus status) override;

    void cleanup();
    void setSystemReady();

private:
    AudioClient();
    /** Member variables to hold Audio Manager and voice stream objects */
    void setActiveSession(SlotId slotId);
    void loadConfFileData();
    void queryInputType();
    void muteStream(SlotId slotId);
    void unmuteStream(SlotId slotId);

    std::shared_ptr<IAudioManager> audioMgr_;
    std::mutex mutex_;
    std::shared_ptr<VoiceSession> activeSession_;
    std::map<SlotId, std::shared_ptr<VoiceSession>> voiceSessions_;
    StreamConfig config_;
    std::atomic<bool> ready_;
    bool hasConcurrentVoiceCall_{false};
    SlotId currentSlotId_{INVALID_SLOT_ID};
    SlotId previousSlotId_{INVALID_SLOT_ID};
    bool audioStartedOnSim1_{false};
    bool audioStartedOnSim2_{false};
};

#endif  // AUDIOCLIENT_HPP
