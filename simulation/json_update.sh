#!/bin/bash

#  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause-Clear

BIN_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
source /$BIN_DIR/setup_simulation.sh

telsdk_event_injector -f json_update -e modify $@