.. _enable-ip-pass-through:

Enable IP Passthrough
==================================================================

This sample application demonstrates how to enable IP passthrough in NAD-2 and apply a dynamic IP
address to a VLAN in NAD-1.

1. Implement IDataConnectionListener listener class in NAD-2

.. code-block::

   class DataConnectionListener : public telux::data::IDataConnectionListener {
    public:
      DataConnectionListener(SlotId slotId) : slotId_(slotId) {}
      void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
        std::cout << "\nonDataCallInfoChanged()" << std::endl;
        std::list<telux::data::IpAddrInfo> ipAddrList;

        std::cout << "Data call details:" << std::endl;
        std::cout << " Slot ID: " << dataCall->getSlotId() << std::endl;
        std::cout << " Profile ID: " << dataCall->getProfileId() << std::endl;
        std::cout << " Interface name: " << dataCall->getInterfaceName() << std::endl;

        std::cout << " Data call status: " <<
            static_cast<int>(dataCall->getDataCallStatus()) << std::endl;
        std::cout << " Data call end reason, type : " <<
            static_cast<int>(dataCall->getDataCallEndReason().type) << std::endl;

        ipAddrList = dataCall->getIpAddressInfo();
        for(auto &it : ipAddrList) {
            std::cout << "\n ifAddress: " << it.ifAddress
                << "\n ifMask: " << it.ifMask
                << "\n gwAddress: " << it.gwAddress
                << "\n ifMask: " << it.ifMask
                << "\n primaryDnsAddress: " << it.primaryDnsAddress
                << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << std::endl;
        }

        std::cout << " IP family type: " <<
            static_cast<int>(dataCall->getIpFamilyType()) << std::endl;
        std::cout << " Tech preference: " <<
            static_cast<int>(dataCall->getTechPreference()) << std::endl;
      }
    private:
     SlotId slotId_;
   };

2. Optionally, create a callback to obtain the status of the initialization of the following
   managers.

 NAD-1 and NAD-2: Create init callback for the data settings manager and vlan manager.

.. code-block::

   std::promise<telux::common::ServiceStatus> p1{};
   auto dataSettingsMgrIntCb = [&p1](telux::common::ServiceStatus status) {
        p1.set_value(status);
   };

   std::promise<telux::common::ServiceStatus> p2{};
   auto vlanMgrIntCb = [&p2](telux::common::ServiceStatus status) {
        p2.set_value(status);
   };

NAD-2: Create Init callback for the data connection manager.

.. code-block::

   std::promise<telux::common::ServiceStatus> p3{};
   auto dataConnectionMgrIntCb = [&p3](telux::common::ServiceStatus status) {
        p3.set_value(status);
   };

3. Get the data factory and following managers (as described below)

 NAD-1 and NAD-2: Get the  data settings manager, vlan manager and wait until the service is
 available.

.. code-block::


   auto &dataFactory = telux::data::DataFactory::getInstance();

   dataSettingsMgr_  = dataFactory.getDataSettingsManager(opType, dataSettingsMgrIntCb);
   serviceStatus = p1.get_future().get();
   std::cout << "Initialization complete for data settings manager" << std::endl;

   dataVlanMgr_ = dataFactory.getVlanManager(opType, vlanMgrIntCb);
   serviceStatus = p2.get_future().get();
   std::cout << "Initialization complete for vlan manager" << std::endl;

NAD-2: Get the data connection manager and wait until the service is available.

.. code-block::

   auto &dataFactory = telux::data::DataFactory::getInstance();

   dataConMgr_  = dataFactory.getDataConnectionManager(slotId, dataConnectionMgrIntCb);
   serviceStatus = p3.get_future().get();
   std::cout << "Initialization complete for dataconnection manager" << std::endl;

4. Register for a data connection listener in NAD-2 to receive the data call status change event.

.. code-block::

   status = dataConMgr_->registerListener(dataConnectionListener);
   if (status != telux::common::Status::SUCCESS) {
       std::cout << "Can't register listener, err " <<
           static_cast<int>(status) << std::endl;
   }

5. Create VLAN for different network types.

 NAD-1: Create VLAN for LAN and WAN network type

