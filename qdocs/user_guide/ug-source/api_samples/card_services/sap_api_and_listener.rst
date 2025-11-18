.. _sap-api-and-listener:

Using SAP APIs 
======================================

This sample application demonstrates how to use SAP APIs to transmit APDU and listen to SAP events.

1. Implement ResponseCallback interface to receive subsystem initialization status

.. code-block::

   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(status == SERVICE_AVAILABLE) {
         std::cout << Phone Manager subsystem is ready << std::endl;
      } else if(status == SERVICE_FAILED) {
         std::cout << Phone Manager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }


2. Get the PhoneFactory instance and SapCardManager instance

.. code-block::

    auto &phoneFactory = PhoneFactory::getInstance();

    std::promise<telux::common::ServiceStatus> prom;
    auto sapCardMgr = phoneFactory.getSapCardManager(
        DEFAULT_SLOT_ID, [&](telux::common::ServiceStatus status) {
        prom.set_value(status);
    });

    if (!sapCardMgr) {
       std::cout << "ERROR - Failed to get SapCardManager instance" << std::endl;
       return;
    }


3. Wait for the SapCardManager subsystem initialization

.. code-block::

    telux::common::ServiceStatus sapCardMgrStatus = sapCardMgr->getServiceStatus();
    if (sapCardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "SapCardManager subsystem is not ready , Please wait" << std::endl;
    }

    sapCardMgrStatus = prom.get_future().get();
    if (sapCardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "SapCardManager subsystem is ready" << std::endl;
    } else {
       std::cout << "ERROR - Unable to initialize SapCardManager subsystem" << std::endl;
       return;
    }


4. Instantiate ICommandResponseCallback, IAtrResponseCallback and ISapCardCommandCallback

.. code-block::

   auto mySapCmdResponseCb = std::make_shared<MySapCommandResponseCallback>();
   auto myAtrCb = std::make_shared<MyAtrResponseCallback>();
   auto myTransmitApduResponseCb = std::make_shared<MySapTransmitApduResponseCallback>();



4.1 Implementation of ICommandResponseCallback interface for receiving notifications on SAP events like open connection and close connection

.. code-block::

   class MySapCommandResponseCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error);
   };

   void MySapCommandResponseCallback::commandResponse(ErrorCode error) {
      std::cout << "commandResponse, error: " << (int)error << std::endl;
   }


4.2 Implementation of IAtrResponseCallback interface for receiving notification on SAP event like request answer to reset(ATR)

.. code-block::

   class MyAtrResponseCallback : public IAtrResponseCallback {
   public:
      void atrResponse(std::vector<int> responseAtr, ErrorCode error);
   };

   void MyAtrResponseCallback::atrResponse(std::vector<int> responseAtr, ErrorCode error) {
      std::cout << "atrResponse, error: " << (int)error << std::endl;
   }


4.3 Implementation of ISapCardCommandCallback interface for receiving notification on SAP event like transmit apdu

.. code-block::

   class MySapTransmitApduResponseCallback : public ISapCardCommandCallback {
   public:
      void onResponse(IccResult result, ErrorCode error);
   };

   void MySapTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
      std::cout << "transmitApduResponse, error: " << (int)error << std::endl;
   }


5. Open SAP connection and wait for request to complete

.. code-block::

   sapCardMgr->openConnection(SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA, mySapCmdResponseCb);
   std::cout << "Opening SAP connection to Transmit the APDU..." << std::endl;


6. Request SAP ATR and wait for complete

.. code-block::

   sapCardMgr->requestAtr(myAtrCb);


7. Send SAP APDU and wait for the request to complete

.. code-block::

   std::cout << "Transmit Sap APDU request made..." << std::endl;
   Status ret = sapCardMgr->transmitApdu(CLA, INSTRUCTION, P1, P2, LC, DATA, 0,
                                         myTransmitApduResponseCb);


8. Close SAP connection and wait for the request to complete

.. code-block::

   sapCardMgr->closeConnection(mySapCmdResponseCb);


