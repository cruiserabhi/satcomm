..
   *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

..
   =============================================================================
                             Edit History
   when       who     what, where, why
   --------   ---     ------------------------------------------------------------
   11/2023    SP      Migrate to RST
   =============================================================================*/

===================
Functional Overview
===================


.. figure:: /../images/sdk_overview.png
   :scale: 75

   **SDK-Overview**

Overview
--------
The Telematics library runs in the user space of the Linux system. It interacts with Telephony services and other sub-systems to provide various services like phone calls, SMS etc. These services are exposed by the SDK through fixed public APIs that are available on all Telematics platforms that support SDK. The Telematics APIs are grouped into the following functional modules:

**Telephony**
   Telephony sub-system consists of APIs for functions related to Phone, Call, SMS and Signal Strength, Network Selection and Serving System Management.

:ref:`SIM Card Services<general/overview:SIM Card Services>`
   SIM Card services sub-system consists of APIs to perform SIM card operations such as Send APDU messages to SIM card applications, SIM Access Profile(SAP) operations etc.

:ref:`Location Services<general/overview:Location Services>`
   Location Services sub-system consists of APIs to receive location details such as GNSS Positions, Satellite Vehicle information, Jammer Data signals, nmea and measurements information. The location manager sub-system also consists of APIs to get location system info, request energy consumed, get year of hardware information, get terrestrial position information and cancel terrestrial position information. LocationConfigurator allows general engine configurations (example: TUNC, PACE etc),configuration of specific engines like SPE (example: minSVElevation, minGPSWeek etc) or DRE, deletion of warm and cold aiding data, NMEA configuration and support for XTRA feature.

**Connection Management**
   Connection Management sub-system consists of APIs for establishing Cellular WAN/ Backhaul connection sessions and for Connection Profile Management etc.

:ref:`Audio Management<general/overview:Audio>`
   Audio Management sub-system consists of APIs for Audio management such as setting up audio streams, switching devices on streams, apply volume/mute etc

:ref:`Thermal Management<general/overview:Thermal Management>`
   Thermal Management sub-system consists of APIs to get list of thermal zones, cooling devices and binding information. The sub-system also provides notifications about certain thermal related events such as when trip event occur for any thermal zone or cooling device changes its level.

:ref:`Thermal Shutdown Mangement<general/overview:Thermal Shutdown Management>`
   Thermal shutdown management sub-system consists of APIs to get/set the thermal auto-shutdown mode and listen to its updates.

:ref:`TCU Activity Management<general/overview:TCU Activity Management>`
   TCU Activity Management sub-system consists of APIs to get TCU-activity state updates, set the TCU-activity state, etc.

:ref:`Remote SIM Services<general/overview:Remote SIM>`
   Remote SIM sub-system consists of APIs that allow a device that does not have a SIM physically connected to it to access a SIM remotely (e.g. over BT, Ethernet, etc.) and perform card operations on that SIM, such as requesting reset, transmitting APDU messages, etc.

:ref:`Modem Config Services<general/overview:Modem Config Management>`
   Modem Config sub-system consists of APIs that allow to request modem config files, load/delete a modem config file from modem's storage, activate/deactivate a modem config file, get/set auto selection mode for config files.

:ref:`Data Network Management<general/overview:Data Services>`
   Data Network Management sub-system consists of APIs to setup VLAN, static NAT, Firewall, Socks, etc.

:ref:`Sensor services<general/overview:Sensors>`
   The sensor sub-system provides API to configure and acquire data from underlying hardware sensors like accelerometers, gyroscopes among others.

:ref:`Platform services<general/overview:Platform>`
   The platform sub-system provides APIs to configure and control platform functionalities, like starting an EFS backup, control filesystem for ECALL and OTA operations.
   This sub-system also provides notifications about certain system related events, for instance filesystem events such as EFS restore and backup events.

:ref:`Remote SIM Provisioning<general/overview:Remote SIM Provisioning>`
   Remote SIM provisioning provides API to add profile, delete profile, activate/deactivate profile on the embedded SIMs (eUICC) , get list of profiles, get server address like SMDP+ and SMDS and update SMDP+ address, update nick name of profile and retrieve Embedded Identity Document(EID) of the SIM.

:ref:`Debug Logger<general/overview:Debug Logger>`
   Logger consists of API that can be utilized to log messages from SDK Applications.

:ref:`WLAN Management<general/overview:WLAN Management>`
   The WLAN management subsystem consists of APIs to configure, enable, and set access point and station configurations.

:ref:`Diagnostics Services<general/overview:Diagnostic Services>`
   The Diagnostics services subsystem consists of APIs to configure, start and stop diagnostic logging.

:ref:`Satcom services<general/overview:Satcom Services>`
   The Satcom services subsystem includes APIs to configure, start, stop, and transmit non-IP data over the Non-Terrestrial Network.

Telematics SDK classes can be broadly divided into the following types:
- Factory -- Factory classes are central classes such as PhoneFactory which can be used to create Manager classes corresponding to their sub-systems such as PhoneManager.
- Manager -- Manager classes such as PhoneManager to manage multiple Phone instances, CardManager to manage multiple SIM Card instances etc.
- Observer/Listener -- Listener for unsolicited responses.
- Command Callback -- Single-shot response callback for asynchronous API requests.
- Logger -- APIs to log messages, control the log levels.

Features
--------
Telematics SDK provides APIs for the following features:


Call Management
~~~~~~~~~~~~~~~
CallManager, Phone and PhoneManager APIs of Telematics SDK provides call related control operations such as

- Initiate a voice call
- Answer the incoming call
- Hold the call
- Hangup waiting, held or active call

CallManager and PhoneManager also provides additional functionality such as

- Allowing conference, and switch between waiting or holding call and active call
- Emergency Call (dial 112)
- Third Party Service (TPS) Emergency Call (dial custom number)
- Notifications about call state change

SMS
~~~
SMS Manager APIs of Telematics SDK provides SMS related functionality such as

- Sends and receives SMS messages of type GSM7, GSM8 and UCS2

SIM Card Services
~~~~~~~~~~~~~~~~~
The SIM Card operations are performed by CardManager and SapCardManager.

CardManager APIs of Telematics SDK perform operations on UICC card such as

- Open or close logical/basic channel to ICC card
- Transmit Application Protocol Data Unit (APDU) to the ICC Card over logical/basic channel
- Receive response APDU from the ICC Card with the status
- Notify about ICC card information change

SapCardManager APIs provides SIM Access Profile(SAP) related functionality such as

- Open or close SIM Access Profile(SAP) connection
- Transmit Application Protocol Data Unit (APDU) over SAP connection
- Receive response APDU over SAP connection
- Perform SAP operations such as Answer to Reset(ATR), SIM Power off, SIM Power On, SIM Reset and fetch Card Reader status.

Phone Information
~~~~~~~~~~~~~~~~~
Phone APIs of Telematics SDK provides phone related information such as

- Get Service state of phone i.e. EMERGENCY_ONLY, IN_SERVICE and OUT_OF_SERVICE
- Get Radio state of device i.e RADIO_STATE_OFF, RADIO_STATE_ON and RADIO_STATE_UNAVAILABLE
- Retrieve the signal strength corresponding to the technology supported by SIM
- Device Identity
- Set or Request Operating Mode
- Subscription Information

Location Services
~~~~~~~~~~~~~~~~~
Location Services APIs of Telematics SDK provide the mechanism to register listener and to receive location updates, satellite vehicle information, jammer signals, nmea and measurement updates. The location manager sub-system also consists of APIs to get location system info, request energy consumed, get year of hardware information, get terrestrial position information and cancel terrestrial position information.
Following parameters are configurable through the APIs.

- Minimum time interval between two consecutive location reports.
- Minimum distance travelled after which the period between two consecutive location reports depends on the interval specified.

LocationConfigurator allows general engine configurations (example: TUNC, PACE etc),configuration of specific engines like SPE (example: minSVElevation, minGPSWeek etc) or DRE, deletion of warm and cold aiding data, NMEA configuration and support for XTRA feature.

Data Services
~~~~~~~~~~~~~
Data Services APIs in the Telematics SDK used for cellular connectivity, modem profile management, filters management, and networking.

