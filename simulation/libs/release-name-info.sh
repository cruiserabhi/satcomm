#!/bin/bash

#  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause-Clear

headTag=$(git -C $1 name-rev --name-only --tags HEAD)

 if [ "$headTag" = "undefined" ];then
         latestTag=$(git -C $1 describe --tags --abbrev=0)
         headCommitHash=$(git -C $1 rev-parse --short HEAD)
         echo "$latestTag-$headCommitHash"
 else
         echo $headTag
 fi
