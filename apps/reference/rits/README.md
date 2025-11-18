# RITS
Reference ITS Stack

1. Overview:

1.1 What?

RITS is an open source V2X software stack implementation, and runs on top of C-V2X radio(PC5 mode).
It is built on top of snaptel SDK, which provides radio Rx/Tx, Gnss location, and other
functionalities. A limited set of messages are implemented by RITS, which covers both SAE and
ETSI standards. It also provides interface to integrate with 3rd party security library.

1.2 Why?

RITS serves as an example of how to integrate snaptel SDK with C-V2X functionalities, allow users to
quickly integrate SW and HW for C-V2X project evaluation purpose and help user to understand the SW
stack architecture.

2. Build and Run:

2.1 Dependency:
   The Rerence ITS stack depends on following components.
   1. telematics SDK.
   2. Security Library.
   3. asn1c compiler (skeleton file only).

The following section desribes how to obtain/install dependencies.

2.1.1.Install Telmatics SDK environment
The SDK contains cross-compiler for target device and all the required library,
please contact with your HW vendor for instruction on how to install SDK environment.

2.1.2. security library.
The reference stack provide capabilities to integrate with licensed, 3rd party library
for message security services. However, the SW can be compiled without this library,
thus the SW can run normally by disabling the security service. If you are interested
in evaluating security capability of the SW stack, please contact with HW vendor.

2.1.3. asn1c skeletons
The ETSI stack of the reference stack requires open source asn1c skeletons files
to compile. The main project https://github.com/vlm/asn1c is moving quickly. So
we recommend to use a forked version, which is stable and proven to work with the reference
stack. The forked version is lockated at:

https://github.com/leisun124/asn1c.git

clone this project into your choosen directory in your build machine, for
example in /home/username/asn1c


2.1.4 Build the reference Stack

    1. Create a build directory in the root of the reference stack directory.
        cd <ITS sack root dir>
        mkdir build
    2. Setup cross compiler environment.
        e.g., source /opt/poky-agl/4.0.0/environment-setup-aarch64-agl-linux
    3. Set environment variables.
        3.1 If you want the security enabled, then do:
            export AEROLINK_PATH=\path\to\aerolink\ (This is optional)
            There must be an "include" directory within the libs directory for the headers.
        3.2 If you want to enable the ETSI stack, you need to provide the asn1c path via env
        varable:
            export ASN1C_PATH=\path\to\asn1c (this is optional)
    4. In the build directory, run
        cmake ../
        make

    5. If build was succesful, there will be a binary produced:
        ./tests/applicationTest/qits


2.1.5 Run the stack test program
Transfer the result qits binary and "ObeConfig.conf" in the reference stack root
directory to your target HW, (adb push). The ObeConfig.conf file contains various
parameters for the stack to run properly.

If you have aerolink security library and plan to turn on the "EnableSecurity" option
in the ObeConifg.conf, then you need to setup Aerolink related options like follows:

export following environment variables in the target HW:

Note: Your Aerolink path may be different but must include "config" or "state"
based on the CONFIG or STATE environment variable.

AEROLINK_CONFIG_DIR=/etc/aerolink/config
AEROLINK_STATE_DIR=/etc/aerolink/state

Optional:
AEROLINK_ENTROPY_SOURCE=/dev/urandom
AEROLINK_UTILS_DIR=/usr/local/bin/aerolink
AEROLINK_TRACE=all
AEROLINK_TRACE_LOGNAME=/tmp/aerolink.log

You will also need to add all of the appropriate demo IEEE certificates
(located in AEROLINK_STATE_DIR/certificates) via the
    /usr/local/bin/aerolink/certadm command.
For each cert, run the executable in the following manner:
certadm add /AEROLINK_STATE_DIR/certificates/NAME_OF_CERT.cert

Now your are ready to run the testing app, see following section for command
line options.

NOTE: presently, the supporting version of Aerolink is 4.8.2

2.1.6 Command line options
RITS contains an extensive implementation of the fundamental functionality that
an ITS Stack should develop. This functionality can be used using the following
command line tools:

Usage: qits [options] <ConfigFileRelativePath>

Example: qits -t -l -s ./ObeConfig.conf
Above cmd will run transmit mode with Local Dynamic Map (LDM) and the safety apps.

Example: qits -t -b ./ObeConfig.conf
Above cmd will run transmit mode with Basic Safety Messages (BSMs) and the safety apps.

Example: qits -t -c ./ObeConfig.conf
Above cmd will run transmit mode with CAM message

Example: qits -r -b ./ObeConfig.conf
Above will run: receive mode with Basic Safety Messages (BSMs) and the safety apps.

At least one option is needed and Config File is always required. Option order
does not matter. If config file is not specified, qits will try to resolve a
configuration file or try to find "ObeConfig.conf current directory"

Options:
-h Prints help options.\n
-t Transmits Cv2x data. Runs with -b/-c/-d for BSMS/CAMS/DENMS respectively.
-r Receives Cv2x data. Runs with -b/-c/-d for BSMS/CAMS/DENMS respectively.
-s Safety Apps Mode; Adds -l if not specified. Runs by default with -b.
-p <Pre-Recorded File Path> Transmists from pre-recorded file.
-T Tunnel Mode Transmit.
-x Tunnel Mode Receive (As of 08/16/2019 this option hasn't been tested) Beta mode and some TODOs
-l LDM mode; Adds -r if nothing specified. Use it with -r or -j.
-b Transmits and Receives BSMS.
-c Transmits and Receives CAMs. (As of 08/16/2019 this option hasn't been implemented)
-d Transmits and Receives DENMs. (As of 08/16/2019 this option hasn't been implemented)
-i <ip> <port> Simulating CV2x with kinematics and can interfaces.
-j <ip> <port> <re-Recorded File Path> Simulating CV2x from pre-recorded file.

3. Source Code Structure

The folders are arranged following Linux premises as:

RITS Stack root/
 src/
   Application/
   qCoder/
     src/
       /etsi
         CAM/
         DENM/
         dictionary/
      include/
    qMessenger/
    tests/
    CMakeLists.txt
    ObeConfig.conf
    README.md

In the "RITS Stack root/" directory, it contains:
README.md - this file
CMakeLists.txt - Top level CMake file that controls the compile/make of this project
ObeConfig.conf - An example configuration file.
src/ - source code and header files.
tests/  -Top level test program and unit test program for various components.

4. Functional Blocks, Modules and Classes
The stack supports both SAE and ETSI standards, with limited implementations.
And the programming langugage is mixed C and C++. All the ASN encoding/decoding
is implemented with C, and the rest of the functionalities are implemented with
C++. From protocol stack point of view, the reference stack impmeneted following
layers:

For SAE stack:
  WSMP
  BSM
For ETSI stack:
  BTP
  GeoNetwork
  CAM
  DENM

And also IEEE1609.2 Security service which is available for both SAE and ETSI stack.

4.1 Functional blocks.
The directory structure refects the functional blocks, in summary:
  qCoder - ASN encoding/decoding functions, in which the SAE related enc/dec
    is hand coded for best performance, and ETSI related enc/dec is
    automatically generated with asn1c.

  qMessenger - C++ wrapper class for various message exchange functionality,
    for example message exchange for kinematics information, and vehicle CAN
    information, Radio Tx/Rx function, and GeoNetwor Router.
  qApplication - Top level Application object.

5. Documentation:
