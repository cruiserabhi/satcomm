.. #=============================================================================
   #
   #  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   #  SPDX-License-Identifier: BSD-3-Clause-Clear
   #
   #=============================================================================
   
===============
TelSDK logging
===============

TelSDK provides a configurable logging facility that can be used by application to log messages. This also makes it easier for application to include its messages along with messages from TelSDK library itself.


------------
Logging API
------------

Use the LOG() API to log any message.

.. code-block::

  #include <telux/common/Log.hpp>

  LOG(DEBUG, "startCallResponse: errorCode: ", static_cast<int>(errorCode));

  
----------------------
Logging configuration
----------------------

TelSDK has a default configuration for logging which defines behaviour of logging. To override this configuration, a configuration file should be provided to the TelSDK. This file is searched by TelSDK logging mechanism in the following order.

* app_name.conf in /etc directory (for example, /etc/telsdk_console_app.conf)
* app_name.conf in the directory that contains the application
* tel.conf in /etc
* tel.conf in the directory that contains the application

Advantage of using different configuration files by applications is, it allows flexibility to either share the same log file or log in separate files. In QC provided builds, /etc/tel.conf has been used as an example configuration by default for all applications.


File name
~~~~~~~~~

Specify name of the log file if log messages are printed on file.

.. code-block::
   
   LOG_FILE_NAME=tel.log

   
File path
~~~~~~~~~

Specify an absolute writable directory where log file(s) will be created. The application must be part of Linux "system" group to access /data/vendor/telsdk directory.

.. code-block::

  LOG_FILE_PATH=/data/vendor/telsdk


Destination
~~~~~~~~~~~

The log messages can be routed to device's console, DIAG and file. This is specified when defining logging level.

.. code-block::

  # NONE means do not print on console

  CONSOLE_LOG_LEVEL=NONE

  
Levels
~~~~~~~

Specifies threshold for emitting messages. The messages which are above this threshold appears on log destination.

**Console and File logging**

.. code-block::

  # NONE - No logging
  # PERF - Prints messages with nanoseconds precision timestamp
  # ERROR - Very minimal logging, prints perf and error messages only
  # WARNING - Prints perf, error and warning messages
  # INFO - Prints perf, errors, warning and information messages
  # DEBUG - Full logging including debug messages

  CONSOLE_LOG_LEVEL=ERROR
  FILE_LOG_LEVEL=DEBUG

**DIAG logging**

The log messages appears in QXDM tool when DIAG is used as destination. The DIAG defines its own log levels and the mapping of them with TelSDK is given below.

.. code-block::

  # SDK Log Levels --> QXDM LOG Levels
  # PERF           --> FATAL (MSG_LEGACY_FATAL)
  # ERROR          --> ERROR (MSG_LEGACY_ERROR)
  # WARNING        --> HIGH (MSG_LEGACY_HIGH)
  # INFO           --> MED (MSG_LEGACY_MED)
  # DEBUG          --> LOW (MSG_LEGACY_LOW)

  DIAG_LOG_LEVEL=ERROR

  
File size
~~~~~~~~~~

Specify the maximum allowed size (in bytes) of the log file. When the maximum limit is reached, it is saved as tel.log.backup.

.. code-block::

  MAX_LOG_FILE_SIZE=5242880


Adding date and time
~~~~~~~~~~~~~~~~~~~~~

Specify whether date and time should be prefixed to log message or not.

.. code-block::

  # FALSE - logs with filename and line number, this is default option
  # TRUE - logs with date, time, filename and line number

  LOG_PREFIX_DATE_TIME=TRUE


Filtering
~~~~~~~~~~

Logs can be emitted selectively based on the technology domain by specifying TELUX_LOG_COMPONENT_FILTER.

.. code-block::

  # 0 - All logs are printed
  # 1 - Audio logs are printed
  # 2 - CV2X logs are printed
  # 3 - Data logs are printed
  # 4 - Location logs are printed
  # 5 - Power logs are printed
  # 6 - Telephony logs are printed
  # 7 - Thermal logs are printed

  # For logging all component
  # use TELUX_LOG_COMPONENT_FILTER= 0

  # For logging more than one component like cv2x and audio (comma separated)
  # use TELUX_LOG_COMPONENT_FILTER= 2,1
