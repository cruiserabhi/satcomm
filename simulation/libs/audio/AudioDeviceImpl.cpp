/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "AudioDeviceImpl.hpp"

namespace telux {
namespace audio {

AudioDeviceImpl::AudioDeviceImpl(DeviceType deviceType,
        DeviceDirection deviceDirection) {
    deviceType_ = deviceType;
    deviceDirection_ = deviceDirection;
}

AudioDeviceImpl::~AudioDeviceImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Gives type of the audio device like mic or speaker etc.
 */
DeviceType AudioDeviceImpl::getType() {
    return deviceType_;
}

/*
 * Gives direction of the audio device like TX/RX.
 */
DeviceDirection AudioDeviceImpl::getDirection() {
    return deviceDirection_;
}

}  // end of namespace audio
}  // end of namespace telux
