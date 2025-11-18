#Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

LIB_DIR=$(cd $(dirname "${BASH_SOURCE[0]}")/../lib && pwd)
BIN_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
ROOT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}")/../ && pwd)

export DEFAULT_SIM_FILE_PATH=$ROOT_DIR
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIB_DIR/
export PATH=$BIN_DIR/:$PATH
export SIMSYSROOT=$ROOT_DIR
export PKG_CONFIG_SYSROOT_DIR=$SIMSYSROOT
export PKG_CONFIG_PATH=$SIMSYSROOT/lib/pkgconfig:$SIMSYSROOT/share/pkgconfig
export CC="gcc -I $SIMSYSROOT/include/ -L $SIMSYSROOT/lib -fstack-protector-strong -pie -fPIE -D_FORTIFY_SOURCE=2 -Wa,--noexecstack -Wformat -Wformat-security -Werror=format-security"
export CXX="g++ -I $SIMSYSROOT/include/ -L $SIMSYSROOT/lib -fstack-protector-strong -pie -fPIE -D_FORTIFY_SOURCE=2 -Wa,--noexecstack -Wformat -Wformat-security -Werror=format-security"
export CPP="gcc -I $SIMSYSROOT/include/ -L $SIMSYSROOT/lib -E -fstack-protector-strong -pie -fPIE -D_FORTIFY_SOURCE=2 -Wa,--noexecstack -Wformat -Wformat-security -Werror=format-security"
export LD="ld -L $SIMSYSROOT/lib"
export CFLAGS=" -O2 -Wa,--noexecstack -fexpensive-optimizations -frename-registers -ftree-vectorize -finline-functions -finline-limit=64 -Wno-error=maybe-uninitialized -Wno-error=unused-result"
export CXXFLAGS=" -O2 -Wa,--noexecstack -fexpensive-optimizations -frename-registers -ftree-vectorize -finline-functions -finline-limit=64 -Wno-error=maybe-uninitialized -Wno-error=unused-result"
export LDFLAGS="-Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,-z,relro,-z,now,-z,noexecstack"
export ARCH=x86_64