Public Logging API {#public_logging}
====================================

# Public Logging API

This Section demonstrates how to use the Public Logging API for SDK Applications.

### 1. Including the Header file

   ~~~~~~{.cpp}
   #include <telux/common/Log.hpp>
   ~~~~~~

### 2. Calling the Logging API

   ~~~~~~{.cpp}
   LOG(DEBUG, "startCallResponse: errorCode: ", static_cast<int>(errorCode));
   ~~~~~~