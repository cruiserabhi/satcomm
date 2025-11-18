..
   *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _set-data-stall-params:

Set Data Stall Params
==================================================================

This sample application demonstrates how to set data stall parameters.

1. Implement IDataControlListener listener class to receive an SSR event.

.. code-block::

   class DataControlListener : public telux::data::IDataControlListener {
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
   };

2. Optionally, create a callback to obtain the status of the initialization of the data control
   manager.

.. code-block::

   std::promise<telux::common::ServiceStatus> p{};
   auto dataControlMgrIntCb = [&p](telux::common::ServiceStatus status) {
        p.set_value(status);
   };

3. Get the data factory and data control manager (as described below) and wait until the service is
   available.

.. code-block::

   auto &dataFactory = telux::data::DataFactory::getInstance();

   dataControlMgr_  = dataFactory.getDataControlManager(dataControlMgrIntCb);
   serviceStatus = p1.get_future().get();
   std::cout << "Initialization complete for data control manager" << std::endl;

4. Register for a data control listener to receive an SSR event.

.. code-block::

   status = dataControlMgr_->registerListener(dataControlListener);
   if (status != telux::common::Status::SUCCESS) {
       std::cout << "Can't register listener, err " <<
           static_cast<int>(status) << std::endl;
   }

5. Set parameters for data stall.

.. code-block::

   params.trafficDir = telux::data::Direction::UPLINK;
   params.appType = telux::data::ApplicationType::CONV_AUDIO;
   params.dataStall = true;

   telux::common::ErrorCode errorCode = dataControlMgr_->setDataStallParams(slotID, params);