Data Connection Manager APIs provide functionality such as

- start / stop data call
- listen for data call state changes

Data Profile Manager APIs provide functionality such as

- List available profiles in the modem
- Create / modify / delete / modify modem profiles
- Query for the selected profile

Data Serving System Manager APIs provide functionality such as

- Get dedicated radio bearer status
- Request Modem Service Status
- Request Modem Roaming Status

Data Filter Manager APIs provide functionality such as

- get / set data filter mode per data call
- get / set data filter mode for all data call in up state
- add / remove data filter per data call
- add / remove data filter for all data call in up state

Data VLAN Manager APIs provide functionality such as

- Create / remove VLAN
- Query VLAN info
- Bind / unbind VLAN to PDN
- Query current VLAN to PDN mapping

Data Static LAN Manager APIs provide functionality such as

- Add / remove static LAN entry
- Request current static NAT entries

Data Firewall Manager APIs provide functionality such as

- Add / remove DMZ entry
- Query current DMZ entries
- Set Firewall configuration to enable / disable Firewall
- Query current status of Firewall
- Add / remove Firewall entry
- Query Firewall entry rules

Data Socks Manager APIs provide functionality such as

- Enable/Disable Socks feature

Data L2TP Manager APIs provides functionality such as

- Set L2TP configuration to enable/disable L2TP, TCP Mss and MTU size
- Add / remove L2TP tunnel
- Query active L2TP configuration

Data Software Bridge Manager provides interface to enable packet acceleration for non-standard WLAN and Ethernet physical interfaces.
It facilitates to configure a software bridge between the interface and Hardware accelerator.
Its APIs provide functionality such as

- Add / remove a software bridge
- Query the software bridges configured in the system
- Enable / Disable the software bridge management

Data Serving System Manager provides the interface to access network and modem low level services. The API includes method for:

- Request Modem Service Status
- Request Modem Roaming Status
- Register to get notifications when Service Status and Roaming status Change

Data client Manager APIs provide functionality such as
- fetch device data usage
Configurations relevant to device data usage monitoring are available in data related configuration files - /etc/data/mobileap_cfg.xml and /etc/data/ipa/IPACM_cfg.xml

Network Selection and Serving System Management
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Network Selection and Service System Management APIs in the Telematics SDK used for configuring the networks and preferences

Network Selection Manager APIs provide functionality such as

- request or set network selection mode (Manual or Automatic)
- scan for available networks
- request or set preferred networks list

Serving System Manager APIs provide functionality such as

- request and set service domain preference and radio access technology mode preference for searching and registering (CS/PS domain, RAT and operation mode)

C-V2X
~~~~~
The C-V2X sub-system contains APIs that support Cellular-V2X operation.

Cellular-V2X APIs in the Telematics SDK include Cv2xRadioManager and Cv2xRadio interfaces.

Cv2xRadioManager provides an interface to a C-V2X capable modem. The API includes methods for

- Enabling C-V2X mode
- Disabling C-V2X mode
- Querying the status of C-V2X
- Updating the C-V2X configuration via a config XML file

Cv2xRadio abstracts a C-V2X radio channel. The API includes methods for

- Obtaining the current capabilities of the radio
- Listen for radio state changes
- Creating and Closing an RX subscription
- Creating and Closing a TX event-driven flow
- Creating and Closing  a TX semi-persistent-scheduling (SPS) flow
- Updating TX SPS flow parameters
- Update Source L2 Info

Audio
~~~~~

The Audio subSystem contains of APIs that support Audio operation.

Audio APIs in the Telematics SDK include AudioManager, AudioStream, AudioVoiceStream, AudioPlayStream, AudioCaptureStream, AudioLoopbackStream, AudioToneGenerator, Transcoder interfaces.

AudioManager provides an interface for creation/deletion of audio stream. The API includes methods for

- Query readiness of subSystem
- Query supported "Device Types"
- Query supported "Stream Types"
- Creating Audio Stream
- Deleting Audio Stream

AudioStream abstracts the properties common to different stream types. The API includes methods for

- Query stream type
- Query routed device
- Set device
- Query volume details
- Set volume
- Query mute details
- Set mute

