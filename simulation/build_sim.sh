#Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

#!/bin/bash
# Additional checks to see if we are in the right directory to be done
ROOTFS=$2
export PATH=${PWD}/build/bin:${PATH}

if [ "$ROOTFS" = "" ] ; then
   ROOTFS=$PWD/rootfs
fi

if [ ! -d "$ROOTFS" ] ; then
    mkdir -p "$ROOTFS"
fi

if [ "$UBUNTU_VERSION" = "" ] ; then
    if [ -f /etc/os-release ];
    then
        lsb_info=$(grep -oP '(?<=PRETTY_NAME=).+' /etc/os-release | tr -d '"')
    elif [ -f /usr/lib/os-release ];
    then
        lsb_info=$(grep -oP '(?<=PRETTY_NAME=).+' /etc/os-release | tr -d '"')
    fi

    export UBUNTU_VERSION=$(echo $lsb_info | grep -oP '\d{2}\.\d{2}' | tr -d '.')
fi

SUPPORTED_UBUNTU_VERSION="1804 2004 2204"

if ! echo "$SUPPORTED_UBUNTU_VERSION" | grep -q -w "$UBUNTU_VERSION"; then
    echo "Unsupported Ubuntu version."
    exit 1
fi

export ROOTFS

make $1
