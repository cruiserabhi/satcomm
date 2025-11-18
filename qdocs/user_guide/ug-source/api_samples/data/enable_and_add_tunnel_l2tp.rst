.. _enable-and-add-tunnel-l2tp:

Enable L2TP and add a tunnel
========================================================

This sample application demonstrates how to enable L2TP and add a tunnel.

1.  Implement initialization callback and get get the DataFactory instance

Optionally, initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

.. code-block::

   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };
   
   auto &dataFactory = telux::data::DataFactory::getInstance();


2. Get the L2tpManager instances

.. code-block::

   std::unique_lock<std::mutex> lck(mtx);
   auto dataL2tpMgr  = dataFactory.getL2tpManager(initCb);

3. Wait for L2tpManager initialization to be complete

.. code-block::

   initCv.wait(lck);


3.1 Check L2tpManager initialization state

If L2tpManager initialization failed, new initialization attempt can be accomplished
by calling step 2. If L2tpManager initialization succeed, proceed to step 4

.. code-block::

   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }

4. Optionally, instantiate setConfig callback instance

.. code-block::

   auto setConfigCb = [&setConfigPass, &promise](telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   std::cout << "CALLBACK: "
             << "setConfig Response"
             << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
   };


5. Set L2TP configuration

.. code-block::

   bool enable = true;        //Enable L2TP
   bool enableMss = true;     // Enable MSS Clamping
   bool enableMtu = true;     // Enable custom size MTU
   int mtuSize = 0;           // Set MTU size to default 1422 bytes, otherwise set desired mtu size
   dataL2tpMgr->setConfig(enable, enableMss, enableMtu, setConfigCb, mtuSize);


6. Configure L2TP tunnel and session

.. code-block::

   std::cout << "L2TP Set Configuration succeeded ... Adding Tunnel" << std::endl;
   telux::data::net::L2tpTunnelConfig l2tpTunnelConfig;
   
   l2tpTunnelConfig.locIface = "eth0.1"; //Set interface name to eth0.x where x is vlan id
   l2tpTunnelConfig.prot = static_cast<telux::data::net::L2tpProtocol>(2); //Set protocol to UDP
   l2tpTunnelConfig.locId = 1;  //Set local tunnel id
   l2tpTunnelConfig.peerId = 1; //Set peer tunnel id
   l2tpTunnelConfig.localUdpPort = 500;   //Set local UDP port if UDP protocol is selected above
   l2tpTunnelConfig.peerUdpPort = 100;    //Set peer UDP port if UDP protocol is selected above
   l2tpTunnelConfig.ipType =  static_cast<telux::data::IpFamilyType>(6); //Set Ip family type
   l2tpTunnelConfig.peerIpv6Addr =  "fe80::b044::c0ff::fec4";  // Set peer Ip address
   telux::data::net::L2tpSessionConfig l2tpSessionConfig;
   l2tpSessionConfig.locId = 1;   //Set local session id
   l2tpSessionConfig.peerId = 1;  //Set peer session id
   l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig); // Add session to tunnel config


7. Optionally, instantiate addTunnel callback instance

.. code-block::

   auto addTunnelCb = [&setConfigPass, &promise](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                << "addTunnel Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error) << "\n";
   };


8. Add the tunnel to L2TP

.. code-block::

   dataL2tpMgr->addTunnel(l2tpTunnelConfig, addTunnelCb);
