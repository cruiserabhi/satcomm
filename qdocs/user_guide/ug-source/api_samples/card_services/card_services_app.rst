.. _card-services-app:

Card service APIs to transmit APDU
=======================================================

This sample application demonstrates how to use card service APIs to transmit APDU.

1. Implement ResponseCallback interface to receive subsystem initialization status

.. code-block::

   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if (subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << Card Manager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << Card Manager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }


2. Get the PhoneFactory and CardManager instances

.. code-block::

   auto &phoneFactory = PhoneFactory::getInstance();
   auto cardManager = phoneFactory.getCardManager(initResponseCb);
   if (cardManager == NULL) {
      std::cout << " Failed to get Card Manager instance" << std::endl;
      return -1;
   }


3.Check if CardManager subsystem is ready

.. code-block::

   telux::common::ServiceStatus status = cardManager.getServiceStatus();


3.1 Wait for the CardManager subsystem initialization

.. code-block::

   telux::common::ServiceStatus status = cbProm.get_future().get();
   if (status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Card Manager subsystem << std::endl;
      return -1;
   }


4. Get number of slots, their IDs and card instance

.. code-block::

   int slotCount;
   cardManager->getSlotCount(slotCount);
   std::cout << "Slots Count is :" << slotCount << std::endl;

   std::vector<int> slotIds;
   cardManager->getSlotIds(slotIds);
   std::cout << "Slot Ids are : { ";
   for(auto id : slotIds) {
      std::cout << id << " ";
   }
   std::cout << "}" << std::endl;

   std::shared_ptr<ICard> cardImpl = cardManager->getCard(slotIds.front());


5. Get supported applications from the card

.. code-block::

   std::vector<std::shared_ptr<ICardApp>> applications;
   if(cardImpl) {
      std::cout << "\nApplications available are : " << std::endl;
      applications = cardImpl->getApplications();
      for(auto cardApp : applications) {
         std::cout << "AppId : " << cardApp->getAppId() << std::endl;
      }
   }



6. Instantiate optional IOpenLogicalChannelCallback, ICommandResponseCallback and ITransmitApduResponseCallback

.. code-block::

   auto myOpenLogicalCb = std::make_shared<MyOpenLogicalChannelCallback>();
   auto myCloseLogicalCb = std::make_shared<MyCloseLogicalChannelCallback>();
   auto myTransmitApduResponseCb = std::make_shared<MyTransmitApduResponseCallback>();


6.1 Implementation of ICardChannelCallback interface for receiving notifications on card event like open logical channel

.. code-block::

   class MyOpenLogicalChannelCallback : public ICardChannelCallback {
   public:
      void onChannelResponse(int channel, IccResult result, ErrorCode error) override;
   };

   void MyOpenLogicalChannelCallback::onChannelResponse(int channel, IccResult result,
                                                        ErrorCode error) {
      std::cout << "onChannelResponse, error: " << (int)error << std::endl;
      std::unique_lock<std::mutex> lock(eventMutex);
      errorCode = error;
      openChannel = channel;
      std::cout << "onChannelResponse: " << result.toString() << std::endl;
      if(cardEventExpected == CardEvent::OPEN_LOGICAL_CHANNEL) {
         std::cout << "Card Event OPEN_LOGICAL_CHANNEL found with code :" << int(error) << std::endl;
         eventCV.notify_one();
      }
   }


6.2. Implementation of ICommandResponseCallback interface for receiving notifications on card event like close logical channel

.. code-block::

   class MyCloseLogicalChannelCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
   };

   void MyCloseLogicalChannelCallback::commandResponse(ErrorCode error) {
      std::cout << "commandResponse, error: " << (int)error << std::endl;
      std::unique_lock<std::mutex> lock(eventMutex);
      errorCode = error;
      if(cardEventExpected == CardEvent::CLOSE_LOGICAL_CHANNEL) {
         std::cout << "Card Event CLOSE_LOGICAL_CHANNEL found with code :" << int(error) << std::endl;
         eventCV.notify_one();
      }
   }


6.3. Implementation of ICardCommandCallback interface for receiving notifications on card event like transmit APDU logical channel and transmit APDU basic channel

.. code-block::

   class MyTransmitApduResponseCallback : public ICardCommandCallback {
   public:
      void onResponse(IccResult result, ErrorCode error) override;
   };

   void MyTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
      std::cout << "onResponse, error: " << (int)error << std::endl;
      std::unique_lock<std::mutex> lock(eventMutex);
      errorCode = error;
      std::cout << "onResponse:  " << result.toString() << std::endl;
      if(cardEventExpected == CardEvent::TRANSMIT_APDU_CHANNEL) {
         std::cout << "Card Event TRANSMIT_APDU_CHANNEL found with code :" << int(error) << std::endl;
         eventCV.notify_one();
      }
   }


7. Open logical channel and wait for request to complete

.. code-block::

   std::string aid;
   for(auto app : applications) {
      if(app->getAppType() == APPTYPE_USIM) {
         aid = app->getAppId();
         break;
      }
   }

   cardImpl->openLogicalChannel(aid, myOpenLogicalCb);
   std::cout << "Opening Logical Channel to Transmit the APDU..." << std::endl;


8. Transmit APDU on logical channel, wait for request to complete

.. code-block::

   cardImpl->transmitApduLogicalChannel(openChannel, CLA, INSTRUCTION, P1, P2, P3, DATA,
                                        myTransmitApduResponseCb);
   std::cout << "Transmit APDU request made..." << std::endl;


9. Close the opened logical channel and wait for the completion

.. code-block::

   cardImpl->closeLogicalChannel(openChannel, myCloseLogicalCb);
   std::cout << "Close the Logical Channel..." << std::endl;


10. Transmit APDU on basic channel and wait for completion

.. code-block::

   cardImpl->transmitApduBasicChannel(CLA, INSTRUCTION, P1, P2, P3, DATA, myTransmitApduResponseCb);
   std::cout << "Transmit APDU request on Basic channel made..." << std::endl;

