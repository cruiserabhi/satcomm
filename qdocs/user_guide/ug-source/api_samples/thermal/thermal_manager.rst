.. _thermal-manager:

Get thermal zones, cooling devices and thermal notifications 
==============================================================

This sample app demonstrates how to get thermal zones and cooling devices.

1. Get thermal factory instance

.. code-block::

   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();


2. Prepare initialization callback

.. code-block::

   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
      std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
      p.set_value(status);
   };


3. Get thermal shutdown manager instance

.. code-block::

   std::shared_ptr<telux::therm::IThermalManager> thermalMgr
      = thermalFactory.getThermalManager(initCb);
   if (!thermalMgr) {
      std::cout << "Thermal manager is nullptr" << std::endl;
      return -1;
   }


4. Wait for the initialization callback and check the service status

.. code-block::

   telux::common::ServiceStatus serviceStatus = p.get_future().get();
   if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Thermal manager initialization failed" << std::endl;
      return -2;
   }

5. Create the listener object

.. code-block::

   std::shared_ptr<ThermalListener> thermalListener
      = std::make_shared<ThermalListener>();


6. To register only trip update notification

  Note: likewise it can be registered for only cooling device changes notification.

.. code-block::

   thermalMgr->registerListener(thermalListener, 1 << TNT_TRIP_UPDATE);


7. Receive service status notification

.. code-block::

   virtual void onServiceStatusChange(ServiceStatus serviceStatus) override {
      PRINT_NOTIFICATION << "Thermal service status: ";
      std::string status;
      switch (serviceStatus) {
         case ServiceStatus::SERVICE_AVAILABLE: {
            status = "Available";
            break;
         }
         case ServiceStatus::SERVICE_UNAVAILABLE: {
            status = "Unavailable";
            break;
         }
         case ServiceStatus::SERVICE_FAILED: {
            status = "Failed";
            break;
         }
         default: {
            status = "Unknown";
            break;
         }
      }
      std::cout << status << std::endl;
   }


8. Receive notification when trip event occurs
  
  Note: Receives notification only if it is registered in step 5.

.. code-block::

   virtual void onTripEvent(std::shared_ptr<ITripPoint> tripPoint, TripEvent tripEvent) override {
      if (tripPoint) {
         PRINT_NOTIFICATION << ": TRIP UPDATE EVENT" << std::endl;
         printTripPointHeader();
         printTripPointInfo(tripPoint, tripEvent);
         return;
      }
      PRINT_NOTIFICATION << ": Invalid trip point" << std::endl;
   }


9. To de-register only trip update notification
   
   Note: likewise it can be de-registered for only cooling device changes notification.
   The SSR notification will not de-registered by default except mask: 0xFFFF.

.. code-block::
   
   thermalMgr->registerListener(thermalListener, 1 << TNT_TRIP_UPDATE);


10. Send get thermal zones request using thermal manager object

.. code-block::

   std::vector<std::shared_ptr<telux::therm::IThermalZone>> zoneInfo
      = thermalMgr->getThermalZones();
   if(zoneInfo.size() > 0) {
      for(auto index = 0; index < zoneInfo.size(); index++) {
         std::cout << "Thermal zone Id: " << zoneInfo(index)->getId() << "Description: "
                     << zoneInfo(index)->getDescription() << "Current temp: "
                     << zoneInfo(index)->getCurrentTemp() << std::endl;
         std::cout << std::endl;
      }
   } else {
      std::cout << "No thermal zones found!" << std::endl;
   }


11. Send get cooling devices request using thermal manager instance

.. code-block::

   std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevice
      = thermalMgr->getCoolingDevices();
   if(coolingDevice.size() > 0) {
      for(auto index = 0; index < coolingDevice.size(); index++) {
         std::cout << "Cooling device Id: " << coolingDevice(index)->getId()
                     << "Description: " << coolingDevice(index)->getDescription()
                     << "Max cooling level: " << coolingDevice(index)->getMaxCoolingLevel()
                     << "Current cooling level: " << coolingDevice(index)->getCurrentCoolingLevel()
                     << std::endl;
         std::cout << std::endl;
      }
   } else {
      std::cout << "No cooling devices found!" << std::endl;
   }


12. Send request to get thermal zone for specific id using thermal manager object

.. code-block::

   int thermalZoneId = THERMAL_ZONE_ID;
   std::cout << "Thermal zone info by Id: " << thermalZoneId << std::endl;
   std::shared_ptr<telux::therm::IThermalZone> tzInfo = thermalMgr->getThermalZone(thermalZoneId);
   if (tzInfo != nullptr) {
      printThermalZoneHeader();
      printZoneInfo(tzInfo);
      printBindingInfo(tzInfo);
   }


13. Cleanup when we don't need to listen to anything and when exit the application.
   
   Note: Here the APP is de-registering all notifications. However, the client can choose to de-register specific as well. For example, to deregister only trip update notifications, the client may provide mask: 0x0001 or mask: 0x0002 to deregister only the notification for change in cdev level.

.. code-block::

   thermalMgr->deregisterListener(thermalListener);
   thermalListener = nullptr;
   thermalMgr = nullptr;

