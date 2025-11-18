#!/bin/bash

#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

# This script is designed to check if individual applications are compilable from their respective CMake files.
# Additionally, it maintains a list of applications that it will ignore if their compilation fails.

cwd=$PWD

# list of failures to be ignored
IGNORE_DIR_LIST=("apps/reference/rits/tests" "apps/reference/rits/tests/applicationTest" "apps/reference/rits/tests/qMonitorTest" "apps/reference/rits/tests/qimcTest" "apps/reference" "apps/samples" "apps/samples/cv2x")

build_app() {
  dir=$1
  found=false
  for dir_to_check in "${IGNORE_DIR_LIST[@]}"; do
    if echo "$dir" | grep -qx "$dir_to_check"; then
        found=true
        break
    fi
  done

  if $found; then
    echo "===$dir: Ignored"
  else
    mkdir -p build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=./install -DCMAKE_INSTALL_SYSCONFDIR=$SDKTARGETSYSROOT/etc/ -DBUILD_ALL_SAMPLES=ON 2>&1 > /dev/null
    if [ $? != "0" ]
    then
      echo "===$dir: cmake failed"
      return
    fi
    make -j 64 2>&1 > /dev/null
    if [ $? != "0" ]
    then
      echo "===$dir: make failed"
      return
    fi
    make install 2>&1 > /dev/null
    if [ $? != "0" ]
    then
      echo "===$dir: make install failed"
      return
    fi
    echo "===$dir: ok"
  fi
}

build_all_apps() {
  basedir=$1

  dirs=$(find $basedir -name CMakeLists.txt | xargs dirname)
  for dir in $dirs
  do
    pushd $dir > /dev/null
    build_app $dir | grep "^==="
    popd > /dev/null
    find $cwd -name build | xargs rm -rf
  done
}

build_all_apps apps


