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
 * @file    TimeListener.hpp
 *
 * @brief   TimeListener provides callback methods for listening to the time information.
 *          Client needs to implement these methods. The methods in listener can be invoked
 *          from multiple threads.So the client needs to make sure that the implementation
 *          is thread-safe.
 */

#ifndef TELUX_PLATFORM_TIMELISTENER_HPP
#define TELUX_PLATFORM_TIMELISTENER_HPP

#include <cstdint>
#include <telux/common/SDKListener.hpp>

namespace telux {

namespace platform {

/** @addtogroup telematics_platform_time
 * @{ */

/**
 * @brief Listener class for getting time information.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls in it. The methods in this class can be invoked from multiple different threads.
 *        Client needs to make sure that the implementation is thread-safe.
 */
class ITimeListener : public telux::common::ISDKListener {
 public:

    /**
     * This function is called every 100 milliseconds after registering a listener by
     * invoking @ref ITimeManager::registerListener.
     * The utc reported via this API is derived from location fix, utc value zero
     * means there is no valid utc derived from location fix.
     *
     * On platforms with Access control enabled, the client needs to have
     * TELUX_LOC_DATA permission for this API to be invoked.
     *
     * @param [out] utcInMs - Milliseconds since Jan 1, 1970.
     *
     */
    virtual void onGnssUtcTimeUpdate(const uint64_t utcInMs) {}

    /**
     * This function is called every second after registering a listener by
     * invoking @ref ITimeManager::registerListener.
     * In order for this API to be invoked, the vehicle needs to be in an
     * area of no GNSS coverage and select a roadside unit as the
     * synchronization reference, and a client (like an ITS stack) needs
     * to have injected a coarse UTC time using @ref
     * telux::cv2x::ICv2xRadioManager::injectCoarseUtcTime().
     *
     * @param [out] utcInMs - Milliseconds since Jan 1, 1970. 0 if no time
     *                        available via SLSS (Sidelink Synchronisation Signal).
     *
     */
    virtual void onCv2xUtcTimeUpdate(const uint64_t utcInMs) {}

    /**
     * Destructor of ITimeListener
     */
    virtual ~ITimeListener() {
    }
};

/** @} */ /* end_addtogroup telematics_platform_time */

}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_TIMELISTENER_HPP
