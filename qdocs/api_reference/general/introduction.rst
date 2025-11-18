..
 
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear

..
    =============================================================================
                              Edit History
 when       who     what, where, why
 --------   ---     ------------------------------------------------------------
 02/27/17   LB      Created to support the Telematics SDK
 01/28/19   RK      Updated with Audio support to the Telematics SDK
 05/10/19   SM      Added TCU Activity management support in Telematics SDK
 06/10/19   RK      Updated with Audio Play, Capture and DTMF support in Telematics SDK
 07/22/19   AK      Added Thermal Shutdown Management support in Telamatics SDK
 09/04/19   AG      Added Remote SIM support to Telematics SDK
 09/05/19   AK      Added Audio Loopback, Tone Generation support in Telematics SDK
 09/15/19   RV      Added Compressed Audio Format Playback support in Telematics SDK
 09/16/19   RV      Added Modem Config Support to Telematics SDK
 10/10/19   GS      Added Data Filter Support to Telematics SDK
 10/22/19   SD      Added location concurrent reports support to Telematics SDK
 10/22/19   SD      Added location constraint time uncertainty support to Telematics SDK
 11/05/19   RV      Added Audio Format Transcoding Support in Telematics SDK
 11/10/19   FS      Added Data Networking Features support to Telematics SDK
 11/15/19   RV      Added compressed audio format playback on voice paths support in Telematics SDK
 01/15/20   SM      Added software bridge management support in Telematics SDK
 01/15/20   FS      Added Socks Proxy Feature Support in Telematics SDK
 02/14/20   SD      Added location configurator support to Telematics SDK
 03/12/20   SD      Added robust location support to location configurator in Telematics SDK
 04/09/20   AK      Added security section with SE-linux details supported in Telematics SDK
 05/22/20   FS      Added L2TP Feature Support in Telematics SDK
 05/28/20   AS      Added Logging supported in Telematics SDK
 08/17/20   SD      Added measurement, nmea, system info, minGpsWeek, minSVElevation, delete data, dREngine and secondaryBand support in Telematics SDK
 11/18/20   ZH      Added cv2x SE-linux interface details in Telematics SDK
 11/20/20   SD      Added year of hardware and configure engine state support in Telematics SDK
 01/12/21   SD      Added terrestrial positioning support in Telematics SDK
 01/13/21   RV      Added SSR(sub system restart) support for Audio.
 02/08/21   SD      Added configuring NMEA types support for Location.
 01/29/21   FS      Updated data subsystem readiness flow for new methodology
 02/12/21   RV      Added new sub system Readiness APIs for Audio.
 03/25/21   RV      Added new calibration init status API for audio.
 04/01/21   FS      Added Data Serving system how to dedicated radio bearer, service status and roaming status.
 04/01/21   BS      Added documentation for sensor sub-system.
 06/07/21   BS      Added documentation for platform sub-system.
 06/16/21   SS      Added Remote SIM provisioning support in Telematics SDK.
 07/02/21   BS      Updated thermal subsystem readiness flow for new methodology
 08/09/21   BS      Added further EFS API related information for platform sub-system.
 08/17/21   FS      Added documentation for subsystem restart
 09/22/21   RV      Added new sub system Readiness APIs for Modem Config.
 11/25/21   RR      Added support for Xtra feature for location in Telematics SDK
 12/24/21   MH      Added filesystem control API for ECALL and OTA operation for platform sub-system.
 12/21/21   BS      Added documentation for sensor self test.
 07/11/22   SS      Added Third Party Service eCall over IMS support in Telematics SDK
 11/02/22   FS      Added support Wlan Management support in Telematics SDK
 06/03/23   MH      Added notifications for thermal sub-system.
 10/13/23   FS      Added support for Diagnostics Services in Telematics SDK
 11/09/23   SP      Migrated to RST
 NOTE: for an appendix, the label must match the filename
 =============================================================================*/

============
Introduction
============

Purpose
-------
The Telematics software development kit (TelSDK) is a set of application programming interfaces (APIs) 
that provide access to the QTI-specific hardware and software capabilities.

This document is intended to act as a reference guide for developers by providing details of the TelSDK 
APIs, function call flows and overview of TelSDK architecture.

Scope
-----
This document focusses on providing details of the TelSDK APIs. It assumes that the developer is familiar 
with Linux and C++11 programming.

Conventions
-----------
Function declarations, function names, type declarations, and code samples appear in a different font. 

For example,

``#include``

Parameter directions are indicated as follows:

.. code-block:: cpp 
   
   [in]      paramname  Indicates an input parameter.
   [out]     paramname  Indicates an output parameter.
   [in,out]  paramname  Indicates a parameter used for both input and output.

Most APIs are asynchronous as underlying sub-systems such as telephony are asynchronous. API names follow 
the convention of using prefix " get " for synchronous calls and " request " for asynchronous calls. 
Asynchronous responses such as listener callbacks come on a different thread than the application thread.

SDK Versioning
--------------

The following convention is used for versioning the SDK releases

**SDK version (major.minor.patch)**

SDK_VERSION = 1.0.0

- **Major version** -- This number will be incremented whenever significant changes or features are introduced.
- **Minor version** -- This number will be incremented when smaller features with some new APIs are introduced.
- **Patch version** -- If the release only contains bug fixes, but no API change then the patch version would be incremented, No change in the actual API interface. 

.. note::
    Use telux::common::Version::getSdkVersion() API to query the current version of SDK or refer the VERSION 
    file in the repository

Public API Status
-----------------

Public APIs are introduced and removed (if necessary) in phases. This allows users of an existing API that is being 
Deprecated to migrate. APIs will be marked with note indicating whether the API is subject to change or if it is not 
recommended to use the API as follows:

- **Eval** -- This is a new API and is being evaluated. It is subject to change and could break backwards compatibility.
- **Obsolete** -- API is not preferred and a better alternative is available.
- **Deprecated** -- API is not going to be supported in future releases. Clients should stop using this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
