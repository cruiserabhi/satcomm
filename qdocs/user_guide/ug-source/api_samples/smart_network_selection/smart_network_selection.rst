..
   *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _smart-network-selection:

Smart network selection
===================================================

This sample application demonstrates how to set dubious cells for LTE and NR networks and perform a
smart network selection.

1. Get phone factory and network selection manager instances

.. code-block::

   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom;
   auto networkMgr
      = phoneFactory.getNetworkSelectionManager(DEFAULT_SLOT_ID),
      [&](telux::common::ServiceStatus status) {
           prom.set_value(status);
   });


2. Wait for the network selection subsystem initialization

.. code-block::

   telux::common::ServiceStatus networkSelMgrStatus = prom.get_future().get();

3. Exit the application if the network selection subsystem cannot be initialized

.. code-block::

   if (networkSelMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Network Selection Manager is ready " << "\n";
   } else {
       std::cout << "ERROR - Unable to initialize,"
           << " network selection manager subsystem " << std::endl;
       return 1;
   }


4. Set dubious cell for the LTE netowrk

.. code-block::

   LteDubiousCellInfo params;
   DubiousCellInfo dbCellInfo;
   dbCellInfo.mcc = mcc;
   dbCellInfo.mnc = mnc;
   dbCellInfo.arfcn = arfcn;
   dbCellInfo.pci = pci;
   dbCellInfo.activeBand = activeBand;
   dbCellInfo.causeCodeMask = causeCodeMask;
   params.ciList.emplace_back(dbCellInfo);
   params.cgi = cgi;

   if(networkMgr) {
      ErrorCode err = networkMgr->setLteDubiousCell(params);
      std::cout << "ErrorCode: " << static_cast<int>(err) <<std::endl;
      }
   }

5. Set dubious cell for the NR netowrk

.. code-block::

   NrDubiousCellInfo params;
   DubiousCellInfo dbCellInfo;
   dbCellInfo.mcc = mcc;
   dbCellInfo.mnc = mnc;
   dbCellInfo.arfcn = arfcn;
   dbCellInfo.pci = pci;
   dbCellInfo.activeBand = activeBand;
   dbCellInfo.causeCodeMask = causeCodeMask;
   params.ciList.emplace_back(dbCellInfo);
   params.cgi = cgi;
   params.spacing = spacing;

   if(networkMgr) {
      ErrorCode err = networkMgr->setNrDubiousCell(params);
      std::cout << "ErrorCode: " << static_cast<int>(err) <<std::endl;
      }
   }
