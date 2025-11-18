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
#ifndef AUDIOHELPER_HPP
#define AUDIOHELPER_HPP

#include <telux/audio/AudioManager.hpp>
#include <telux/common/CommonDefines.hpp>

using namespace telux::audio;
using namespace telux::common;

enum Freq {
    Freq_697 = 697,
    Freq_770 = 770,
    Freq_852 = 852,
    Freq_941 = 941,
    Freq_1209 = 1209,
    Freq_1336 = 1336,
    Freq_1477 = 1477,
    Freq_1633 = 1633,
};

/** AudioHelper class provides methods to get user inputs */
class AudioHelper {
public:
    static void getUserSlotIdInput(SlotId &slotId);
    static void getAudioFormatInput(AudioFormat &audioFormat);
    static void getUserSampleRateInput(uint32_t &sampleRate);
    static void getUserChannelInput(ChannelTypeMask &channels);
    static void getUserDeviceInput(std::vector<DeviceType> &devices);
    static void getUserCreateStreamInput(StreamConfig &config);
    static void getUserDirectionInput(StreamDirection &direction);
    static void getUserVolumeInput(StreamVolume &streamVolume);
    static void getVolumeValueInput(float &vol);
    static void getUserVoicePathInput(std::vector<Direction> &direction);
    static void getUserMuteStatusInput(StreamMute &mute);
    static Status getUserDtmfInput(DtmfTone &tone, uint32_t &duration, uint16_t &gain);
    static void getUserEcnrModeInput(EcnrMode &ecnrMode);

private:
    static Status lowFrequencyHelper(uint32_t lowFreq, DtmfLowFreq &lowFrequency);
    static Status highFrequencyHelper(uint32_t highFreq, DtmfHighFreq &highFrequency);
};

#endif  // AUDIOHELPER_HPP
