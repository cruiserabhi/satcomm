#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

# Download base image ubuntu 20.04
FROM ubuntu:20.04

# LABEL about the custom image
LABEL description="This is custom docker image for SDK simulation users"

RUN export DEBIAN_FRONTEND=noninteractive

RUN apt-get -qq update
RUN apt-get -y install sudo
RUN apt-get update --fix-missing

RUN apt-get -y install build-essential
RUN apt-get -y install nano
RUN apt-get -y install net-tools
RUN apt-get -y install python3
RUN apt-get -y install python
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install gdb
RUN apt-get -y install iproute2
RUN apt-get -y install python3-pip
RUN apt-get -y install git
RUN apt-get -y install wget
RUN apt-get -y install libcap-dev
RUN apt-get -y install pkg-config
RUN pip install supervisor
RUN pip install supervisor-stdout
RUN apt-get -y install libasound2-dev
RUN apt-get -y install alsa alsa-utils alsa-tools
RUN apt-get -y install pulseaudio pulseaudio-utils
RUN apt-get -y install libglib2.0-dev