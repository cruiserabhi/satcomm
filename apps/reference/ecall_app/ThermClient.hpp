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

#ifndef THERMCLIENT_HPP
#define THERMCLIENT_HPP

#include <telux/therm/ThermalFactory.hpp>
#include <telux/therm/ThermalShutdownManager.hpp>

using namespace telux::common;
using namespace telux::therm;

class ThermClient : public telux::therm::IThermalShutdownListener,
                         public std::enable_shared_from_this<ThermClient> {
public:
    /**
     * Initialize Thermal subsystem
     */
    telux::common::Status init();

    /**
     * This function disables automatic thermal shutdown. From this point of time, the auto-shutdown
     * will be disabled until it is enabled explicitly using enableAutoShutdown().
     * This is typically invoked when an eCall is triggered.
     *
     * @returns Status of disableAutoShutdown i.e success or suitable status code.
     *
     */
    telux::common::Status disableAutoShutdown();

    /**
     * This function enables automatic thermal shutdown.
     * This is typically invoked when an eCall is cleared down.
     *
     * @returns Status of enableAutoShutdown i.e success or suitable status code.
     *
     */
    telux::common::Status enableAutoShutdown();

    void onShutdownEnabled() override;
    void onShutdownDisabled() override;
    void onImminentShutdownEnablement(uint32_t imminentDuration) override;
    void onServiceStatusChange(ServiceStatus status) override;

    ThermClient();
    ~ThermClient();

private:
    bool isShutdownAllowed();
    void setShutdownAllowedState(bool state);
    telux::common::Status sendAutoShutdownModeCommand(AutoShutdownMode state);
    telux::common::Status registerForUpdates();
    telux::common::Status deregisterForUpdates();

    /** Represents whether auto-shutdown is allowed at any instant */
    bool shutdownAllowed_;
    /** Member variable to hold Thermal manager object */
    std::shared_ptr<telux::therm::IThermalShutdownManager> thermShutdownMgr_;
    std::mutex mutex_;

};

#endif  // THERMCLIENT_HPP
