/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef ECALLOVERIMS_HPP
#define ECALLOVERIMS_HPP

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "ConsoleApp.hpp"
#include "ECallManager.hpp"

/**
 * EcallOverImsMenu class provides an user-interactive console to trigger an custom number eCall and
 * update MSD over IMS.
 */
class EcallOverImsMenu : public ConsoleApp {
 public:

    EcallOverImsMenu(std::shared_ptr<ECallManager> eCallManger, std::string appName,
                     std::string cursor);

    /**
     * Initialize the subsystems, console commands and display the menu.
     */
    void init();

    /**
     * Hangs up a triggered eCall and gracefully clears down the subsystems.
     */
    void cleanup();

    ~EcallOverImsMenu();
 private:
    /**
     * Trigger a Voice eCall to the specified phone number over IMS
     *
     */
    void makeCustomNumberECallOverIms();

    /**
     * Update MSD for custom number eCall over IMS
     *
     */
    void updateCustomNumberECallOverIms();

    /**
     * Function to get optional SIP request headers
     *
     */
    void getOptionalSIPHeader(std::string &contentType, std::string &acceptInfo);
    // Member variable to keep the eCall manager object alive until the application quits.
    std::weak_ptr<ECallManager> eCallMgr_;
};

#endif  // ECALLAPP_HPP