.. code-block::

   telux::data::VlanConfig nad1LanVlanConfig;
   nad1LanVlanConfig.iface = telux::data::InterfaceType::ETH;
   nad1LanVlanConfig_.vlanId = vlanId;
   nad1LanVlanConfig_.priority = priority;
   nad1LanVlanConfig_.isAccelerated = isAccelerated;
   nad1LanVlanConfig_.createBridge  = true;
   nad1LanVlanConfig_.nwType  = telux::data::NetworkType::LAN;

   dataVlanMgr_->createVlan(nad1LanVlanConfig, respCb);

   telux::data::VlanConfig nad1WanVlanConfig;
   nad1WanVlanConfig.iface = telux::data::InterfaceType::ETH;
   nad1WanVlanConfig_.vlanId = vlanId;
   nad1WanVlanConfig_.priority = priority;
   nad1WanVlanConfig_.isAccelerated = isAccelerated;
   nad1WanVlanConfig_.createBridge  = false;
   nad1WanVlanConfig_.nwType  = telux::data::NetworkType::WAN;

   dataVlanMgr_->createVlan(nad1WanVlanConfig, respCb);


NAD-2: Create VLAN for LAN network type

.. code-block::

   telux::data::VlanConfig nad2LanVlanConfig;
   nad2LanVlanConfig.iface = telux::data::InterfaceType::ETH;
   nad2LanVlanConfig_.vlanId = vlanId;
   nad2LanVlanConfig_.priority = priority;
   nad2LanVlanConfig_.isAccelerated = isAccelerated;
   nad2LanVlanConfig_.createBridge  = true;
   nad2LanVlanConfig_.nwType  = telux::data::NetworkType::LAN;

   dataVlanMgr_->createVlan(nad2LanVlanConfig, respCb);

6. Bind VLAN with ETH backhaul in NAD-1.

.. code-block::

   telux::data::net::VlanBindConfig nad1VlanBindConfig;
   nad1VlanBindConfig.vlanId = nad1LanVlanConfig.vlanId;
   nad1VlanBindConfig.bhInfo.backhaul = telux::data::BackhaulType::ETH;
   nad1VlanBindConfig.bhInfo.vlanId = nad1WanVlanConfig.vlanId;

   dataVlanMgr_->bindToBackhaul(nad1VlanBindConfig, respCb);

7. Start data call in NAD-2.

 When the data is connected, obtain a reference to the data call that was brought up. This
 information would be sent to the listener (as shown in step-1) and the IP address information
 can be used to set STATIC IP address configuration in NAD-1.

.. code-block::

   {
     std::promise<telux::common::ErrorCode> p;
     telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;
     dataConnMgr->startDataCall(
            profileId, ipFamilyType,
            [&](const std::shared_ptr<telux::data::IDataCall> &dataCall,
                telux::common::ErrorCode errorCode) {
                std::cout << "startCallResponse: errorCode: " << static_cast<int>(errorCode)
                          << std::endl;
                p.set_value(errorCode);
            },
            opType);

     telux::common::ErrorCode errorCode = p.get_future().get();
     if (errorCode != telux::common::ErrorCode::SUCCESS) {
         std::cerr << "Failed to start data call. Exiting..." << std::endl;
         exit(1);
     }
    }

8. Bind VLAN with WWAN backhaul in NAD-2.

.. code-block::

  telux::data::net::VlanBindConfig nad2VlanBindConfig;
  nad2VlanBindConfig.bhInfo.backhaul = telux::data::BackhaulType::WWAN;
  nad2VlanBindConfig.vlanId = nad2LanVlanConfig_.vlanId;
  nad2VlanBindConfig.bhInfo.profileId = profileId;
  nad2VlanBindConfig.bhInfo.slotId = slotId;

  dataVlanMgr_->bindToBackhaul(nad2VlanBindConfig, respCb);

9. Enable IP passthrough configuration in NAD-2.

.. code-block::

   telux::data::IpptParams ipptParams;
   telux::data::IpptConfig ipptConfig;
   ipptParams.profileId                = profileId;
   ipptParams.vlanId                   = nad2LanVlanConfig.vlanId;
   ipptParams.slotId                   = slotId;
   ipptConfig.ipptOpr                  = telux::data::Operation::ENABLE;
   ipptConfig.devConfig.nwInterface    = telux::data::InterfaceType::ETH;
   ipptConfig.devConfig.macAddr        = deviceMacAddr;

   dataSettingsMgr_->setIpPassThroughConfig(ipptParams, ipptConfig);

10. Set IP configuration to VLAN in NAD-1.

.. code-block::

   ipConfigParams_.ifType       = telux::data::InterfaceType::ETH;
   ipConfigParams_.ipFamilyType = telux::data::IpFamilyType::IPV4;
   ipConfigParams_.vlanId = nad1WanVlanConfig.vlanId;
   /* In case of IpAssignType is STATIC_IP, user must give data call IP address which is
    * running in NAD-2 (received as part of step-1 indication)
    */
   ipConfig_.ipType = telux::data::IpAssignType::DYNAMIC_IP;
   ipConfig_.ipOpr = telux::data::IpAssignOperation::ENABLE;
   dataSettingsMgr_->setIpConfig(ipConfigParams, ipConfig);

