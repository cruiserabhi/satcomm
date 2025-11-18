.. _add-firewall:

Add Firewall Entry 
==================================

Please follow below steps to create and add Firewall Entry

1. Implement initialization callback and get the DataFactory instances

Optionally initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

.. code-block::

   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };
   auto &dataFactory = telux::data::DataFactory::getInstance();


2. Get the FirewallManager instances

.. code-block::

   std::unique_lock<std::mutex> lck(mtx);
   auto dataFwMgr  = dataFactory.getFirewallManager(opType, initCb);


3. Wait for FirewallManager initialization to be complete

.. code-block::

   initCv.wait(lck);


3.1 Check FirewallManager initialization state

If FirewallManager initialization failed, new initialization attempt can be accomplished
by calling step 2. If FirewallManager initialization succeed, proceed to step 4

.. code-block::

   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }


4. Get firewall Entry instance

.. code-block::

   std::shared_ptr<telux::data::net::IFirewallEntry> fwEntry
      = dataFactory.getNewFirewallEntry(proto, fwDir, ipFamType);

      
5. Get pointer to Ip Filter

.. code-block::

   std::shared_ptr<telux::data::IIpFilter> ipFilter = fwEntry->getIProtocolFilter();


6. Populate Ip Filter based on Ip Family type

.. code-block::

   switch (ipFamType) {
      case telux::data::IpFamilyType::IPV4: {
            telux::data::IPv4Info info;
            info.srcAddr = srcAddr;
            info.destAddr = destAddr;
            info.srcSubnetMask = configParser->getValue(std::string("IPV4_SRC_SUBNET_MASK"));
            info.destSubnetMask = configParser->getValue(std::string("IPV4_DEST_SUBNET_MASK"));
            info.value = (uint8_t)std::atoi(
               configParser->getValue(std::string("IPV4_SERVICE_TYPE")).c_str());
            info.mask = (uint8_t)std::atoi(
               configParser->getValue(std::string("IPV4_SERVICE_TYPE_MASK")).c_str());
            info.nextProtoId = proto;
            ipFilter->setIPv4Info(info);
      } break;
      case telux::data::IpFamilyType::IPV6: {
            telux::data::IPv6Info info;
            info.srcAddr = srcAddr;
            info.destAddr = destAddr;
            info.nextProtoId = proto;
            info.val = (uint8_t)std::atoi(
               configParser->getValue(std::string("IPV6_TRAFFIC_CLASS")).c_str());
            info.mask = (uint8_t)std::atoi(
               configParser->getValue(std::string("IPV6_TRAFFIC_CLASS_MASK")).c_str());
            info.flowLabel = (uint32_t)std::atoi(
               configParser->getValue(std::string("IPV6_FLOW_LABEL")).c_str());
            ipFilter->setIPv6Info(info);
      } break;
      default: {
         std::cout <<"Error: Unrecognized Ip Family used .. exiting app" <<std::endl;
         return 1;
      } break;
   }


7. Populate Protocol information

.. code-block::

   switch (proto) {
      case 6: {   // TCP
         telux::data::TcpInfo tcpInfo;
         tcpInfo.src.port = (uint16_t)protSrcPort;
         tcpInfo.src.range = (uint16_t)protSrcRange;
         tcpInfo.dest.port = (uint16_t)protDestPort;
         tcpInfo.dest.range = (uint16_t)protDestRange;
         auto tcpFilter = std::dynamic_pointer_cast<telux::data::ITcpFilter>(ipFilter);
         if(tcpFilter) {
               tcpFilter->setTcpInfo(tcpInfo);
         }
      } break;
      case 17: {  //UDP
         telux::data::UdpInfo info;
         info.src.port = (uint16_t)protSrcPort;
         info.src.range = (uint16_t)protSrcRange;
         info.dest.port = (uint16_t)protDestPort;
         info.dest.range = (uint16_t)protDestRange;
         auto udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(ipFilter);
         if(udpFilter) {
               udpFilter->setUdpInfo(info);
         }
      } break;
      default: {
      } break;
   }


8. Instantiate add firewall entry callback instance - this is optional

.. code-block::

   auto respCb = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
               << "addFirewallEntry Response"
               << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
               << ". ErrorCode: " << static_cast<int>(error) << std::endl;
               promise.set_value(1);
   };

   std::future<int> future = promise.get_future();
   dataFwMgr->addFirewallEntry(profileId, fwEntry, respCb);

