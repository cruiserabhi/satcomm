.. #=============================================================================
   #
   #  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   #  SPDX-License-Identifier: BSD-3-Clause-Clear
   #
   #=============================================================================
   
=============
Linux groups
=============

Certain APIs of the SDK require the caller to be part of one or more Linux groups. Given below is a summary of APIs and the corresponding group IDs required by the caller. This mapping of API and group Ids might differ based on the software product family. The table below also lists the applicable software product.

-------
Common
-------

+-------------+---------------------------------------+----------------------------------+
| Linux group | Usage                                 |  Applicable software products    |
+=============+=======================================+==================================+
|  system     |  Accessing telsdk configuration files |  All software products with 4.14 |
|             |  (for ex; /etc/tel.conf)              |  or older Linux kernel           |
+-------------+---------------------------------------+----------------------------------+
|  diag       |  Logging using diag (qxdm logs)       |  All software products with 4.14 |
|             |                                       |  or older Linux kernel           |
+-------------+---------------------------------------+----------------------------------+
|  system     |  Logging on file on file system       |  All software products with 4.14 |
|             |                                       |  or older Linux kernel           |
+-------------+---------------------------------------+----------------------------------+
|  telux      |  Logging on file on file system       |  All software products with 5.4  |
|             |                                       |  or newer Linux kernel           |
+-------------+---------------------------------------+----------------------------------+


---------
Location
---------

+-------------+---------------------------------------+----------------------------------+
| Linux group | Usage                                 |  Applicable software products    |
+=============+=======================================+==================================+
|  locclient  |  Needed by APIs in                    |  All software products           |
|             |  telux::loc::* namespace.             |                                  |
+-------------+---------------------------------------+----------------------------------+


-------
Sensor
-------

+-------------+---------------------------------------------+----------------------------------+
| Linux group | Usage                                       |  Applicable software products    |
+=============+=============================================+==================================+
|  sensors    |  Needed by APIs in:                         | All software products except SPs |
|             |                                             | for SA415M target                |
|             |  telux::sensor::ISensorManager,             |                                  |
|             |                                             |                                  |
|             |  telux::sensor::ISensorFeatureManager,      |                                  |
|             |                                             |                                  |
|             |  telux::sensor::ISensor/telux::sensor::I\   |                                  |
|             |  SensorClient namespace.                    |                                  |
|             |                                             |                                  |
|             |                                             |                                  |
|             |                                             |                                  |
|             |                                             |                                  |
+-------------+---------------------------------------------+----------------------------------+


------
Power
------

+-------------+---------------------------------------+----------------------------------+
| Linux group | Usage                                 |  Applicable software products    |
+=============+=======================================+==================================+
|  net_raw    |  Needed by APIs in                    |  All software products           |
|             |  telux::power::ITcuActivityManager.   |                                  |
+-------------+---------------------------------------+----------------------------------+


-----
Cv2x
-----

+-------------+---------------------------------------+----------------------------------+
| Linux group | Usage                                 |  Applicable software products    |
+=============+=======================================+==================================+
|  radio      |  Required for communication with      |  All software products           |
|             |  modem firmware.                      |                                  |
+-------------+---------------------------------------+----------------------------------+


-----
Data
-----

+-------------+---------------------------------------+----------------------------------+
| Linux group | Usage                                 |  Applicable software products    |
+=============+=======================================+==================================+
|  inet       |  Required for accessing data layers.  |  All software products           |
|             |                                       |                                  |
+-------------+---------------------------------------+----------------------------------+


---------
Platform
---------

+-------------+---------------------------------------+----------------------------------+
| Linux group | Usage                                 |  Applicable software products    |
+=============+=======================================+==================================+
|  system     |  Required for QMI access to fs-hms    |  All software products based on  |
|             |  QMI service (/etc/sec_config)        |  sa415m targets                  |
+-------------+---------------------------------------+----------------------------------+


----------
Telephony
----------

+-------------+------------------------------------------+----------------------------------+
| Linux group | Usage                                    |  Applicable software products    |
+=============+==========================================+==================================+
| net_raw     |  Required by APIs in:                    |  All software products based on  |
| or          |                                          |  mdm9650, mdm9607 and sa415m     |
| radio       |  telux::tel::IPhoneManager,              |  targets                         |
|             |                                          |                                  |
|             |  telux::tel::IPhone,                     |                                  |
|             |                                          |                                  |
|             |  telux::tel::ICallManager,               |                                  |
|             |                                          |                                  |
|             |  telux::tel::ICall,                      |                                  |
|             |                                          |                                  |
|             |  telux::tel::INetworkSelectionManager    |                                  |
|             |                                          |                                  |
|             |  telux::tel::IServingSystemManager       |                                  |
|             |                                          |                                  |
+-------------+------------------------------------------+----------------------------------+
|  radio      |  Required by APIs in                     |  All software products based on  |
|             |                                          |  mdm9650, mdm9607 and sa415m     |
|             |  telux::tel::ISubscriptionManager,       |  targets                         |
|             |                                          |                                  |
|             |  telux::tel::ISubscription,              |                                  |
|             |                                          |                                  |
|             |  telux::tel::ICardManager,               |                                  |
|             |                                          |                                  |
|             |  telux::tel::ICard,                      |                                  |
|             |                                          |                                  |
|             |  telux::tel::ISapCardManager,            |                                  |
|             |                                          |                                  |
|             |  telux::tel::ISimProfileManager,         |                                  |
|             |                                          |                                  |
|             |  telux::tel::ICellBroadcastManager,      |                                  |
|             |                                          |                                  |
|             |  telux::tel::IRemoteSimManager           |                                  |
|             |                                          |                                  |
|             |  telux::tel::IMultiSimManager            |                                  |
|             |                                          |                                  |
+-------------+------------------------------------------+----------------------------------+

---------
Security
---------

+-------------+---------------------------------------+----------------------------------+
| Linux group | Usage                                 |  Applicable software products    |
+=============+=======================================+==================================+
|  mvm        |  Required for accessing crypto        |  All software products           |
|             |  accelerators.                        |                                  |
+-------------+---------------------------------------+----------------------------------+


------
Audio
------

Does not require application to join any particular group.


--------
Thermal
--------

Does not require application to join any particular group.


-------
Config
-------

Does not require application to join any particular group.
