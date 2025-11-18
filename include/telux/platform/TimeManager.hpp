/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       TimeManager.hpp
 * @brief      TimeManager provides APIs to register and deregister
 *             a listener for time reports.
 */

#ifndef TELUX_PLATFORM_TIMEMANAGER_HPP
#define TELUX_PLATFORM_TIMEMANAGER_HPP

#include <memory>
#include <bitset>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/TimeListener.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform_time
 * @{ */

/**
 * Defines supported utc report types.
 */
enum SupportedTimeType {
    GNSS_UTC_TIME = 0,    /**< GNSS UTC time derived from location fix. */
    CV2X_UTC_TIME  = 1,   /**< UTC time derived from injected UTC when the
                               vehicle has selected a roadside unit as the
                               synchronization reference for V2X communication. */
    MAX_SUPPORTED_TIME_TYPES
};

/**
 * Bit mask that denotes which of the time types defined in
 * @ref SupportedTimeType are supported.
 */
using TimeTypeMask = std::bitset<MAX_SUPPORTED_TIME_TYPES>;

/**
 * @brief   ITimeManager provides interface to retrieve time information.
 */
class ITimeManager {
 public:
    /**
     * This status indicates whether the object is in a usable state.
     *
     * @returns @ref telux::common::ServiceStatus indicating the current status of the device info
     *          service.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener for time updates.
     * This will result in frequent notifications and will result in
     * wakeups when system is suspended. If wakeups are not desired
     * then deregister should be called.
     *
     * @param [in] listener      - pointer to implemented listener.
     * @param [in] mask          - mask to indicate which times the client is
     *                             interested in registering for.
     *
     * @returns status of the registration request.
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ITimeListener> listener,
        TimeTypeMask mask) = 0;

    /**
     * Deregisters the previously registered listener for time updates.
     *
     * @param [in] listener      - pointer to registered listener that needs to be removed.
     * @param [in] mask          - mask to indicate which times the client has registering for.
     *
     * @returns status of the deregistration request.
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ITimeListener> listener,
        TimeTypeMask mask) = 0;

    /**
     * Destructor of ITimeManager
     */
    virtual ~ITimeManager(){};
};

/** @} */ /* end_addtogroup telematics_platform_time */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_TIMEMANAGER_HPP
