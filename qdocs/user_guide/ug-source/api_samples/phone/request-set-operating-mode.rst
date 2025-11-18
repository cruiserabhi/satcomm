.. #=============================================================================
   #
   #  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   #  SPDX-License-Identifier: BSD-3-Clause-Clear
   #
   #=============================================================================

.. _request-set-operating-mode: 

===============================
Request and set operating mode
===============================

This sample application demonstrates how to request and set Operating mode of device.

**1. Implement ResponseCallback interface to receive subsystem initialization status**

.. code-block::

  std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
  void initResponseCb(telux::common::ServiceStatus status) {
     if(subSystemsStatus == SERVICE_AVAILABLE) {
        std::cout << Phone Manager subsystem is ready << std::endl;
     } else if(subSystemsStatus == SERVICE_FAILED) {
        std::cout << Phone Manager subsystem initialization failed << std::endl;
     }
     cbProm.set_value(status);
  }

**2. Get the PhoneFactory and PhoneManager instance**

.. code-block::

  auto &phoneFactory = PhoneFactory::getInstance();
  auto phoneManager = phoneFactory.getPhoneManager(initResponseCb);
  if(phoneManager == NULL) {
     std::cout << " Failed to get Phone Manager instance" << std::endl;
     return -1;
  }

**3. Wait for Phone Manager subsystem to be ready**

.. code-block::

  telux::common::ServiceStatus status = cbProm.get_future().get();
  if(status != SERVICE_AVAILABLE) {
     std::cout << Unable to initialize Phone Manager subsystem << std::endl;
     return -1;
  }

**4. Implement IPhoneListener interface to receive service state change notifications**

.. code-block::

  class MyPhoneListener : public telux::tel::IPhoneListener {
  public:
     void onRadioStateChanged(int phoneId, telux::tel::RadioState radiostate) {
     }
     ~MyPhoneListener() {
     }
  }

**4.1 Instantiate MyPhoneListener**

.. code-block::

  auto myPhoneListener = std::make_shared<MyPhoneListener>();

**5. Register for phone info updates**

.. code-block::

  phoneManager->registerListener(myPhoneListener);

**6. Implement IOperatingModeCallback interface and instantiate MyOperatingModeCallback**

.. code-block::

  std::promise<bool> callbackPromise;
  telux::tel::OperatingMode operatingMode;
  class MyOperatingModeCallback : public telux::tel::IOperatingModeCallback {
  public:
     void operatingModeResponse(OperatingMode mode, telux::common::ErrorCode error) {
        if(error == ErrorCode::SUCCESS) {
           std::cout << "requestOperatingMode response successful" << std::endl;
           std::cout << "Operating Mode: " << mode << std::endl;
           operatingMode = static_cast<telux::tel::OperatingMode>(mode);
        } else {
           std::cout << "requestOperatingMode is failed, errorCode: " << static_cast<int>(error) << std::endl
         ;
        }
     }
     callbackPromise.set_value(true);
  };

  auto myOperatingModeCallback = std::make_shared<MyOperatingModeCallback>();

  void setOperatingModeResponse(telux::common::ErrorCode error) {
     std::cout << "Set Operating Mode is :, errorCode: " << static_cast<int>(error)
        << std::endl;
  }

**7. Request the operating mode of device and set the operating mode to ONLINE, if the operating mode is
OFFLINE to perform any operations on the phone**

.. code-block::

  phoneManager->requestOperatingMode(myOperatingModeCallback);
  if ((callbackPromise.get_future().get()) &&
     (telux::tel::operatingMode == telux::tel::OperatingMode::OFFLINE )) {
      phoneManager->setOperatingMode(telux::tel::OperatingMode::ONLINE, &setOperatingModeResponse);
  }
