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

#ifndef CANTRIGGER_HPP
#define CANTRIGGER_HPP

#include "../../Event.hpp"
#include "../../EventManager.hpp"
#include "../../IEventListener.hpp"
#include "../../common/ConfigParser.hpp"

#include <CwBase.h>
#include <CanWrapper.h>
#include <CwFrame.h>

typedef int RegistrationToken;

class CANTrigger :  public IEventListener ,
                    public enable_shared_from_this<CANTrigger>
{
private:

    static std::shared_ptr<CANTrigger> canTrigger_;
    static void triggerEvent(CwFrame * pf, void* userData, int ifNo);

    CANTrigger(std::shared_ptr<EventManager> eventManager);
    std::shared_ptr<EventManager> eventManager_;

    /** map to store can frame id along with respective expected TcuActivityState
     * and registration token*/
    std::map<uint32_t, pair<TcuActivityState, RegistrationToken>> triggers_;

    ConfigParser * config_;     /** config parser to fetch data from config file */
    CanWrapper * canWrapper_;

    bool loadTrigger();
    bool registerCanListener();
    void deRegisterCanListener();
    bool loadTriggers();

public:
    static std::shared_ptr<CANTrigger> getInstance(std::shared_ptr<EventManager> eventManager);
    bool init();

    ~CANTrigger();

    //EventListener
    void onEventRejected(shared_ptr<Event> event,EventStatus reason) override;
    void onEventProcessed(shared_ptr<Event> event,bool success) override;
};

#endif //CANTRIGGER_HPP