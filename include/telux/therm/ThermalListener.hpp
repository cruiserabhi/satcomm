/*
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file       ThermalListener.hpp
 *
 * @brief      IThermalListener - Interface for Thermal listener object. the clients needs
 *             to implement this interface to get access to thermal service notifications
 *             like onServiceStatusChange.
 *             The methods in listener can be invoked from multiple threads.So the client
 *             needs to make sure that the implementation is thread-safe.
 */

#ifndef TELUX_THERM_THERMALLISTENER_HPP
#define TELUX_THERM_THERMALLISTENER_HPP

#include <memory>
#include <vector>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace therm {

/** @addtogroup telematics_therm_management
 * @{ */

class ITripPoint;
class ICoolingDevice;
enum class TripEvent;

/**
 * @brief Listener class for getting notifications when thermal service status changes.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls in it. The methods in this class can be invoked from multiple different threads.
 *        Client needs to make sure that the implementation is thread-safe.
 */
class IThermalListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * Destructor of IThermalListener
     */
    virtual ~IThermalListener() {
    }

    /**
     * This function is called at the time of cooling device level update.
     * On platforms with Access control enabled, the client needs to have
     * TELUX_THERM_DATA_READ permission to receive this event.
     *
     * @param [in] coolingDevice - vector of cooling device for which the level has been
     *                             updated.
     */
    virtual void onCoolingDeviceLevelChange(std::shared_ptr<ICoolingDevice> coolingDevice) {
    }

    /**
     * This function is called at the time of trip event occurs.
     * On platforms with Access control enabled, the client needs to have
     * TELUX_THERM_DATA_READ permission to receive this event.
     *
     * @param [in] tripInfo  - Vector of the trip point for which trip event has been occured.
     * @param [in] tripEvent - Indicates trip event.
     *                       - NONE
     *                       - CROSSED_UNDER
     *                       - CROSSED_OVER
     */
    virtual void onTripEvent(std::shared_ptr<ITripPoint> tripPoint, TripEvent tripEvent) {
    }
};

/** @} */ /* end_addtogroup telematics_therm_management */

}  // end of namespace therm
}  // end of namespace telux

#endif // TELUX_THERM_THERMALLISTENER_HPP
