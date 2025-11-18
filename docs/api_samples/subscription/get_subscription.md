Get network subscription information {#get_subscription}
========================================================

This sample application demonstrates how to get modem network subscription information.

### 1. Implement ResponseCallback interface to receive subsystem initialization status

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << Subscription Manager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << Subscription Manager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }
   ~~~~~~

### 2. Get the PhoneFactory and SubscriptionManager instance

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto subscriptionMgr = phoneFactory.getSubscriptionManager(initResponseCb);
   if(subscriptionMgr == NULL) {
      std::cout << " Failed to get Subscription Manager instance" << std::endl;
      return -1;
   }
   ~~~~~~

### 3. Wait for Subscription subsystem subsystem to be ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Subscription Manager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 4. Get the Subscription information

   ~~~~~~{.cpp}
   std::shared_ptr<ISubscription> subscription = subscriptionMgr->getSubscription();

   if(subscription != nullptr) {
      std::cout << "Subscription Details" << std::endl;
      std::cout << " CarrierName : " << subscription->getCarrierName() << std::endl;
      std::cout << " CountryISO : " << subscription->getCountryISO() << std::endl;
      std::cout << " PhoneNumber : " << subscription->getPhoneNumber() << std::endl;
      std::cout << " IccId : " << subscription->getIccId() << std::endl;
      std::cout << " Mcc : " << subscription->getMcc() << std::endl;
      std::cout << " Mnc : " << subscription->getMnc() << std::endl;
      std::cout << " SlotId : " << subscription->getSlotId() << std::endl;
      std::cout << " SubscriptionId : " << subscription->getSubscriptionId() << std::endl;
   }
   ~~~~~~
