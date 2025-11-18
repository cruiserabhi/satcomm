.. _remote-sim-api:

Using remote SIM manager APIs 
===============================================

This sample application demonstrates how to use remote SIM manager APIs for remote SIM card operations.

1. Implement ResponseCallback interface to receive subsystem initialization status

.. code-block::

   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if (subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << RemoteSim Manager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << RemoteSim Manager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }


2. Get the PhoneFactory and RemoteSimManager instances

.. code-block::

   #include <telux/tel/PhoneFactory.hpp>

   using namespace telux::common;
   using namespace telux::tel;

   PhoneFactory &phoneFactory = PhoneFactory::getInstance();
   auto remoteSimMgr =
       phoneFactory.getRemoteSimManager(DEFAULT_SLOT_ID, initResponseCb);
    if (remoteSimMgr == NULL) {
      std::cout << " Failed to get RemoteSim Manager instance" << std::endl;
      return -1;
   }


3.Check if telephony subsystem is ready

.. code-block::

   telux::common::ServiceStatus status = remoteSimMgr.getServiceStatus();


3.1 Wait for the telephony subsystem initialization

.. code-block::

   telux::common::ServiceStatus status = cbProm.get_future().get();
   if (status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Card Manager subsystem << std::endl;
      return -1;
   }

4. Instantiate and register RemoteSimListener

.. code-block::

   std::shared_ptr<IRemoteSimListener> listener = std::make_shared<RemoteSimListener>();
   remoteSimMgr.registerListener(listener);


4.1 Implementation of IRemoteSimListener interface for receiving Remote SIM notifications

.. code-block::

   class RemoteSimListener : public IRemoteSimListener {
   public:
       void onApduTransfer(const unsigned int id, const std::vector<uint8_t> &apdu) override {
           // Send APDU to SIM card
       }
       void onCardConnect() override {
           // Connect to SIM card and request AtR
       }
       void onCardDisconnect() override {
           // Disconnect from SIM card
       }
       void onCardPowerUp() override {
           // Power up SIM card and request AtR
       }
       void onCardPowerDown() override {
           // Power down SIM card
       }
       void onCardReset() override {
           // Reset SIM card
       }
       void onServiceStatusChange(ServiceStatus status) {
           // Handle case where modem goes down or comes up
       }
   };


4.2 Implementation of event callback for asynchronous requests

.. code-block::

   void eventCallback(ErrorCode errorCode) {
       std::cout << "Received event response with errorcode " << static_cast<int>(errorCode)
           << std::endl;
   }


5. Send connection available event request

   When the remote card is available and ready, make it available to the modem by sending a
   connection available request.

.. code-block::

   if (remoteSimMgr->sendConnectionAvailable(eventCallback) != Status::SUCCESS) {
       std::cout << "Failed to send connection available request!" << std::endl;
   }


6. Send card reset request after receiving onCardConnect() notification from listener

   You will receive an onCardConnect notification on the listener when the modem accepts the
   connection.

.. code-block::

   // After connecting to SIM card, requesting AtR, and receiving response with AtR bytes

   if (remoteSimMgr->sendCardReset(atr, eventCallback) != Status::SUCCESS) {
       std::cout << "Failed to send card reset request!" << std::endl;
   }


7. Send response APDU after receiving onTransmitApdu() notification from listener

.. code-block::

   // After sending command APDU to SIM and receiving the response

   if (remoteSimMgr->sendApdu(id, apdu, true, apdu.size(), 0, eventCallback) != Status::SUCCESS) {
       std::cout << "Failed to send response APDU!" << std::endl;
   }


8. Send connection unavailable request before exiting

   When the card becomes unavailable (or before you exit), tear down the connection with the modem.

.. code-block::

   if (remoteSimMgr->sendConnectionUnavailable() != Status::SUCCESS) {
       std::cout << "Failed to send connection unavailable request!" << std::endl;
   }

