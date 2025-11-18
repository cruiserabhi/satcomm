Simulation Folder contains the Simulation of Telematics SDK APIs.

This has been compiled and tested with Ubuntu versions 18.04

User guide has been updated with the steps to perform build.
https://developer.qualcomm.com/software/qualcomm-telematics-sdk/user-guide

Offline builds
---------------
If internet access is restricted, set the `DOWNLOAD_CACHE` environment variable to a
directory containing pre-downloaded dependency archives (for example, the gRPC and
jsoncpp tarballs expected by the simulation Makefile). The build system will copy
artifacts from that cache before attempting any network fetches.


