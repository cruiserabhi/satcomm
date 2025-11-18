/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef IMSSERVINGSYSTEMMENU_HPP
#define IMSSERVINGSYSTEMMENU_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/tel/ImsServingSystemManager.hpp>
#include "console_app_framework/ConsoleApp.hpp"
#include "MyImsServSysListener.hpp"

#define DEFAULT_NUM_SLOTS 1
#define MULTI_SIM_NUM_SLOTS 2

class ImsServingSystemMenu : public ConsoleApp {
public:
    /**
     * Initialize commands and SDK
     */
    bool init();

    ImsServingSystemMenu(std::string appName, std::string cursor);
    ~ImsServingSystemMenu();

    void requestImsRegStatus(std::vector<std::string> userInput);
    void requestServiceStatusOverIms(std::vector<std::string> userInput);
    void requestPdpStatusOverIms(std::vector<std::string> userInput);

private:
    // Member variable to keep the Listener object alive till application ends.
    std::map<SlotId, std::shared_ptr<telux::tel::IImsServingSystemListener>> imsServSysListeners_;
    std::map<SlotId, std::shared_ptr<telux::tel::IImsServingSystemManager>> imsServingSystemMgrs_;
    int slotCount_ = DEFAULT_NUM_SLOTS;
};

#endif  // IMSSERVINGSYSTEMMENU_HPP
