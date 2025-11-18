
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
#ifndef POWERREF_DEFINES_HPP
#define POWERREF_DEFINES_HPP

#include <telux/data/DataDefines.hpp>

#include <thread>
#include <iostream>

#define DAEMON_NAME           "telux_power_refd"
#define WAKE_LOCK             "temp_telux_power_refd"

/** concerned node with regard to power state */
#define WAKELOCK_PATH         "/sys/power/wake_lock"
#define WAKEUNLOCK_PATH       "/sys/power/wake_unlock"
#define AUTOSLEEP_NODE_PATH   "/sys/power/autosleep"

#define AUTOSLEEP_NODE_OFF    "off"
#define AUTOSLEEP_NODE_MEM    "mem"

#define TRIGGER_SUSPEND       "TRIGGER_SUSPEND"
#define TRIGGER_RESUME        "TRIGGER_RESUME"
#define TRIGGER_SHUTDOWN      "TRIGGER_SHUTDOWN"

#define MACHINE_NAME_DELIMINATOR            ':'

/**
 * Represents the inputs/triggers that the daemon listens to, for initiating TcuActivity state
 * transition
 */
enum TriggerType
{
  NAOIP_TRIGGER = 1,
  SMS_TRIGGER,
  GPIO_TRIGGER,
  CAN_TRIGGER,
  UNKNOWN
};

enum EventStatus
{
  INITIALIZED = 1,                    /** trigger is initialized */
  IN_QUEUE,                           /** trigger is added to the queue */
  IN_PROGRESS_TCU_ACTIVITY,           /** trigger passed to TCU activity manager */
  REJECTED_INVALID_STATE_TRANSITION,  /** trigger rejected due to invalid state transition */
  REJECTED_INVALID_MACHINE_NAME,      /** trigger rejected due to invalid machine name */
  REJECTED_EVENT_OVERRIDDEN,          /** trigger is overridden */
  FAILED_TCU_ACTIVITY,                /** TCU activity state transition failed */
  FAILED_TCU_ACTIVITY_TIMEOUT,        /** TCU activity state transition failed due to timeout */
  SUCCEED                             /** TCU activity state transition succeeded */
};

#endif
