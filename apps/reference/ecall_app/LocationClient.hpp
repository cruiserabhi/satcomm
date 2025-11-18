/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#ifndef LOCCLIENT_HPP
#define LOCCLIENT_HPP

#include <telux/loc/LocationFactory.hpp>
#include <telux/loc/LocationManager.hpp>

using namespace telux::common;
using namespace telux::loc;

/** Structure to store necessary location information required for MSD */
struct ECallLocationInfo {
    int32_t latitude;
    int32_t longitude;
    uint8_t direction;  /**< Direction of travel in 2 degrees steps from magnetic north */
    uint32_t timestamp; /**< Seconds elapsed since midnight 01.01.1970 UTC */
};

/** Listener class that provides location updates */
class LocationListener {
public:
    /**
     * This function is called when a new location fix is available
     *
     * @param [in] locInfo Location information like latitude, longitude and Timestamp
     *
     */
    virtual void onLocationUpdate(ECallLocationInfo locInfo) {
    }

    /**
     * Destructor of LocationListener
     */
    virtual ~LocationListener() {
    }
};

/**
 * LocationClient provides methods to start and stop receiving location updates
 */
class LocationClient : public ILocationListener,
                       public std::enable_shared_from_this<LocationClient> {
public:
    /**
     * Initialize location subsystem
     */
    telux::common::Status init();

    /**
     * This function starts the location updates.
     * This is typically invoked when an eCall is triggered.
     *
     * @param [in] interval        time interval used for periodic location updates
     * @param [in] locListener     shared pointer to LocationListener object
     *
     * @returns Status of startLocUpdates i.e success or suitable status code.
     *
     */
    telux::common::Status startLocUpdates(uint32_t interval,
                                                    std::shared_ptr<LocationListener> locListener);

    /**
     * This function stops the location updates.
     * This is typically invoked when an eCall ends.
     *
     * @returns Status of stopLocUpdates i.e success or suitable status code.
     *
     */
    telux::common::Status stopLocUpdates();

    void onBasicLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo)
                    override;
    void commandCallback(ErrorCode errorCode);

    LocationClient();
    ~LocationClient();

private:
    /** Member variable to hold Location manager object */
    std::shared_ptr<ILocationManager> locMgr_;
    /** Member variable to hold LocationListener object, to which the location fixes are passed */
    std::shared_ptr<LocationListener> locListener_;
};

#endif  // LOCCLIENT_HPP