AudioVoiceStream along with inheriting AudioStream, provides additional APIs to manage voice call session as stated below.

- Start Voice Audio Operation
- Stop Voice Audio Operation
- Play DTMF tone
- Detect DTMF tone

AudioPlayStream along with inheriting AudioStream, provides additional APIs to manage audio play session as stated below.

- Write audio samples
- Write audio samples for compressed audio format
- Stop Audio for compressed audio format
- Play compressed format audio on voice paths

AudioCaptureStream along with inheriting AudioStream, provides additional APIs to manage audio capture session as stated below.

- Read audio samples

AudioLoopbackStream along with inheriting AudioStream, provides additional APIs to manage audio loopback session as stated below.

- Start loopback
- Stop loopback

AudioToneGeneratorStream along with inheriting AudioStream, provides additional APIs to manage audio tone generator session as stated below.

- Play tone
- Stop tone

Transcoder provides APIs to manage audio transcoder which is able to perform below operations.

- Convert one audio format to another

Audio SDK provides details of supported "Device Types" and "Stream Types" in the audio subsystem of Reference Telematics platform.

“Device Type” encapsulates the type of device supported in Reference Telematics platform.
The representation of these devices would be made available via public header file <usr/include/telux/audio/AudioDefines.hpp>.

Example: DEVICE_TYPE_XXXX

Internally SDK DeviceTypes are mapped to Audio HAL devices as per <usr/include/system/audio.h>.

In current release it is mapped per below table.

**Current Device Mapping Table:**

.. csv-table::
   :header: "SDK Audio Device Representation", "Audio HAL Representation"
   :widths: 50, 50 
   
   "DEVICE_TYPE_SPEAKER", "AUDIO_DEVICE_OUT_SPEAKER"
   "DEVICE_TYPE_MIC", "AUDIO_DEVICE_IN_BACK_MIC" 

However Device Mapping is configurable as stated below. This configurability provides flexibility to map different Audio HAL devices
to SDK representation.

Update tel.conf file with below details before boot of system.

- NUM_DEVICES -- Specifies the number of device types supported
- DEVICE_TYPE -- Specifies the SDK type of each device (in comma separated values)
- DEVICE_DIR -- Specifies the device direction for each device in order above (in comma separated values)
- AHAL_DEVICE_TYPE -- Specifies the mapped Audio HAL type of each device (in comma separated values)

Example:

.. note::
   The default values provided here are based on QTI's reference design.

.. code-block:: text 
   
   NUM_DEVICES=6
   
   DEVICE_TYPE=1,2,3,257,258,259

   DEVICE_DIR=1,1,1,2,2,2

   AHAL_DEVICE_TYPE=2,1,4,2147483776,2147483652,2147483664

For any stream types, maximum device supported would be one. Single stream per multiple devices not supported.
For voice stream Rx Device would decide corresponding Tx Device pair as decided by Audio HAL.

**NOTE FOR SYSTEM INTEGRATORS:**

The mapping of Audio devices to Audio HAL devices is static currently based on QTI's Reference Telematics platform.
For custom platforms this mapping need to be updated.

“Stream Type” encapsulates the type of stream supported in Reference Telematics Platform.
The representation of these stream types made available via public header file <usr/include/telux/audio/AudioDefines.hpp>.

Example:  VOICE_CALL, PLAY, CAPTURE, LOOPBACK, TONE_GENERATOR etc

**Volume Support Table:**

This table captures scenarios where the volume could be modified.

.. csv-table::
   :header: "Stream Type", "Stream Direction (RX)", "Stream Direction (Tx)"
   :widths: 26, 36, 36 
   
   "VOICE_CALL", "Applicable", "Not Applicable"
   "PLAY","Applicable", "Not Applicable" 
   "CAPTURE", "Not Applicable", "Applicable"
   "LOOPBACK", "Not Applicable", "Not Applicable" 
   "TONE_GENERATOR", "Not Applicable", "Not Applicable" 

In case QTI's reference design does not support volume for specific stream category, API responds with error.

**Mute Support Table:**

This table captures when stream could be muted and in which direction.

