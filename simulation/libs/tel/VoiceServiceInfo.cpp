/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <telux/tel/VoiceServiceInfo.hpp>

namespace telux {
namespace tel {

VoiceServiceInfo::VoiceServiceInfo(VoiceServiceState voiceServiceState,
   VoiceServiceDenialCause denialCause, RadioTechnology radioTech)
   : voiceServiceState_(voiceServiceState)
   , denialCause_(denialCause)
   , radioTech_(radioTech) {
}

VoiceServiceState VoiceServiceInfo::getVoiceServiceState() {
   return voiceServiceState_;
}

VoiceServiceDenialCause VoiceServiceInfo::getVoiceServiceDenialCause() {
   return denialCause_;
}

bool VoiceServiceInfo::isEmergency() {
   return (
      (voiceServiceState_ == VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING)
      || (voiceServiceState_ == VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING)
      || (voiceServiceState_ == VoiceServiceState::REG_DENIED_AND_EMERGENCY_AVAILABLE)
      || (voiceServiceState_ == VoiceServiceState::UNKNOWN_AND_EMERGENCY_AVAILABLE));
}

bool VoiceServiceInfo::isInService() {
   return (voiceServiceState_ == VoiceServiceState::REG_HOME
           || voiceServiceState_ == VoiceServiceState::REG_ROAMING);
}

bool VoiceServiceInfo::isOutOfService() {
   return (voiceServiceState_ == VoiceServiceState::UNKNOWN
           || voiceServiceState_ == VoiceServiceState::NOT_REG_AND_NOT_SEARCHING
           || voiceServiceState_ == VoiceServiceState::REG_DENIED
           || voiceServiceState_ == VoiceServiceState::NOT_REG_AND_SEARCHING);
}

RadioTechnology VoiceServiceInfo::getRadioTechnology() {
   return radioTech_;
}
}
}