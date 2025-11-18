#!/bin/bash

#  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause-Clear

ETC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}")/../etc && pwd)

if [ -f /etc/os-release ];
then
    lsb_info=$(grep -oP '(?<=PRETTY_NAME=).+' /etc/os-release | tr -d '"')
elif [ -f /usr/lib/os-release ];
then
    lsb_info=$(grep -oP '(?<=PRETTY_NAME=).+' /etc/os-release | tr -d '"')
fi

compiler_version=$(g++ --version | head -1 | awk '{print $4}')
glibc_version=$(ldd --version | head -1 | awk '{print $5}')

echo "
{
    \"buildHost\" : {
        \"lsbInfo\" : \"$lsb_info\",
        \"compilerVersion\" : \"$compiler_version\",
        \"glibcVersion\" : \"$glibc_version\"
    }
}" > /$ETC_DIR/system_details.json