.. csv-table::
   :header: "Stream Type", "Stream Direction (RX)", "Stream Direction (Tx)"
   :widths: 26, 36, 36

   "VOICE_CALL", "Applicable", "Applicable"
   "PLAY", "Applicable", "Not Applicable"
   "CAPTURE", "Not Applicable", "Applicable"
   "LOOPBACK", "Not Applicable", "Not Applicable"
   "TONE_GENERATOR", "Not Applicable", "Not Applicable"

In case QTI's reference design does not support mute for specific stream category, API responds with error.

.. note::
   If mute operations is performed for  play or capture stream direction(Tx or RX), the stream will get muted irrespective of the direction provided.

Thermal Management
~~~~~~~~~~~~~~~~~~
Thermal Management APIs in the Telematics SDK are used for reading thermal zone, cooling device and binding information.

Thermal Management APIs provide functionality such as

- get thermal zones with thermal zone description, current temperature, trip points and binding info
- get cooling devices with cooling device type, maximum and current cooling level
- get thermal zone by Id
- get cooling device by Id

Thermal Shutdown Management
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Thermal Shutdown Management APIs provide funtionality such as

- Query auto-shutdown mode.
- Set auto-shutdown mode.
- Get notifications on auto-shutdown mode updates.

TCU Activity Management
~~~~~~~~~~~~~~~~~~~~~~~
TCU-activity Manager APIs in Telematics SDK provides TCU-activity state related operations such as

- Set the activity state of the machines in TCU
- Get notifications about the imminent activity state changes on a machine in TCU
- Set the modem activity state
- Get all machine names
- Query the current TCU-activity state

Remote SIM
~~~~~~~~~~
Remote SIM APIs in the Telematics SDK allow a device to use the WWAN capabilities of a SIM on another device.

Remote SIM APIs provide functionality such as

- Sending card events (reset, power up, errors) to the modem
- Sending/receiving APDU messages from/to the modem and remote SIM.
- Receiving operations from the modem (disconnect, power up, reset) to the remote SIM.

Modem Config Management
~~~~~~~~~~~~~~~~~~~~~~~
Modem Config APIs in the Telematics SDK provides modem config related functionalities such as

- Request modem config files from modem's storage.
- Load a modem config file to modem's storage.
- Activate/Deactivate a modem config file from modem's storage.
- Get Active config info details.
- Get/Set config auto selection mode.
- Delete a modem config file from modem's storage.
- Ability to get notified whenever a SW config file is activated.

Sensors
~~~~~~~
The sensor sub-system provides APIs to

- Configure and acquire continuous stream of data from an underlying sensor
- Create multiple clients for a given sensor, each of which can have their own configuration (sampling rate, batch count) for data acquisition.
- Query and control sensor features available on the hardware or those offerred by the software framework. Availability of sensor features depend on the sensor hardware being used and the capabilities it offers.

In addition to the sensor sub-system APIs, configuration items relevant to the underlying sensors are also available in /etc/sensors.conf on the device filesystem.
This includes the range for the sensors, the limits on sampling frequency and batch count among other parameters.

Platform
~~~~~~~~
The platform sub-system provides APIs to

- Register and listen to filesystem events such as EFS backup and restore notifications
- Request EFS backup

Remote SIM Provisioning
~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/rsp_block_diagram.png
   :scale: 75

   **Remote SIM Provisioning**

The Telematics Application can leverage Remote SIM Provisioning (RSP) APIs to perform eUICC profile management operations.

Remote SIM Provisioning APIs in Telematics SDK provides operations such as

- Download a profile on the eUICC. Allow downloading of profile based on activation code and confirmation code. Also provide user consent for downloading of profile.
- Enable or disable a profile to activate/deactivate subscription corresponding to profile.
- Delete a profile from an eUICC.
- Query list of profile on the eUICC.
- Get and update the server address( SMDP+ and SMDS)
- Get EID of the eUICC.
- Update nickname of the profile.
- Perform memory reset which allows to delete test and operational profiles or reset to default SMDP+ address.

