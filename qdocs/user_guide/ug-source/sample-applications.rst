.. #=============================================================================
   #
   #  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   #  SPDX-License-Identifier: BSD-3-Clause-Clear
   #
   #=============================================================================
   
====================
Sample applications
====================

Sample applications provided by TelSDK quickly demonstrates how TelSDK APIs can be used to achieve
end functionality. Full source code for all sample applications is in ``/telux/public/apps/samples`` directory.

**Note:** Some of the applications contain default values for parameters that might differ from product to
product. These values can be overridden by using configuration files used by these applications. The
configuration is specified in the form of key-value pair. For example, the number to dial used by
make_call_app can be specified by DIAL_NUMBER item and then application can be run passing this file
as command-line argument to application.

.. code-block::

  # Contents of /data/make_call_app.conf
  DIAL_NUMBER = +1234512345

  # Run the application
  $ make_call_app /data/make_call_app.conf

.. toctree::
   :maxdepth: 1

   api_samples/audio/audio
   api_samples/cv2x/cv2x
   api_samples/phone/phone
   api_samples/data/data
   api_samples/loc/location
   api_samples/config/modem_config
   api_samples/power/power
   api_samples/thermal/thermal
   api_samples/sensor/sensor
   api_samples/platform/platform
   api_samples/crypto/crypto
   api_samples/crypto_accelerator/crypto_accelerator
   api_samples/cellular_connection_security/cellular_connection_security
   api_samples/wifi_connection_security/wifi_connection_security
   api_samples/wlan/wlan
   api_samples/satcom/satcom
   api_samples/diag/diag

..
   * :ref:`audio`
   * :ref:`cv2x`
   * :ref:`telephony`
   * :ref:`simulation-data`
   * :ref:`location`
   * :ref:`modem-configuration`
   * :ref:`power`
   * :ref:`thermal`
   * :ref:`sensor`
   * :ref:`platform`
   * :ref:`crypto`
   * :ref:`crypto-accelerator`
   * :ref:`cellular-connection-security`
   * :ref:`wifi-connection-security`
   * :ref:`wlan`
   * :ref:`satcom`
   * :ref:`diagnostics`
