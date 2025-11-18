/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIODEVICEIMPL_HPP
#define AUDIODEVICEIMPL_HPP

#include <telux/audio/AudioManager.hpp>

namespace telux {
namespace audio {

/*
 * Represents an audio device.
 *
 * Audio clients (applications) uses IAudioDevice.*() APIs to set and get
 * specific details about this device.
 */
class AudioDeviceImpl : public IAudioDevice,
                        public std::enable_shared_from_this<AudioDeviceImpl> {

 public:
    AudioDeviceImpl(DeviceType deviceType, DeviceDirection deviceDirection);

    ~AudioDeviceImpl();

    DeviceType getType(void) override;

    DeviceDirection getDirection(void) override;

 private:
    DeviceType deviceType_ = DeviceType::DEVICE_TYPE_NONE;
    DeviceDirection deviceDirection_ = DeviceDirection::NONE;
};

}  // end of namespace audio
}  // end of namespace telux

#endif // AUDIODEVICEIMPL_HPP
