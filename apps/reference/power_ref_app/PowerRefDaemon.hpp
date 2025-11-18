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

#ifndef POWERREFDAEMON_HPP
#define POWERREFDAEMON_HPP

#include <grp.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <ostream>
#include <telux/common/CommonDefines.hpp>
#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityListener.hpp>
#include <telux/common/Log.hpp>
#include "common/define.hpp"
#include "Event.hpp"
#include "EventManager.hpp"
#include "trigger/NAOIP/NAOIpTrigger.hpp"
#include "trigger/SMS/SMSTrigger.hpp"
#include "../../common/utils/Utils.hpp"

#ifdef CAN_TRIGGER_SUPPORTED
#include "trigger/CAN/CANTrigger.hpp"
#endif // CAN_TRIGGER_SUPPORTED


using namespace telux::power;
using namespace telux::common;
/**
 * @brief PowerRefDaemon class initialise all triggers, EventManager instance and handles signal
 *
 */

class PowerRefDaemon {
public:
   telux::common::Status init();
   int startDaemon(int argc, char **argv);
   void stopDaemon();
   void registerForUpdates();

   static PowerRefDaemon &getInstance();

private:
   std::mutex mtx_;
   std::condition_variable cv_;
   bool exiting_ = false;
   ConfigParser* config_;
   shared_ptr<EventManager> eventManager_;
   shared_ptr<NAOIpTrigger> naoIpTrigger_;
   shared_ptr<SMSTrigger> smsTrigger_;

   #ifdef CAN_TRIGGER_SUPPORTED
   shared_ptr<CANTrigger> canTrigger_;
   #endif // CAN_TRIGGER_SUPPORTED

   static void signalHandler(int signum);
   void printUsage(char **argv);
   telux::common::Status parseArguments(int argc, char **argv);
};

#endif