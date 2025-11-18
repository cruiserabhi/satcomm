.. _send-sms:

Sending SMS 
=======================

This sample application demonstrates how to send a SMS to a given cellphone specified by mobile number.

1. Implement ResponseCallback interface to receive subsystem initialization status

.. code-block::

   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(subSystemsStatus == SERVICE_AVAILABLE) {
         std::cout << SmsManager subsystem is ready << std::endl;
      } else if(subSystemsStatus == SERVICE_FAILED) {
         std::cout << SmsManager subsystem initialization failed << std::endl;
      }
      cbProm.set_value(status);
   }


2. Get the PhoneFactory and default SmsManager instance

.. code-block::

   auto &phoneFactory = PhoneFactory::getInstance();
   std::shared_ptr<ISmsManager> smsManager = phoneFactory.getSmsManager(initResponseCb);
   if(smsMgr == NULL) {
      std::cout << " Failed to get Sms Manager  instance" << std::endl;
      return -1;
   }


3. Wait for SmsManager subsystem to be ready

.. code-block::

   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Sms Manager subsystem << std::endl;
      return -1;
   }


4. Instantiate SMS sent and delivery callback and implement ICommandResponseCallback interface to know SMS sent and delivery status

.. code-block::

   auto smsSentCb = std::make_shared<SmsCallback>();
   auto smsDeliveryCb = std::make_shared<SmsDeliveryCallback>();

   class SmsCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
   };

   void SmsCallback::commandResponse(ErrorCode error) {
      std::cout << "onSmsSent callback" << std::endl;
   }

   class SmsDeliveryCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
   };

   void SmsDeliveryCallback::commandResponse(ErrorCode error) {
      std::cout << "SMS Delivery callback" << std::endl;
   }


5. Send an SMS using ISmsManager by passing the text and receiver number along with required callback

.. code-block::

   if(smsManager) {
      std::string receiverAddress("+18989531755");
      std::string message("TEST message");

      smsManager->sendSms(message, receiverAddress, smsSentCb, smsDeliveryCb);
   }


Now, we will receive resonses in callbacks defined at step 3 (smsSentCb, smsDeliveryCb).
