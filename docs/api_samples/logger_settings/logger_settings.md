TelSDK logging {#logger_settings}
=================================

TelSDK provides a configurable logging facility that can be used by application to log messages. This also make it easier for application to include it's messages along with messages from TelSDK itself.

### 1. Logging API

   Use the LOG() API to log any message.

   ~~~~~~{.cpp}
   #include <telux/common/Log.hpp>
   
   LOG(DEBUG, "startCallResponse: errorCode: ", static_cast<int>(errorCode));
   ~~~~~~

### 2. Logging configuration

TelSDK has a default configuration for logging which defines behaviour of logging. To override this configuration, a configuration file should be provided to the TelSDK. This file is searched by TelSDK logging mechanism in the following order.
-  app_name.conf in /etc directory (for example, /etc/telsdk_console_app.conf)
-  app_name.conf in the directory that contains the application
-  tel.conf in /etc
-  tel.conf in the directory that contains the application

Advantage of using different configuration files by applications is, it allows flexibility to either share the same log file or log in separate files. In QC provided builds, /etc/tel.conf has been used as an example configuration by default for all applications.

###### Log file name

LOG_FILE_NAME specifies name of the log file.

   ~~~~~~{.cpp}
   LOG_FILE_NAME=tel.log
   ~~~~~~
   
###### Log file path

LOG_FILE_PATH specifies an absolute writable directory where log file will be created. Additionally, the application must be part of Linux "system" group to access /data/vendor/telsdk directory.

   ~~~~~~{.cpp}
   LOG_FILE_PATH=/data/vendor/telsdk
   ~~~~~~

###### Log destination

The log messages can be routed to device's console, DIAG and file. This is specified when defining logging level.

   ~~~~~~{.cpp}
   # NONE means do not print on console
   
   CONSOLE_LOG_LEVEL=NONE
   ~~~~~~

###### Log levels



###### Log file size

MAX_LOG_FILE_SIZE specifies the maximum allowed size(in bytes) of the log file. When the maximum limit is reached, it is saved as tel.log.backup.

   ~~~~~~{.cpp}
   MAX_LOG_FILE_SIZE=5242880
   ~~~~~~

###### Adding date and time

LOG_PREFIX_DATE_TIME specifies whether date and time should be prefixed to log message or not.

   ~~~~~~{.cpp}
   # FALSE - logs with filename and line number, this is default option
   # TRUE - logs with date, time, filename and line number
   
   LOG_PREFIX_DATE_TIME=TRUE
   ~~~~~~
   
###### Log filtering

Logs can be emitted selectively based on technology domain by specifying TELUX_LOG_COMPONENT_FILTER.

   ~~~~~~{.cpp}
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
   ~~~~~~