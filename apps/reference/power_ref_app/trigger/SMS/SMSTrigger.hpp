/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef SMSTRIGGER_HPP
#define SMSTRIGGER_HPP

#include <memory>
#include <string>
#include <vector>
#include "telux/tel/SmsManager.hpp"

#include "../../Event.hpp"
#include "../../EventManager.hpp"
#include "../../IEventListener.hpp"
#include "../../common/ConfigParser.hpp"

class SMSTrigger :  public telux::tel::ISmsListener,
                    public IEventListener ,
                    public enable_shared_from_this<SMSTrigger> {
private:
    std::map<string, TcuActivityState> triggerText_;        /** map which stores trigger text with
                                                                respect to TcuActivityState */
    ConfigParser * config_;         /** config parser to fetch data from config file */
    std::shared_ptr<EventManager> eventManager_;            /** event management */
    std::shared_ptr<telux::tel::ISmsManager> smsManager_;

    bool loadConfig();
    void triggerEvent(TcuActivityState event, std::string machineName);
    bool validateTrigger(std::string text, TcuActivityState& tcuActivityState,
        std::string& machineName);

public:
    SMSTrigger(std::shared_ptr<EventManager> eventManager);
    ~SMSTrigger();
    bool init();

    //SmsListener
    void onIncomingSms(int phoneId, std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs)
        override;

    //EventListener
    void onEventRejected(shared_ptr<Event> event,EventStatus reason) override;
    void onEventProcessed(shared_ptr<Event> event,bool success) override;
};

#endif //SMSTRIGGER_HPP