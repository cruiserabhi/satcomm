.. _wlan-config:

How to configure and enable WLAN
===================================


The following steps illustrate how to configure WLAN.

1. Create the IWlanListener class.

.. code-block::

    class NotificationListener: public telux::wlan::IWlanListener {
    public:
    void onEnableChanged(bool enable) {
        promise_.set_value(enable);
    }

    bool getEnableStatus() {
        return promise_.get_future().get();
    }

    void resetPromise() {
        promise_ = std::promise<bool>();
    }
    private:
    std::promise<bool> promise_;
    };


2. Create the initialization callback object to be notified when initialization is complete and the object is ready to be used.

.. code-block::

    auto initCb = [&](telux::common::ServiceStatus status) {
        initPromise.set_value(status);
    };


3. Get the WlanFactory and WlanDeviceManager instances.

.. code-block::

    auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
    wlanDevMgr  = wlanFactory.getWlanDeviceManager(initCb);


4. Wait for the object to complete initialization.

.. code-block::

    telux::common::ServiceStatus = subSystemStatus = initPromise.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** Wlan SubSystem is Ready *** " << std::endl;
    }
    else {
        std::cout << " *** Unable to initialize Wlan subsystem *** " << std::endl;
    }


5. If WLAN is enabled, disable it.

.. code-block::

    bool enableStat = false;
    std::vector<telux::wlan::InterfaceStatus> ifStatus;
    if(telux::common::ErrorCode::SUCCESS != wlanDevMgr->getStatus(enableStat, ifStatus)) {
        std::cout << "Failed to retrieve Wlan status" <<std::endl;
    }
    if(enableStat) {
        //Disable Wlan before changing configuration...
        wlanDevMgr->registerListener(listener);
        //Ensure callback returns SUCCESS and indication for Wlan disablement is received
        if((telux::common::ErrorCode::SUCCESS != wlanDevMgr->enable(false)) ||
            (true == listener->getEnableStatus())) {
            std::cout << "Failed to disable Wlan " <<std::endl;
            break;
        }
    }


6. Set the desired WLAN configurations.

.. code-block::

    if(telux::common::ErrorCode::SUCCESS != wlanDevMgr->setMode(numAp, numSta)) {
        std::cout << "Failed to set Wlan configuration" <<std::endl;
    }


7. Enable WLAN for new configurations to take effect

.. code-block::

    listener->resetPromise();
    //Ensure callback returns SUCCESS and indication for Wlan enablement is received
    if((telux::common::ErrorCode::SUCCESS != wlanDevMgr->enable(true)) ||
       (false == listener->getEnableStatus())) {
        std::cout << "Failed to enable Wlan " <<std::endl;
    }