When modem LPA/eUICC needs to reach SMDP+/SMDS server on the cloud for HTTP transaction, the HTTP
request is sent to RSP service i.e RSP HTTP daemon. The RSP HTTP Daemon performs these HTTP
transactions on behalf of modem with SMDP+/SMDS server running on the cloud. The HTTP response from
cloud is sent back to modem LPA/eUICC to take appropriate action.

Debug Logger
~~~~~~~~~~~~
Logging APIs in the Telematics SDK provides logging related functionalities such as

- Runtime configurable logging to console, diag and file.
- Possible LOG_LEVEL values are NONE, PERF, ERROR, WARNING, INFO, DEBUG.

WLAN Management
~~~~~~~~~~~~~~~
WLAN management APIs in the Telematics SDK provide services related to the following Wi-Fi functionality.

- Enable/disable WLAN.
- Set/request WLAN mode: number of access points and number of stations to be enabled.
- Request current WLAN status.
- Set/request an access points configuration.
- Request access point status
- Set/request a station's configuration.
- Request station status
- Request list of devices connected to any access point.
- Restart hostapd and wpa_supplicant daemons

Satcom Services
~~~~~~~~~~~~~~~
Satcom services APIs in the Telematics SDK offer functionalities for configuring the NTN and transmitting non-IP data over it. These include:

- Enabling/disabling NTN.
- Updating the system selection specifiers (SFL list) used by the modem to scan for the NTN network.
- Requesting NTN network capabilities.
- Sending/receiving data over NTN.
- Enabling background PLMN scans.

Diagnostic Services
~~~~~~~~~~~~~~~~~~~
Two methods are supported to collect logs; file and callback method. In file method, logs are captured
and saved to file. In callback method, log is passed to client provided callback whenever new log is generated.
Logging is also provided per device or peripheral level. With device level logging, logs from all peripherals in
that device is collected. With peripheral logging, logs from client selected perihpherals only are captured.
Diagnostic services APIs in the Telematics SDK provide the following functionality.

- Configure diagnostic servics which include:

   - Set logging level (Device/Peripheral).
   - Set logging method (File/Callback).
   - Set logging mode (Streaming, Threshold, or Circular Buffer).
   - Set mask file for both modem and or EAP.
   - Set max file size and max number of files generated if file method logging is selected.

- Start/Stop logging.

Subsystem Restart
------------------
Subsystem restart events occur when device operating system or services crashes due to any reason and then reboots
to operational state. This section explains notifications that are sent to application when such an event
occurs, the impact on application, notifications that are sent to application when device recovers to
operating state, and suggested action application should take after recovery.

Examples of Subsystem Restart events:

- External application processor crash
- Modem application processor crash
- Modem processor crash

If application is running on either application processors when it crashes, application is expected to
be restarted to initial state. For other scenarios, details are explained below for each subsystem.

Data Services
~~~~~~~~~~~~~

Data services behavior when Subsystem Restart event occur is shown in table below:

.. figure:: /../images/data_ssr.png
   :scale: 65

   **Data SSR and Recovery**


Security
--------

SELinux
~~~~~~~
SELinux is an access control framework provided by the Linux kernel. It provides a mechanism to restrict/control access to system resources such as file nodes and sockets. SELinux framework expects any process running in userspace to declare all its interactions with the system resources in the form of SELinux policies. On platforms enabled with SELinux, an app that uses an SDK API would also need to declare its usage through SELinux policies to ensure that it has all the required permissions.

Listed below are the SELinux Interfaces which are generic for any API in particular namespace that app needs to declare in its policies.

.. note::
   For the below list, let us consider the application's security context to be ``app_t`` (also called domain context).

.. csv-table::
   :header: "Namespace", "SELinux interface", "Arguments", "Usage"
   :widths:  10, 25, 25, 40

   "tel", "telux_allow_tel()", "domain context", "telux_allow_tel(app_t)"
   "data", "telux_allow_data()", "domain context", "telux_allow_data(app_t)"
   "audio", "telux_allow_audio()", "domain context", "telux_allow_audio(app_t)"
   "loc", "telux_allow_loc()", "domain context", "telux_allow_loc(app_t)"
   "thermal", "telux_allow_thermalmanager()", "domain context", "telux_allow_thermalmanager(app_t)"
   "power", "telux_allow_power()", "domain context", "telux_allow_power(app_t)"
   "config", "telux_allow_modemconfig()", "domain context", "telux_allow_modemconfig(app_t)"
   "cv2x", "telux_allow_v2x()", "domain context", "telux_allow_v2x(app_t)"
   "sensor", "telux_allow_sensor()", "domain context", "telux_allow_sensor(app_t)"
   "platform", "telux_allow_platform()", "domain context", "telux_allow_platform(app_t)"
   "satcom", "telux_allow_satcom()", "domain context", "telux_allow_satcom(app_t)"

