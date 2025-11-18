.. _ntn:

How to enable NTN and send non-IP data
======================================


The following steps illustrate how to configure and use NTN.

1. Create the INtnListener class.

.. code-block::

    class NtnListener: public telux::satcom::INtnListener {
    public:
    void onNtnStateChange(NtnState newState) {
        // Handle NTN state change
    }

    void onCapabilitiesChange(NtnCapabilities capabilities) {
        // Handle NTN capabilities change
    }

    void onSignalStrengthChange(SignalStrength signalStrength) {
        // Handle signal strength change
    }

    onServiceStatusChange(telux::common::ServiceStatus status) {
        // Handle service status change
    }

    void onDataAck(telux::common::ErrorCode err, TransactionId id) {
        // Handle data acks for sent data packets
    }

    void onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) {
        // Process downlink data coming from NTN network
    }
    };


2. Create the initialization callback object to be notified when initialization is complete and the object is ready to be used.

.. code-block::

    auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };

3. Get the SatcomFactory and NtnMAnager instance.

.. code-block::

    auto &satcomFactory = telux::satcom::SatcomFactory::getInstance();
    ntnMgr = satcomFactory.getNtnManager(initCb);

4. Wait for the object to complete initialization.

.. code-block::

    telux::common::ServiceStatus = subSystemStatus = initPromise.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** Satcom SubSystem is Ready *** " << std::endl;
    }
    else {
        std::cout << " *** Unable to initialize Satcom subsystem *** " << std::endl;
    }


5. Register listener.

.. code-block::

   status = ntnMgr->registerListener(listener);


6. Update system selection specifier list (optional).

.. code-block::

   std::vector<SystemSelectionSpecifier> params;
   // Fill params as per vendor specification.
   err = ntnMgr->updateSystemSelectionSpecifiers(params);


7. Enable NTN.

.. code-block::

   err = ntnMgr_->enableNtn(enable, isEmergency, iccid);


8. Get NTN capabilities.

.. code-block::

    err = ntnMgr_->getNtnCapabilities(cap);


9. Send data.

.. code-block::

    status = ntnMgr_->sendData(data, size, tid);

