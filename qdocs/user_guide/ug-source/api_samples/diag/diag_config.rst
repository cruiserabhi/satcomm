.. _diag-config:

How to configure, start and stop logging 
===================================================

This sample application demonstrates how to configure, start and stop diagnostics logging.

1. Create the initialization callback object to be notified when initialization is complete and the object is ready to be used.

.. code-block::

     auto initCb = [&](telux::common::ServiceStatus status) {
        initPromise.set_value(status);
     };


2. Get the DiagnosticsFactory and DiagLogManager instances.

.. code-block::

     auto &diagFactory = telux::platform::diag::DiagnosticsFactory::getInstance();
     diagMgr  = diagFactory.getDiagLogManager(initCb);


3. Ensure subsystem is ready.

.. code-block::

     subSystemStatus = initPromise.get_future().get();


4. Ensure logging is not currently in progress.

.. code-block::

     if(diagStatus.isLoggingInProgress) {
        std::cout << " *** Logging already in progress - quitting application *** " << std::endl;
        break;
     }


5. Configure diagnostics logging.

.. code-block::

     telux::platform::diag::DiagConfig fileMethodCfg {};
     fileMethodCfg.method = telux::platform::diag::LogMethod::FILE;
     fileMethodCfg.srcType = telux::platform::diag::SourceType::DEVICE;
     fileMethodCfg.srcInfo.device = static_cast<uint8_t>(2);
     fileMethodCfg.mdmLogMaskFile = maskFile;
     fileMethodCfg.modeType = telux::platform::diag::DiagLogMode::STREAMING;
     fileMethodCfg.methodConfig.fileConfig.maxSize = 0;
     fileMethodCfg.methodConfig.fileConfig.maxNumber = 0;
     errCode = diagMgr->setConfig(fileMethodCfg);


6. Start logging.

.. code-block::

     errCode = diagMgr->startLogCollection();
     if(telux::common::ErrorCode::SUCCESS != errCode) {
        std::cout << "Failed to start logging... Error Code: "
                  << static_cast<int>(errCode) << ". quitting application" <<std::endl;
        break;
     }


7. Wait for 10 seconds.

.. code-block::

     std::this_thread::sleep_for(std::chrono::milliseconds(10000));


8. Stop logging.

.. code-block::

     errCode = diagMgr->stopLogCollection();
     if(telux::common::ErrorCode::SUCCESS != errCode) {
        std::cout << "Failed to stop logging... Error Code: "
                  << static_cast<int>(errCode) << ". quitting application" <<std::endl;
        break;
     }


9. Cleanup.

.. code-block::

     if(diagMgr) {
        diagMgr = nullptr;
     }

