/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * @file    AntennaListener.hpp
 *
 * @brief   AntennaListener provides callback methods to listen for service status change
 *          notifications.
 *          The client needs to implement these methods. AntennaListener methods can be
 *          invoked from multiple threads, so the client needs to ensure that the
 *          implementation is thread-safe.
 */

#ifndef TELUX_PLATFORM_HARDWARE_ANTENNALISTENER_HPP
#define TELUX_PLATFORM_HARDWARE_ANTENNALISTENER_HPP

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace platform {

namespace hardware {
/** @addtogroup telematics_platform_hardware_antenna
 * @{ */

/**
 * @brief Listen class to get antenna configuration related notifications.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls. Class methods can be invoked from multiple threads, so the client needs to
 *        ensure that the implementation is thread-safe.
 */
class IAntennaListener : public common::IServiceStatusListener {
 public:

    /**
     * This function is called whenever any active cellular antenna is changed.
     *
     * On platforms with access control enabled, the caller needs to have
     * TELUX_PLATFORM_ANTENNA_MGMT permission to receive this notification.
     *
     * @param [in] antIndex        Indicates which antenna is now active.
     *
     */
    virtual void onActiveAntennaChange(int antIndex) {
    }

    /**
     * IAntennaListener destructor.
     */
    virtual ~IAntennaListener() {
    }
};

/** @} */ /* end_addtogroup telematics_platform_hardware_antenna */
}  // end of namespace hardware

}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_HARDWARE_ANTENNALISTENER_HPP
