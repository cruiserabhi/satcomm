/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "v2x_log.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <syslog.h>

int v2x_use_syslog = 0;
int v2x_log_level  = LOG_ERR;

void v2x_log_level_set(int x) {
    v2x_log_level = x;
    LOGI("V2X Debug log level set to %d %s\n", x, v2x_log_prio_name(x));
}

void v2x_log_to_syslog(int newval) {
    if (newval) {
        LOGI("V2X Debug logging swithed to syslog, read with logread command\n");
    } else {
        LOGI("V2X Debug set to use stdout.\n");
    }
    v2x_use_syslog = newval;
}
