.. _add-enable-software-bridge:

Adding a software bridge and enable its management 
=================================================================================

This sample application demonstrates how to add a software bridge and enable its management.

1. Implement initialization callback Get the DataFactory instances

Optionally initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

.. code-block::

   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };

   auto &dataFactory = telux::data::DataFactory::getInstance();


2. Get the BridgeManager instances

.. code-block::

    std::unique_lock<std::mutex> lck(mtx);
    auto dataBridgeMgr  = dataFactory.getBridgeManager(initCb);


3. Wait for BridgeManager initialization to be complete

.. code-block::

   initCv.wait(lck);


3.1 Check BridgeManager initialization state

BridgeManager needs to be ready to remove a software bridge and disable the software bridge
management in the system. If BridgeManager initialization failed, new initialization attempt can
be accomplished by calling step 2. If BridgeManager initialization succeed, proceed to step 4.

.. code-block::

   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }


4. Implement callbacks for adding a software bridge enabling its management

.. code-block::

   auto respCbAdd = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Add software bridge request"
                 << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };
   
   auto respCbEnable = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Enable software bridge management request is"
                 << (error == telux::common::ErrorCode::SUCCESS ? " successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };


5. Add software bridge based on the interface name, interface type and bandwidth required

.. code-block::

   telux::data::net::BridgeInfo config;
   config.ifaceName = infName;
   config.ifaceType = infType;
   config.bandwidth = bridgeBw;
   dataBridgeMgr->addBridge(config, respCbAdd);

Now, response callback will be invoked with the addBridge response.

6. Enable the software bridge management if not enabled already

.. code-block::

   bool enable = true;
   dataBridgeMgr->enableBridge(enable, respCbEnable);

Now, response callback will be invoked with the enableBridge response.