The following example illustrates how an application can incorporate the SELinux interfaces exposed by SDK in its SELinux policies. Below code snippet is part of a Type Enforcement (TE) file of the application which grants required permissions to perform SDK data operations.

.. code-block:: cpp

   policy_module(application, 1.0)
   type app_t;

   #Granting SDK data client permissions to the application

   telux_allow_data(app_t);

In addition to the above SELinux interfaces list, below are the SELinux interfaces specific to a usecase. When an app needs to use any API, it should
identify which SELinux interface to be used corresponding to the permission type from the below table and add it to the policy file.

To determine which permission type to be used for a API, please refer to the documentation for the API in the API Reference or in the API header file

.. note::
   The system integrator has the option to turn on/off this feature, where APIs related to a particular use case require certain permissions. If this feature is turned off, the use case specific permissions listed below are not required by the caller.

+--------------+------------------------------------+------------------------------------------+
| Tech Area    | Permission Type                    | SELinux Interface                        |
+==============+====================================+==========================================+
| Audio        | TELUX_AUDIO_VOICE                  | telux_allow_audio_voice                  |
|              +------------------------------------+------------------------------------------+
|              | TELUX_AUDIO_PLAY                   | telux_allow_audio_play                   |
|              +------------------------------------+------------------------------------------+ 
|              | TELUX_AUDIO_FACTORY_TEST           | telux_allow_audio_factory_test           |
|              +------------------------------------+------------------------------------------+
|              | TELUX_AUDIO_CAPTURE                | telux_allow_audio_capture                |
|              +------------------------------------+------------------------------------------+
|              | TELUX_AUDIO_TRANSCODE              | telux_allow_audio_transcode              |
+--------------+------------------------------------+------------------------------------------+
| Data         | TELUX_DATA_SETTING                 | telux_allow_data_setting                 |
|              +------------------------------------+------------------------------------------+
|              | TELUX_DATA_CALL_OPS                | telux_allow_data_call_ops                |
|              +------------------------------------+------------------------------------------+
|              | TELUX_DATA_CALL_PROPS              | telux_allow_data_call_props              |
|              +------------------------------------+------------------------------------------+
|              | TELUX_DATA_PROFILE_OPS             | telux_allow_data_profile_ops             |
|              +------------------------------------+------------------------------------------+
|              | TELUX_DATA_FILTER_OPS              | telux_allow_data_filter_ops              |
|              +------------------------------------+------------------------------------------+
|              | TELUX_DATA_NETWORK_CONFIG          | telux_data_network_config                |
+--------------+------------------------------------+------------------------------------------+
| ModemConfig  | TELUX_CONFIG_MODEM_CONFIG          | telux_allow_config_modem_config          |
+--------------+------------------------------------+------------------------------------------+
| Power        | TELUX_POWER_CONTROL_STATE          | telux_allow_power_control_state_t        | 
+--------------+------------------------------------+------------------------------------------+
| Sensor       | TELUX_SENSOR_DATA_READ             | telux_allow_sensor_data_read             |
|              +------------------------------------+------------------------------------------+
|              | TELUX_SENSOR_PRIVILEGED_OPS        | telux_allow_sensor_privileged_ops        |
|              +------------------------------------+------------------------------------------+
|              | TELUX_SENSOR_FEATURE_CONTROL       | telux_allow_sensor_feature_control       | 
+--------------+------------------------------------+------------------------------------------+
| Telephony    | TELUX_TEL_CARD_POWER               | telux_allow_tel_card_power               |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_CARD_OPS                 | telux_allow_tel_card_ops                 |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_PRIVATE_INFO_READ        | telux_allow_tel_private_info_read        |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_CARD_PRIVILEGED_OPS      | telux_allow_tel_card_privileged_ops      |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SAP                      | telux_allow_tel_sap                      |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_CELL_BROADCAST_CONFIG    | telux_allow_tel_cell_broadcast_config    |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_CELL_BROADCAST_LISTEN    | telux_allow_tel_cell_broadcast_listen    |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SIM_PROFILE_OPS          | telux_allow_tel_sim_profile_ops          |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SIM_PROFILE_USER_CONSENT | telux_allow_tel_sim_profile_user_consent |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SIM_PROFILE_CONFIG       | telux_allow_tel_sim_profile_config       |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SIM_PROFILE_READ         | telux_allow_tel_sim_profile_read         |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SIM_PROFILE_HTTP_PROXY   | telux_allow_tel_sim_profile_http_proxy   |
|              +------------------------------------+------------------------------------------+ 
|              | TELUX_TEL_REMOTE_SIM               | telux_allow_tel_remote_sim               |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SMS_OPS                  | telux_allow_tel_sms_ops                  |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SMS_LISTEN               | telux_access_tel_sms_listen_t            |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SMS_CONFIG               | telux_access_tel_sms_config              | 
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_IMS_SETTINGS             | telux_allow_tel_ims_settings             |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SRV_SYSTEM_CONFIG        | telux_allow_tel_srv_system_config        |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SRV_SYSTEM_READ          | telux_allow_tel_srv_system_read          |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_NETWORK_SELECTION_OPS    | telux_allow_tel_network_selection_ops    |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_NETWORK_SELECTION_READ   | telux_allow_tel_network_selection_read   |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_MULTISIM_MGMT            | telux_allow_tel_multisim_mgmt            |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_PRIVATE_INFO_READ        | telux_allow_tel_private_info_read        |
|              +------------------------------------+------------------------------------------+ 
|              | TELUX_TEL_SUB_PRIVATE_INFO         | telux_allow_tel_sub_private_info_read    |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_SUBSCRIPTION_READ        | telux_allow_tel_subscription_read        |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_CALL_INFO_READ           | telux_allow_tel_call_info_read           |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_CALL_MGMT                | telux_allow_tel_call_mgmt                |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_CALL_PRIVATE_INFO        | telux_allow_tel_call_private_info        |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_EMERGENCY_OPS            | telux_allow_tel_emergency_ops            | 
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_ECALL_MGMT               | telux_allow_tel_ecall_mgmt               |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_PHONE_MGMT               | telux_allow_tel_phone_mgmt               |
|              +------------------------------------+------------------------------------------+    
|              | TELUX_TEL_PRIVATE_INFO_READ        | telux_access_tel_private_info_read       |
|              +------------------------------------+------------------------------------------+
|              | TELUX_TEL_PHONE_CONFIG             | telux_allow_tel_phone_config             |
|              +------------------------------------+------------------------------------------+ 
|              | TELUX_TEL_ECALL_CONFIG             | telux_allow_tel_ecall_config             |
|              +------------------------------------+------------------------------------------+ 
|              | TELUX_TEL_SUPP_SERVICES            | telux_allow_tel_supp_services            |
+--------------+------------------------------------+------------------------------------------+
| Satcom       | TELUX_NTN_CONFIG                   | telux_access_ntn_config_t                |
|              +------------------------------------+------------------------------------------+
|              | TELUX_NTN_DATA                     | telux_access_ntn_data_t                  |
+--------------+------------------------------------+------------------------------------------+


The following example illustrates how to declare permissions for an application that wants to use :cpp:func:`telux::data::IDataConnectionManager::startDataCall` to setup a cellular data connection.The documentation of this API indicates
that the caller needs to have TELUX_DATA_CALL_OPS permission. From the above table, the permission maps to **telux_allow_data_call_ops** SELinux interface:

In order for the app to use the API the below code snippet needs to be entered in the Type Enforcement (TE) file of the application.

.. code-block:: cpp

   policy_module(application, 1.0)
   type app_t;
   
   #Allow data call operations
   
   telux_allow_data_call_ops(app_t)
