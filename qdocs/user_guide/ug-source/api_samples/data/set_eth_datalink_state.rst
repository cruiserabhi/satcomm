..
   *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _set-eth-datalink-state:

Set Ethernet Datalink State
==================================================================

This sample application demonstrates how to set the Ethernet datalink state.

1. Implement the IDataLinkListener class to receive SSR events and Ethernet data link state changes.

.. code-block::

   class DataLinkListener : public telux::data::IDataLinkListener {
   public:
       void onServiceStatusChange(telux::common::ServiceStatus status) {
           switch(status) {
               case telux::common::ServiceStatus::SERVICE_AVAILABLE:
                   std::cout << " SERVICE_AVAILABLE";
                   break;
               case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
                   std::cout << " SERVICE_UNAVAILABLE";
                   break;
               default:
                   std::cout << " Unknown service status";
                   break;
           }
       }

       void onEthDataLinkStateChange(telux::data::LinkState linkState) {
           switch(linkState) {
               case telux::data::LinkState::UP:
                   std::cout << "Ethernet datalink state is UP";
                   break;
               case telux::data::LinkState::DOWN:
                   std::cout << "Ethernet datalink state is DOWN";
                   break;
               default:
                   std::cout << " Unknown Ethernet datalink state";
                   break;
           }
       }
   };

2. Optionally, create a callback to obtain the status of the initialization of the data link
   manager.

.. code-block::

   std::promise<telux::common::ServiceStatus> p{};
   auto dataLinkMgrIntCb = [&p](telux::common::ServiceStatus status) {
        p.set_value(status);
   };

3. Get the data factory and data link manager (as described below) and wait until the service is
   available.

.. code-block::

   auto &dataFactory = telux::data::DataFactory::getInstance();

   dataLinkMgr_  = dataFactory.getDataLinkManager(dataLinkMgrIntCb);
   serviceStatus = p1.get_future().get();
   std::cout << "Initialization complete for data link manager" << std::endl;

4. Register a data link listener to receive SSR events and Ethernet data link state changes.

.. code-block::

   status = dataLinkMgr_->registerListener(dataLinkListener);
   if (status != telux::common::Status::SUCCESS) {
       std::cout << "Can't register listener, err " <<
           static_cast<int>(status) << std::endl;
   }

5. Bring up the Ethernet datalink state.

.. code-block::

   telux::data::LinkState linkState = telux::data::LinkState::UP;

   telux::common::ErrorCode errorCode = dataLinkMgr_->setEthDataLinkState(linkState);

