..
   *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

==================
Call Flow Diagrams
==================

Application initialization call flow
------------------------------------

Telematics-SDK initializes various sub-systems during start-up. It marks each sub-system's service status as SERVICE_AVAILABLE once the initialization procedures are completed for that sub-system. The application should pass callback function when requesting for manager object. This callback is invoked by underlying library which provides subsystem initialisation status.
The application has to wait until the corresponding sub-system callback is invoked. Application should make API requests when service status is SERVICE_AVAILABLE. Telematics-SDK provides APIs to fetch sub-system status.

Example:

Phone manager initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/phone_initialization_call_flow.png

1. Application uses PhoneFactory to call getPhoneManager by providing application callback function as parameter.
2. The application recieves IPhoneManager.
3. Application requests for serviceStatus from IPhoneManager.
4. Application waits for the subsystem initialization callback, which notifies the subsystem initialization status.

Telephony
---------

Dial call flow
~~~~~~~~~~~~~~

.. figure:: /../images/dial_call_flow.png

1. Application requests an instance of Call Manager object using PhoneFactory, providing the initialization callback.
2. Application receives a Call Manager instance.
3. Application waits for the subsystem initialization callback, which notifies the subsystem initialization status.
4. If the subsystem is initialized successfully, application registers a listener for call info change notifications like DIALING, ALERTING, ACTIVE and ENDED.
5. Application receives the status(SUCCESS or suitable failure) based on registration of listener to CallManager.
6. Application dials a number by using makeCall API, optionally specifying callback to get asynchronous response.
7. Application receives the status(SUCCESS or suitable failure) based on the execution of makeCall API.
8. Optionally, the application gets asynchronous response for makeCall operation status, through makeCallResponse callback.
9. Application receives the listener notifications on call status change like DIALING/ALERTING/ACTIVE when other party accepts the call.
10. Application sends hangup command to hangup the call, optionally specifying callback to get asynchronous response.
11. Application receives the status(SUCCESS or suitable failure) based on the execution of hangup API.
12. Optionally, the application gets asynchronous response for hangup using CommandResponseCallback.
13. Application receives the listener notification for the call end notification.

eCall call flow
~~~~~~~~~~~~~~~

.. figure:: /../images/ecall_call_flow.png

1. Application requests an instance of Call Manager object using PhoneFactory, providing the initialization callback.
2. Application receives a Call Manager instance.
3. Application waits for the subsystem initialization callback, which notifies the subsystem initialization status.
4. If the subsystem is initialized successfully, application registers a listener for call info change notifications like DIALING, ALERTING, ACTIVE and ENDED.
5. Application receives the status(SUCCESS or suitable failure) based on registration of listener to CallManager.
6. Application dials a standard or NG emergency call by using makeECall API, optionally specifying callback to get asynchronous response.
7. Application receives the status(SUCCESS or suitable failure) based on the execution of makeECall API.
8. Optionally, the application gets asynchronous response for makeECall operation status, through makeCallResponse callback.
9. Application receives the listener notifications on call status change like DIALING/ALERTING/ACTIVE when other party accepts the call.
10. Application receives the listener notifications related to MSD transmission status updates.
11. Application receives the listener notifications related to eCall HLAP timer status updates.
12. CallManager sends MSD update request recieved from network to the application by using OnMsdUpdateRequest API.
13. Optionally, the application can update the MSD using updateECallMsd API, whenever there is an update to MSD.
14. Application receives the status(SUCCESS or suitable failure) based on the execution of updateECallMsd API.
15. Optionally, the application gets asynchronous response for updateECallMsd operation status.
16. Application sends hangup command to hangup the call, optionally specifying callback to get asynchronous response.
17. Application receives the status(SUCCESS or suitable failure) based on the execution of hangup API.
18. Optionally, the application gets asynchronous response for hangup using CommandResponseCallback.
19. Application receives the listener notification for the call end notification.

TPS eCall over IMS call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Private eCall:

1. It is a normal VOLTE call(to a custom number) with MSD information.
2. Application processor (AP) sends the MSD data at call connect and later AP gets an explicit indication,
   upon which it can provide updated MSD(unlike standard eCall where AP updates MSD constantly)
3. Device doesn't support fallback to CS. It is the AP's responsibility to retry over CS.
4. AP can send the MSD information data in non standard format and PSAP should be able to
   recognize it.


.. figure:: /../images/tps_ecall_over_ims_call_flow.png

1. Application requests an instance of Call Manager object using PhoneFactory, providing the initialization callback.
2. Application receives a Call Manager instance.
3. Application waits for the subsystem initialization callback, which notifies the subsystem initialization status.
4. The application registers a listener with CallManager to listen to the call info change notifications like DIALING, ALERTING, ACTIVE etc.
5. The application receives the status like SUCCESS, FAILED etc  based on registration of listener from CallManager.
6. The application dials a TPS emergency call over IMS(i.e normal VOLTE call) by using makeECall API, optionally specifying header and callback to get asynchronous response.
7. The application receives the status like SUCCESS, FAILED etc based on the execution of makeECall API.
8. Optionally, the application gets asynchronous response for makeECall using makeCallResponseCallback.
9. Application receives the listener notifications on call status change like DIALING/ALERTING/ACTIVE when other party accepts the call.
10. CallManager sends eCall MSD transmission status to the application by using onECallMsdTransmissionStatus API.
11. CallManager sends MSD update request recieved from network to the application by using OnMsdUpdateRequest API.
12. The application sends the MSD data to network using updateECallMsd API, optionally specifying callback to get asynchronous response.
13. The application receives the status like SUCCESS, FAILED etc based on the execution of updateECallMsd API.
14. CallManager sends eCall MSD transmission status to the application by using onECallMsdTransmissionStatus API for MSD sent at step 10.
15. The application sends hangup command to hangup the call, optionally gets asynchronous response using callback.
16. The application receives the status like SUCCESS, FAILED etc based on the execution of hangup API.
17. Optionally, the application gets asynchronous response for hangup using CommandResponseCallback.
18. CallManager sends call info change i.e Call Ended to the application by using onCallInfoChange callback function.

Answer, Reject, RejectWithSMS call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/answer_reject_reject_with_sms_call_flow.png

1. The application gets the PhoneManager object using PhoneFactory.
2. The application receives the PhoneManager object in order to register listener.
3. The application registers a listener with CallManager to listen incoming call notifications.
4. The application receives the status like SUCCESS, FAILED etc  based on registration of listener from CallManager.
5. The application receives onIncomingCall notification when there is an incoming call.
6. The application performs answer/reject/rejectWithSMS operation using ICall.
7. The application receives the status like SUCCESS, FAILED etc  based on execution of answer/reject/rejectWithSMS.
8. Optionally, the application gets asynchronous response for answer/reject/rejectWithSMS using CommandResponseCallback.
9. The CallManager sends call info change i.e CALL_ACTIVE or CALL_ENDED to the application by using onCallInfoChange callback function.

Hold call flow
~~~~~~~~~~~~~~

.. figure:: /../images/hold_call_flow.png

1. The application gets the PhoneManager object using PhoneFactory.
2. The application receives the PhoneManager object in order to get Phone.
3. The application gets the Phone object for given phone identifier using PhoneManager.
4. PhoneManager returns Phone object to application, returns default phone in case phone identifier is not specified.
5. The application registers a listener with CallManager to listen to the call info change notifications like DIALING, ALERTING, ACTIVE etc.
6. The application receives the status like SUCCESS or INVALIDPARAM based on registration of listener to CallManager.
7. The application dials a number by using makeCall API, optionally specifying callback to get asynchronous response.
8. The application receives the status like SUCCESS, INVALIDPARAM and FAILED etc based on the execution of makeCall API.
9. Optionally, the application gets asynchronous response for makeCall using makeCallResponseCallback.
10. The CallManager sends call info change i.e CALL_ACTIVE to application by using onCallInfoChange API.
11. The application sends hold command to hold the call, optionally specifying callback to get asynchronous response.
12. The application receives the status like SUCCESS, FAILED etc based on the execution of hold API.
13. Optionally, the application gets asynchronous response for hold using CommandResponseCallback.
14. The application receives call info change i.e CALL_ON_HOLD from CallManager.

Hold, Conference, Swap call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/hold_conference_swap_call_flow.png

1. The application makes first call using ICall.
2. The application receives the status like SUCCESS, FAILED etc based on execution of makeCall operation.
3. Optionally, the application gets asynchronous response for makeCall using makeCallResponseCallback.
4. The CallManager sends call info change i.e CALL_ACTIVE to application by using onCallInfoChange API.
5. The application sends hold command to hold the call, optionally specifying callback to get asynchronous response.
6. The application receives the status like SUCCESS, FAILED etc based on the execution of hold API.
7. Optionally, the application gets asynchronous response for hold using CommandResponseCallback.
8. The application receives call info change i.e CALL_ON_HOLD from CallManager.
9. The application makes second call using ICall.
10. The application receives the status like SUCCESS, FAILED etc based on execution of makeCall operation.
11. Optionally, the application gets asynchronous response for makeCall using makeCallResponseCallback.
12. The CallManager sends call info change i.e CALL_ACTIVE to application by using onCallInfoChange API.
13. The application requests the PhoneFactory to get ICallManager object.
14. The application receives the ICallManager object using PhoneFactory.
15. The application performs conference/swap operation using ICallManager by passing first call and second call. optionally application can pass callback to receive hold response asynchronously.
16. The application receives the status like SUCCESS, FAILED etc based on the execution of conference/swap API.
17. Optionally, the application gets asynchronous response for conference/swap using CommandResponseCallback.

SMS call flow
~~~~~~~~~~~~~

.. figure:: /../images/sms_call_flow.png

1. Application requests to check multi sim configuration on device.
2. Application receives the result.
3. The application updates the list of phone identifier based on device configuration and gets SmsManager object corresponding to specific phone identifier.
4. PhoneFactory returns the SmsManager object to application in order to perform operations like send SMS and get SMSC address.
5. The application sends SMS to the receiver address and optionally gets asynchronous response using CommandResponseCallback.
6. Application receives the status i.e. either SUCCESS or FAILED based on execution of sendSms API in SmsManager.
7. Optionally, the response for send SMS is received by the application.
8. Application gets notified for incoming SMS.
9. The application requests for SmscAddress and optionally gets asynchronous response using ISmscAddressCallback.
10. Application receives the status i.e. either SUCCESS or FAILED based on successful execution of requestSmscAddress API in SmsManager.
11. Optionally, the application receives the SMSC address on success or gets error on failure in the command response callback.

Signal strength call flow
~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/signal_strength_call_flow.png

1. The application gets PhoneManager object using PhoneFactory.
2. The application receives the PhoneManager object in order to get Phone instance.
3. The application gets phone instance for a given phone identifier using PhoneManager object.
4. PhoneManager returns IPhone object to the application.
5. Application registers the listener to get notification for signal strength change.
6. The application receives the status i.e. either SUCCESS or FAILED based on the registration of the listener.
7. The application requests for signal strength and optionally, gets asynchronous response using ISignalStrengthCallback.
8. The application receives the status i.e. either SUCCESS or FAILED based on execution of requestSignalStrength API in SapCardManager.
9. Optionally, the response for signal strength request is received by the application.
10. Application receives a notification when there is a change in signal strength.

CellBroadcast Call Flow
~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cellbroadcast_call_flow.png

1. Application requests an instance of CellBroadcast Manager object using PhoneFactory, providing the initialization callback.
2. Application receives a CellBroadcast Manager instance.
3. Application waits for the subsystem initialization callback, which notifies the subsystem initialization status.
4. If the subsystem is initialized successfully, application registers to ICellBroadcastListener for incoming Cell broadcast messages and message filter changes.
5. The application receives the status i.e. either SUCCESS or INVALIDPARAM based on successful registration of the listener.
6. Application gets notified for incoming CellBroadcast Message by ICellBroadcastListener.
7. The application requests for message filters and gets response with RequestFiltersResponseCallback.
8. Application receives the status i.e. either SUCCESS or FAILED based on successful execution of requestMessageFilters API in CellBroadcastManager.
9. The application gets asynchronous response for requestMessageFilters through RequestFiltersResponseCallback.
10. Application can configure/update message filters by specifying message filters and gets response with optional callback.
11. Application receives the status i.e. either SUCCESS or FAILED based on successful execution of updateMessageFilters API in CellBroadcastManager.
12. Optionally, the application gets asynchronous response for updateMessagefilters.
13. Application gets notified for updated message filters for CellBroadcast Message.
14. The application requests for activation status and gets response with RequestActivationStatusResponseCallback.
15. Application receives the status i.e. either SUCCESS or FAILED based on successful execution of requestActivationStatus API in CellBroadcastManager.
16. The application gets asynchronous response for requestActivationStatus through RequestActivationStatusResponseCallback.
17. Apllication can activate/deactivate configured broadcast messages by setActivationStatus.
18. Application receives the status i.e. either SUCCESS or FAILED based on successful execution of setActivationStatus API in CellBroadcastManager.
19. Optionally, the application gets asynchronous response for setActivationStatus.

Radio and Service state call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/radio_and_service_state_call_flow.png

1. The application gets PhoneManager object using PhoneFactory.
2. The application receives the PhoneManager object in order to get Phone instance.
3. The application gets phone instance for a given phone identifier using PhoneManager object.
4. PhoneManager returns IPhone object to the application.
5. Application registers the listener to get notifications for radio and service state change.
6. The application receives the status i.e. either SUCCESS or FAILED based on the registration of the listener.
7. The application request the PhoneManager to get operatingMode of device.
8. The application receives the status i.e. either SUCCESS or FAILED.
9. The application receives the callback of the request with the information of current operating mode of device.
10. The application request the PhoneManager to set operatingMode of device.
11. The application receives the status i.e. either SUCCESS or FAILED.
12. Application receives a notification when there is a change in operatingMode.
13. The application request the PhoneManager to get voice service state.
14. The application receives the status i.e. either SUCCESS or FAILED.
15. The application receives the voice service state, radio technology information etc.
16. Application receives a notification when there is a change in voice service state.

Network Selection Manager call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Network selection manager provides APIs to get and set network selection mode,
get and set preferred networks, set dubious cell for LTE and NR network and perform network scan for
available networks. Registered listener will get notified for the change in network selection mode.

.. figure:: /../images/network_selection_call_flow.png

1. Application requests phone factory for network selection manager.
2. Phone factory returns INetworkSelectionManager object using which application will
   register or deregister a listener.
3. Application can register a listener to get notifications for network selection mode change.
4. Status of register listener i.e. either SUCCESS or other status will be returned to the application.
5. Application requests for network selection mode using INetworkSelectionManager object and gets
   asynchronous response using SelectionModeResponseCallback.
6. The application receives the status i.e. either SUCCESS or other status based on the execution
   of requestNetworkSelectionMode API.
7. The response for get network selection mode request is received by the application.
8. The application can also set network selection mode and optionally gets asynchronous response using
   ResponseCallback. MCC and MNC are optional for AUTOMATIC network selection mode.
9. Application receives the status i.e. either SUCCESS or other status based on the execution
   of setNetworkSelectionMode API.
10. Optionally the response for set network selection mode request is received by the application.
11. Registered listener will get notified for the network selection mode change.
12. Similarly, the application requests for preferred networks using INetworkSelectionManager object
    and gets asynchronous response using PreferredNetworksCallback.
13. The application receives the status i.e. either SUCCESS or other status based on the execution
    of requestPreferredNetworks API.
14. The response for get preferred networks request i.e. 3GPP preferred network list and
    static 3GPP preferred network list is received by the application asynchronously. Higher
    priority networks appear first in the list. The networks that appear in the 3GPP Preferred
    Networks list get higher priority than the networks in the static 3GPP preferred networks list.
15. The application can set 3GPP preferred network list and optionally gets asynchronous response using
    ResponseCallback. If clear previous networks flag is false then new 3GPP preferred network list is
    appended to existing preferred network list. If flag is true then old list is flushed and new 3GPP
    preferred network list is added.
16. Application receives the status i.e. either SUCCESS or other status based on the execution
    of setPreferredNetworks API.
17. Optionally the response for set preferred networks request is received by the application.
18. The application can perform network scan for available networks using INetworkSelectionManager object
    and gets asynchronous response using NetworkScanCallback.
19. Application receives the status i.e. either SUCCESS or other status based on the execution
    of performNetworkScan API.
20. Network name, MCC, MNC and status of the operator will be received by the application.
21. The application can set dubious cell list for LTE network..
22. Application receives the errorcode i.e. either SUCCESS or other errorcode based on execution of
    setLteDubiousCell API.
23. The application can set dubious cell list for NR network..
24. Application receives the errorcode i.e. either SUCCESS or other errorcode based on execution of
    setNrDubiousCell API.
25. Application can deregister a listener there by it would not get notifications.
26. Status of deregister listener i.e. either SUCCESS or other status will be returned to the application.

Serving System Manager Call Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Serving system manager provides APIs to get and set RAT mode preference and get and set service domain preference.
Registered listener will get notified for the change in RAT mode and service domain preference change.

.. figure:: /../images/serving_system_call_flow.png

1. Application requests phone factory for serving system manager.
2. Phone factory returns IServingSystemManager object using which application will
   register or deregister a listener.
3. Application can register a listener to get notifications for RAT mode and service domain
   preference changes.
4. Status of register listener i.e. either SUCCESS or other status will be returned to the application.
5. Application requests for RAT mode preference using IServingSystemManager object and gets
   asynchronous response using RatPreferenceCallback.
6. The application receives the status i.e. either SUCCESS or other status based on the execution
   of requestRatPreference API.
7. The response for get RAT preference request is received by the application.
8. The application can also set RAT mode preference and optionally gets asynchronous response using
   ResponseCallback.
9. Application receives the status i.e. either SUCCESS or other status based on the execution
   of setRatPreference API.
10. Optionally the response for set RAT preference request is received by the application.
11. Registered listener will get notified for the RAT mode preference change.
12. Application requests for service domain preference using IServingSystemManager object and gets
    asynchronous response using ServiceDomainPreferenceCallback.
13. The application receives the status i.e. either SUCCESS or other status based on the execution
    of requestServiceDomainPreference API.
14. The response for get service domain preference request is received by the application.
15. The application can also set service domain preference and optionally gets asynchronous response using
    ResponseCallback.
16. Application receives the status i.e. either SUCCESS or other status based on the execution
    of setServiceDomainPreference API.
17. Optionally the response for set service domain preference request is received by the application.
18. Registered listener will get notified for the service domain preference change.
19. Application can deregister a listener there by it would not get notifications.
20. Status of deregister listener i.e. either SUCCESS or other status will be returned to the application.

Card-Get applications call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/get_applications_call_flow.png

1. Application gets the CardManager object from PhoneFactory.
2. Application receives CardManager object in order to perform operations like getSlotIds and getCard.
3. The application registers a listener for Card info change event with CardManager.
4. Application receives the status i.e. either SUCCESS or INVALIDPARAM based on the registration of listener.
5. The response from onCardInfoChanged is received by the application whenever there is card info change.
6. The application gets the slotIds from the sub-system using CardManager.
7. Application receives the status i.e. either SUCCESS or NOTREADY along with the updated slotIds.
8. Then, the application sends request to CardManager to get Card object for a specific slotId.
9. Application receives Card object from CardManager in order to perform card operation like getApplications.
10. The application gets the CardApps from Card object.
11. The application receives CardApps which contain information such as AppId, AppType and AppState.
12. Now the application removes the listener associated with CardManager.
13. Application receives the status i.e. either SUCCESS or NOSUCH for the removal of listener.

Card-Transmit APDU call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On logical channel
^^^^^^^^^^^^^^^^^^

.. figure:: /../images/transmit_apdu_logical_call_flow.png

1. The Application requests CardApp for the SIM application identifier.
2. The application receives the application identifier to perform open logical channel.
3. Application sends request to open the logical channel with the application identifier and optionally, gets asynchronous response in OpenLogicalChannelCallback.
4. The application receives the status i.e. either SUCCESS or FAILED based on execution of openLogicalChannel API.
5. Optionally, the application receives the response which contains either channel number on success or error in case of failure.
6. Then, the application transmits the APDU data on logical channel using the channel obtained earlier. Optionally, gets asynchronous response in TransmitApduResponseCallback.
7. The application receives the status i.e. either SUCCESS or FAILED based on execution of transmitApduLogicalChannel API.
8. Optionally, the application receives the response which contains either result on success or error in case of failure.
9. Finally, the application closes the logical channel that is opened to transmit APDU and optionally, gets asynchronous response in CommandResponseCallback.
10. The application receives the status i.e. either SUCCESS or FAILED based on execution of closeLogicalChannel API.
11. Optionally, the application receives the response which contains error in case of failure.

On basic channel
^^^^^^^^^^^^^^^^

.. figure:: /../images/transmit_apdu_basic_call_flow.png

1. Application gets the ICardManager object from PhoneFactory.
2. Application receives ICardManager object in order to perform operation like getCard.
3. The application gets ICard object for a specific slotId from ICardManager.
4. Application receives ICard object in order to perform card operation like transmitApduBasicChannel.
5. The application transmits APDU data on basic channel and optionally, gets asynchronous response in TransmitApduResponseCallback.
6. The application receives the status i.e. either SUCCESS or FAILED based on execution of transmitApduBasicChannel API.
7. Optionally, the application receives the response which contains either result on success or error in case of failure.

SAP card manager call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~

Request card reader status, Request ATR, Transmit APDU call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/sap_card_operations.png

1. The application gets SapCardManager object corresponding to slotId using PhoneFactory.
2. The application receives the SapCardManager object in order to perform SAP operations like request ATR, Card Reader Status and transmit APDU.
3. The application opens SIM Access Profile(SAP) connection with SIM card using default SAP condition (i.e. SAP_CONDITION_BLOCK_VOICE_OR_DATA) and optionally, gets asynchronous
   response using CommandResponseCallback.
4. The application receives the status i.e. either SUCCESS or FAILED based on execution of openConnection API in SapCardManager.
5. Optionally, the response for openConnection is received by the application.
6. The application sends request card reader status command and optionally, gets asynchronous response using ICardReaderCallback.
7. The application receives the status i.e. either SUCCESS or FAILED based on execution of requestCardReaderStatus API in SapCardManager.
8. Optionally, the response for card reader status is received by the application.
9. Similarly, the application can send SAP Answer To Reset command and optionally, gets asynchronous response using IAtrResponseCallback.
10. The application receives the status i.e. either SUCCESS or FAILED based on execution of requestAtr API in SapCardManager.
11. Optionally, the response for SAP Answer To Reset is received by the application.
12. Similarly, the application sends the APDU on SAP mode and optionally, gets asynchronous response using ISapTransmitApduResponseCallback.
13. The application receives the status i.e. either SUCCESS or FAILED based on execution of transmitApdu API in SapCardManager.
14. Optionally, the response for transmit APDU is received by the application.
15. Now the application closes the SAP connection with SIM and optionally, gets asynchronous response using CommandResponseCallback.
16. The application receives the status i.e. either SUCCESS or FAILED based on execution of closeConnection API in SapCardManager.
17. Optionally, the response for SAP close connection is received by the application.

SIM Turn off, Turn on and Reset call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/sap_mgr_sim_call_flow.png

1. Application gets SapCardManager object corresponding to slotID using PhoneFactory.
2. PhoneFactory returns the SapCardManager object to application in order to perform SAP operations like SIM power off, on or reset.
3. The application opens SIM Access Profile(SAP) connection with SIM card using default SAP condition (i.e. SAP_CONDITION_BLOCK_VOICE_OR_DATA) and optionally, gets
   asynchronous response using CommandResponseCallback.
4. Application receives the status i.e. either SUCCESS or FAILED based on execution of openConnection API in SapCardManager.
5. Optionally, the response for openConnection is received by the application.
6. The application sends SIM Power Off command to turn off the SIM and optionally, gets asynchronous response using CommandResponseCallback.
7. The application receives the status i.e. either SUCCESS or FAILED based on execution of requestSimPowerOff API in SapCardManager.
8. Optionally, the response for SIM Power Off is received by the application.
9. Similarly, the application can send SIM Power On command to turn on the SIM and optionally, gets asynchronous response using CommandResponseCallback.
10. The application receives the status i.e. either SUCCESS or FAILED based on execution of requestSimPowerOn API in SapCardManager.
11. Optionally, the response for SIM Power On is received by the application.
12. Similarly, the application sends SIM Reset command to perform SIM Reset and optionally, gets asynchronous response.
13. The application receives the status i.e. either SUCCESS or FAILED based on execution of requestSimReset API in SapCardManager.
14. Optionally, the response for SIM Reset is received by the application.
15. Now the application closes the SAP connection with SIM and optionally, gets asynchronous response using CommandResponseCallback.
16. The application receives the status i.e. either SUCCESS or FAILED based on execution of closeConnection API in SapCardManager.
17. Optionally, the response for SAP close connection is received by the application.

Subscription Call flows
~~~~~~~~~~~~~~~~~~~~~~~

Subscription initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/subscription_initialization_call_flow.png

1. Application requests an instance of SubscriptionManager using PhoneFactory by providing the initialization callback.
2. Application receives a Subscription Manager instance.
3. Application uses SubscriptionManager::getServiceStatus to determine if the Subscription Manager is ready.
4. Application waits for the subsystem initialization callback, which notifies the subsystem initialization status.
5. SubscriptionManager updates the application once subscription manager initialization completes.

Subscription call flow
^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/subscription_call_flow.png

1. The application gets the PhoneManager object using PhoneFactory.
2. The application receives the PhoneManager object in order to get Subscription.
3. The application gets the Subscription object for given slot identifier using SubscriptionManager.
4. SubscriptionManager returns Subscription object to application. Subscription can be used to get subscription details like countryISO, operator details etc.
5. The Subscription manager registers a listener with CardManager to listen to the card info change notifications like card state PRESENT, ABSENT, UNKNOWN, ERROR and RESTRICTED.
6. The SubscriptionManager receives the status like SUCCESS or INVALIDPARAM based on registration of listener to CardManager.
7. The SubscriptionManager receives callback card info change i.e subscription info changed or removed.
8. The SubscriptionManager updates the application once the subscription info is updated.

Remote SIM Provisioning Call Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Remote SIM provisioning provides API to add profile, delete profile, activate/deactivate profile
on the embedded SIMs (eUICC) , get list of profiles, get server address like SMDP+ and SMDS and
update SMDP+ address, update nick name of profile and retrieve Embedded Identity Document(EID)
of the SIM.

Download and deletion of profile call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/rsp_add_delete_profile_call_flow.png

1. Application requests Phonefactory for SimProfileManager object with application callback.
2. Phonefactory returns shared pointer to SimProfileManager object to application using which
   application performs profile related operations. Wait for subsystem to get ready.
3. If subsystem is ready, create a listener of type ISimProfileListener which would receive
   notifications about profile download status, user display info and confirmation code
   is required. Register the created listener with the ISimProfileManager object.
4. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
5. Application can send a request to add profile with activation code. The confirmation code can
   be optional and user consent supported can be specified in order to receive user consent info.
6. Application receives synchronous status which indicates if the add profile request was sent
   successfully.
7. The response of addProfile request can be received by application in the application-supplied
   callback. The modem sends indication about the HTTP request in order to download profile to AP
   and the AP performs HTTP transaction on behalf of modem and sends HTTP request to SMDP+/SMDS
   server and response of HTTP request from SMDP+/SMDS is sent back to AP and then to modem. The
   HTTP response contains information related to profile.
8. Optionally, the application receives notification about user consent required and profile policy
   rules. The application needs to provide the user consent and also reason (if in case consent not
   provided) in order to proceed with downloading of profile. The application receives the
   synchronous status which indicates provide user consent sent successfully and response of provide
   user consent can be received by application in the application-supplied callback. Optionally, the
   application receives notification about confirmation code required. The application needs to
   provide the code in order to proceed with downloading of profile. The application receives the
   synchronous status which indicates confirmation code required sent successfully and response of
   provide user consent can be received by application in the application-supplied callback.
9. The application receives notification about the download and installation status of profile on
   the eUICC. When download and installation of profile is completed, notification is also sent to
   SMDP+/SMDS server.
10. Application can send a delete request associated with profile identifier on SimProfileManager
    object.
11. Application receives synchronous status which indicates if the delete profile request was sent
    successfully.
12. The response of delete request can be received by application in the application-supplied
    callback. Once deletion of profile is completed notification is sent to SMDP+/SMDS server
    in order to synchronise the profile state on the server.
13. De-register the listener with the ISimProfileManager object.
14. Application receives the status i.e. either SUCCESS or FAILED based on the execution of
    de-register listener.

SIM profile management operations call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/rsp_other_operations_call_flow.png

The SIM profile sub-system should have been initialized successfully with SERVICE_AVAILABLE as a
pre-requisite for remote SIM provisioning operations and a valid SimProfileManager object is
available.

1. Application can send a set profile request associated with profile identifier on SimProfileManager
   object. This allows to enable or disable the profile on the eUICC.
2. Application receives synchronous status which indicates if the set profile request was sent
   successfully.
3. The response of set profile request can be received by application in the application-supplied
   callback. Once enable or disable of profile is completed notification is sent to SMDP+/SMDS
   server in order to synchronise the profile state on the server.
4. Application can send a get profile list request on SimProfileManager object to retrieve the all
   available profiles on the eUICC with all profile related details like service provider name(SPN),
   ICCID etc.
5. Application receives synchronous status which indicates if the get profile list request was sent
   successfully.
6. The response of get profile list request can be received by application in the application-supplied
   callback along with all the profile details.
7. Application can get Card object from CardManager associated with the slot.
8. CardManager returns shared pointer to Card object to application using which
   application can retrieve EID.
9. Application can send a get EID request on Card.
10. Application receives synchronous status which indicates if the get EID request was sent
    successfully.
11. The response of get EID request can be received by application in the application-supplied
    callback along with EID details.
12. Application can send update nickname of profile request on SimProfileManager.
13. Application receives synchronous status which indicates if the update nickname request was sent
    successfully.
14. The response of update nickname request can be received by application in the application
    supplied callback.
15. Application can send memory reset request on SimProfileManager to delete test or operational
    profiles or set SMDP address to default.
16. Application receives synchronous status which indicates if the memory reset request was sent
    successfully.
17. The response of memory reset request can be received by application in the application-supplied
    callback. Once deletion of profiles happens notification is sent to SMDP+/SMDS to synchronise
    profile state on the server.
18. Application can send set address name on SimProfileManager in order to set SMDP+ address.
19. Application receives synchronous status which indicates if the set address request was sent
    successfully.
20. The response of set address request can be received by application in the application-supplied
    callback.
21. Application can send get address name on SimProfileManager in order to get SMDP+/SMDS address.
22. Application receives synchronous status which indicates if the get address request was sent
    successfully.
23. The response of get address request can be received by application in the application-supplied
    callback along with SMDP+/SMDS address.


HttpTransaction subsystem readiness and handling of indication from modem call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/rsp_http_transaction_subsystem_startup_call_flow.png

1. Get the reference to the PhoneFactory, with which we can further acquire other remote SIM
   provisioning sub-system objects.
2. The reference daemon gets the Phonefactory object.
3. Reference daemon requests Phonefactory for HttpTransactionManager object along with init
   callback which gets called once initialization is completed.
4. Phonefactory returns shared pointer to HttpTransactionManager object to reference daemon using
   which reference daemon sends HTTP response to modem and register the listener to listen for HTTP
   transaction indication. If HttpTransactionManager does not exists then create new object.
5. Wait for subsystem to be ready and get the Service status.
6. The reference daemon check the service status whether service is UNAVAILABLE, AVAILABLE or FAILED.
   If the service status is not AVAILABLE, then wait for init callaback to be called. The subsystem
   either gets AVAILABLE or FAILED and Service status is recieved in init callback. If the service
   status is notified as SERVICE_FAILED, retry intitialization starting with step (3).
7. If subsystem is AVAILABLE , then create a listener of type IHttpListener which would receive
   notifications about HTTP transaction indication. Register the created listener with the
   IHttpTransactionManager object.
8. Status of register listener i.e. either SUCCESS or FAILED will be returned to the reference daemon.
9. Modem sends the HTTP transaction indication with HTTP request payload and other details to
   HTTPTransactionManager.
10. The reference daemon on AP receives notification about HTTP transaction which process HTTP request.
11. The reference daemon sends HTTP request result (SUCCESS or FAILURE) with HTTP response back to
    HTTPTransactionManager.
12. Steps (9-11) keep repeating for multiple HTTP Transaction indication for add, delete, set profile
    and memory reset operations.

Remote SIM
~~~~~~~~~~

Application will get the remote SIM manager object from phone factory. The application must
register a listener to receive commands/messages from the modem to send to the SIM. After sending
the connection available message, a onCardConnect() notification tells the application to connect
to the SIM and perform an Answer to Reset. After sending the card reset message (with the AtR
bytes), APDU messages will begin to be sent/received.

.. figure:: /../images/remote_sim_call_flow.png

1. Application requests remote SIM manager object from phone factory, specifying a slot id and application callback.
2. Phone factory returns IRemoteSimManager object.
3. If subsystem is ready, application registers a listener to receive commands/messages from the modem to send to the SIM.
4. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
5. Application sends a connection available message indicating that a SIM is available for use.
6. Status of send connection available i.e. either SUCCESS or FAILED will be returned to the application.
7. Optionally, the response to send connection available request can be received by the application.
8. Application will receive a card connect notification by the listener.
9. After the application successfully connects to the SIM and requests an AtR, it sends a card reset message with the AtR bytes.
10. Status of send card reset i.e. either SUCCESS or FAILED will be returned to the application.
11. Optionally, the response to send card reset request can be received by the application.
12. Application will receive an APDU transfer notification by the listener (with APDU message id).
13. After forwarding the APDU transfer to the SIM and receiving the response, application will send APDU response.
14. Status of send APDU i.e. either SUCCESS or FAILED will be returned to the application.
15. Optionally, the response to send APDU request can be received by the application.
16. To close the connection, application will send connection unavailable message.
17. Status of send connection unavailable i.e. either SUCCESS or FAILED will be returned to the application.
18. Optionally, the response to send connection unavailable can be received by the application.

Location Services
-----------------
Application will get the location manager object from location factory. The caller needs to register a listener. Application would then need to start the reports using one of 2 APIs depending on if the detailed or basic reports are needed.
When reports are no longer required, the app needs to stop the report and de-register the listener.

.. note::
   Applications need to have "locclient" Linux group permissions to be able to operate successfully with underlying services.

Call flow to register/remove listener for generating basic reports
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_basic_reports_callflow.png


1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object using which application will register or remove a listener.
3. Application can register a listener for getting notifications for location updates.
4. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
5. Application starts the basic reports using startBasicReports API for getting location updates.
6. Status of startBasicReports i.e. either SUCCESS or FAILED will be returned to the application.
7. The response for startBasicReports is received by the application.
8. Application will get location updates like latitude, longitude and altitude etc.
9. Application stops receiving the report through stopReports API.
10. Status of stopReports i.e. either SUCCESS or FAILED will be returned to the application.
11. The response for stopReports is received by the application.
12. Application can remove listener and when the number of listeners are zero then location service will get stopped automatically.
13. Status of remove listener i.e. either SUCCESS or FAILED will be returned to the application.

Call flow to register/remove listener for generating detailed reports
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_detailed_reports_callflow.png

1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object using which application will register or remove a listener.
3. Application can register a listener for getting notifications for location, satellite vehicle, jammer signal, nmea and measurements updates.
4. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
5. Application starts the detailed reports using startDetailedReports API for getting location, satellite vehicle, jammer signal, nmea and measurements updates.
6. Status of startDetailedReports i.e. either SUCCESS or FAILED will be returned to the application.
7. The response for startDetailedReports is received by the application.
8. Application will get location updates like latitude, longitude and altitude etc.
9. Application will receive satellite vehicle information like SV status and constellation etc.
10. Application will receive nmea information.
11. Application will receive jammer information etc.
12. Application will receive measurement information.
13. Application stops receiving all the reports through stopReports API.
14. Status of stopReports i.e. either SUCCESS or FAILED will be returned to the application.
15. The response for stopReports is received by the application.
16. Application can remove listener and when the number of listeners are zero then location service will get stopped automatically.
17. Status of remove listener i.e. either SUCCESS or FAILED will be returned to the application.

Call flow to register/remove listener for generating detailed engine reports
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_detailed_engine_reports_callflow.png

1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object using which application will register or remove a listener.
3. Application can register a listener for getting notifications for location, satellite vehicle and jammer signal, nmea and measurements updates.
4. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
5. Application starts the detailed engine reports using startDetailedEngineReports API for getting location, satellite vehicle, jammer signal, nmea and measurements updates.
6. Status of startDetailedReports i.e. either SUCCESS or FAILED will be returned to the application.
7. The response for startDetailedReports is received by the application.
8. Application will get location updates like latitude, longitude and altitude etc from the requested engine type(SPE/PPE/Fused).
9. Application will receive satellite vehicle information like SV status and constellation etc depending on the requested SPE/PPE/Fused engine type.
10. Application will receive nmea information depending on the requested SPE/PPE/Fused engine type
11. Application will receive jammer information etc depending on the requested SPE/PPE/Fused engine type.
12. Application will receive measurement information etc depending on the requested SPE/PPE/Fused engine type.
13. Application stops receiving all the reports through stopReports API.
14. Status of stopReports i.e. either SUCCESS or FAILED will be returned to the application.
15. The response for stopReports is received by the application.
16. Application can remove listener and when the number of listeners are zero then location service will be stopped automatically.
17. Status of remove listener i.e. either SUCCESS or FAILED will be returned to the application.

Call flow to register/remove listener for system info updates
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_system_info_updates_callflow.png

1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object using which application will register or remove a listener.
3. Application can register a listener for system information updates with registerForSystemInfoUpdates.
4. Status of registerForSystemInfoUpdates i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for registerForSystemInfoUpdates is received by the application.
6. Application will get system information update.
7. Application can remove listener with deRegisterForSystemInfoUpdates.
8. Status of deRegisterForSystemInfoUpdates i.e. either SUCCESS or FAILED will be returned to the application.
9. The response for deRegisterForSystemInfoUpdates is received by the application.

Call flow to request energy consumed information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_request_energy_consumed_callflow.png

1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object.
3. Application can request for energy consumed information with requestEnergyConsumedInfo.
4. Status of requestEnergyConsumedInfo i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for requestEnergyConsumedInfo is received by the application.

Call flow to get year of hardware information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_get_year_of_hardware_callflow.png

1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object.
3. Application can request for year of hardware information with getYearOfHw.
4. Status of getYearOfHw i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for getYearOfHw is received by the application.

Call flow to get terrestrial positioning information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_get_terrestrial_positioning_callflow.png

1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object.
3. Application can request for terrestrial positioning information with getTerrestrialPosition API.
4. Status of getTerrestrialPosition i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for getTerrestrialPosition is received by the application.
6. Single shot terrestrial position information is received by the application.

Call flow to cancel terrestrial positioning information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_manager_cancel_terrestrial_positioning_callflow.png

1. Application requests location factory for location manager object.
2. Location factory returns ILocationManager object.
3. Application can cancel request for terrestrial positioning information with cancelTerrestrialPositionRequest API.
4. Status of cancelTerrestrialPositionRequest i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for cancelTerrestrialPositionRequest is received by the application.

Call flow to enable/disable constraint time uncertainty
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_constraint_tunc_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application enables/disables constraint tunc using configureCTunc API.
4. Status of configureCTunc i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureCTunc is received by the application.

Call flow to enable/disable PACE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_pace_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application enables/disables PACE using configurePACE API.
4. Status of configurePACE i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configurePACE is received by the application.

Call flow to delete all aiding data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_delete_aiding_data_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application deletes all aiding data using deleteAllAidingData API.
4. Status of deleteAllAidingData i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for deleteAllAidingData is received by the application.

Call flow to configure lever arm parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_lever_arm_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures lever arm parameters using configureLeverArm API.
4. Status of configureLeverArm i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureLeverArm is received by the application.

Call flow to configure blacklisted constellations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_constellation_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures blacklisted constellations using configureConstellations API.
4. Status of configureConstellations i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureConstellations is received by the application.

Call flow to configure robust location
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_robust_location_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures robust location using configureRobustLocation API.
4. Status of configureRobustLocation i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureRobustLocation is received by the application.

Call flow to configure min gps week
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_min_gps_week_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures min gps week using configureMinGpsWeek API.
4. Status of configureMinGpsWeek i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureMinGpsWeek is received by the application.

Call flow to request min gps week
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_request_min_gps_week_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application requests min gps week using requestMinGpsWeek API.
4. Status of requestMinGpsWeek i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for requestMinGpsWeek is received by the application.

Call flow to delete specified data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_delete_aiding_data_warm_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application requests delete specified data using deleteAidingData API.
4. Status of deleteAidingData i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for deleteAidingData is received by the application.

Call flow to configure min sv elevation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_min_sv_elevation_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures min SV elevation using configureMinSVElevation API.
4. Status of configureMinSVElevation i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureMinSVElevation is received by the application.

Call flow to request min sv elevation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_request_min_sv_elevation_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application requests min SV elevation using requestMinSVElevation API.
4. Status of requestMinSVElevation i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for requestMinSVElevation is received by the application.

Call flow to request robust location
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_request_robust_location_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application requests robust location using requestRobustLocation API.
4. Status of requestRobustLocation i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for requestRobustLocation is received by the application.

Call flow to configure dead reckoning engine
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_dr_engine_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures dead reckoning engine using configureDR API.
4. Status of configureDR i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureDR is received by the application.

Call flow to configure secondary band
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_secondary_band_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures secondary band using configureSecondaryBand API.
4. Status of configureSecondaryBand i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureSecondaryBand is received by the application.

Call flow to request secondary band
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_request_secondary_band_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application requests secondary band using requestSecondaryBandConfig API.
4. Status of requestSecondaryBandConfig i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for requestSecondaryBandConfig is received by the application.

Call flow to configure engine state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_engine_state_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures engine state using configureEngineState API.
4. Status of configureEngineState i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureEngineState is received by the application.

Call flow to provide user consent for terrestrial positioning
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_provide_user_consent_terrestrial_positioning_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application provides user consent for terrestrial positioning using provideConsentForTerrestrialPositioning API.
4. Status of provideConsentForTerrestrialPositioning i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for provideConsentForTerrestrialPositioning is received by the application.

Call flow to configure NMEA sentence type
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_configure_nmea_types_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application configures NMEA sentence type using configureNmeaTypes API.
4. Status of configureNmeaTypes i.e. either SUCCESS or FAILED will be returned to the application.
5. The response for configureNmeaTypes is received by the application.

Call flow to represent Xtra Feature
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/location_configurator_xtra_callflow.png

1. Application requests location factory for location configurator object.
2. Location factory returns ILocationConfigurator object.
3. Application registers config listener for Xtra Status indications.
4. Listener API onXtraStatusUpdate is invoked while registering for Xtra indications.
5. Application configures Xtra Params using configureXtraParams.
6. Location Configurator invokes response callback with errorcode.
7. Listener API onXtraStatusUpdate is invoked when Xtra feature is enabled/disabled.
8. Application requests Xtra Status using requestXtraStatus.
9. Location Configurator invokes GetXtraStatus callback with XtraStatus and errorcode.
10. Application deregisters config listener from Xtra Status indications.

Call flow to inject RTCM correction data with dgnss manager
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/dgnss_call_flow.png

1. Application requests location factory for Dgnss manager object.
2. Location factory create IDgnssManager instance and perform initialization.
3. Location factory returns IDgnssManager object.
4. Application register a status listener to get notification of the dgnss manager status change.
5. Application start injecting RTCM data.
6. If the status listener received any error notification, it perform desired operation.
7. An example case is the listener received DATA_SOURCE_NOT_USABLE notification, it then release the
   current source and create a new source and perform RTCM data injection.


Data Services
-------------

Applications need to have "radio" Linux group permissions to be able to operate successfully with underlying services.

Start/Stop for data connection manager call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/data_start_stop_data_call_callflow.png

1. Application requests Data Connection Manager object associated with sim id from data factory.
   Application can optionally provide callback to be called when manager initialization is completed.
2. Data factory returns shared pointer to data connection manager object to application.
3. Application request current service status of data connection manager returned by data factory.
4. Data connection manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Data connection manager calls application callback with initialization result (success/failure).

5. Application registers as listener to get notifications for data call change.
6. The application receives the status i.e. either SUCCESS or FAILED based on the registration of the listener.
7. Application requests for start data call and optionally gets asynchronous response using startDataCallback.
8. Application receives the status i.e. either SUCCESS or FAILED based on the execution of startDataCall.
9. Optionally, the application gets asynchronous response for startDataCall using startDataCallback.
10. Application requests for stop data call and optionally gets asynchronous response using stopDataCallback.
11. Application receives the status i.e. either SUCCESS or FAILED based on the execution of stopDataCall.
12. Optionally, the application gets asynchronous response for stopDataCall using stopDataCallback.

Request data profile list call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/data_request_profile_list_call_flow.png

1. Application requests Data profile Manager object associated with sim id from data factory.
   Application can optionally provide callback to be called when manager initialization is completed.
2. Data factory returns shared pointer to data profile manager object to application.
3. Application request current service status of data profile manager returned by data factory.
4. Data profile manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Data profile manager calls application callback with initialization result (success/failure).

5. Application creates listener class of type IDataProfileListener
6. Application register created object in step 5 as listener to data profile changes
7. Application receives the status i.e. either SUCCESS or FAILED based on the execution of registerListener
8. Application requests list of profile
9. Application receives the status i.e. either SUCCESS or FAILED based on the execution of requestProfileList
10. Application gets callback with list of all profiles

Data Serving System Manager Call Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Data Serving System manager provides the interface to access network and modem low level services.
It provides APIs to get current dedicated radio bearer, get current service status and preferred RAT, and get current roaming status.
Serving System Listener provides an interface for application to receive data serving system notification events such as
change in dedicated radio bearer, change in service status, or change in roaming status.
The application must register as a listener for Serving System updates.

Get Dedicated Radio Bearer Call Flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_get_drb_status_call_flow.png

1. Application requests data factory for data serving system manager object with slot Id and init callback.
2. Data factory returns IServingSystemManager object to application.
3. Serving System Manager calls application callback provided in step 1 with initialization result pass/fail.
4. Application register itself as listener with Serving System manager to receive dedicated radio bearer changes notification.
5. Application gets success for registering as listener.
6. Application calls getDrbStatus to get current dedicated radio bearer status.
7. Application receives the current dedicated bearer status.
8. Dedicated radio bearer changes in modem
9. Application gets onDrbStatusChanged indication with new status

Request Service Status Call Flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_request_service_status_call_flow.png

1. Application requests data factory for data serving system manager object with slot Id and init callback.
2. Data factory returns IServingSystemManager object to application.
3. Serving System Manager calls application callback provided in step 1 with initialization result pass/fail.
4. Application register itself as listener with Serving System manager to receive service status changes notification.
5. Application gets success for registering as listener.
6. Application calls requestServiceStatus to get current service status and provides callback.
7. Application receives the status i.e. either SUCCESS or FAILED based on the execution of requestServiceStatus.
8. Application gets asynchronous response for requestServiceStatus through callback provided in step 4 with current service status.
9. Service status changes in modem
10. Application gets onServiceStateChanged indication with new status

Request Roaming Status Call Flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_request_roaming_status_call_flow.png

1. Application requests data factory for data serving system manager object with slot Id and init callback.
2. Data factory returns IServingSystemManager object to application.
3. Serving System Manager calls application callback provided in step 1 with initialization result pass/fail.
4. Application register itself as listener with Serving System manager to receive roaming status changes notification.
5. Application gets success for registering as listener.
6. Application calls requestRoamingStatus to get current service status and provides callback.
7. Application receives the status i.e. either SUCCESS or FAILED based on the execution of requestRoamingStatus.
8. Application gets asynchronous response for requestRoamingStatus through callback provided in step 4 with current service status.
9. Roaming status changes in modem
10. Application gets onRoamingStatusChanged indication with new status

Data Filter Manager Call Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Data Filter manager provides APIs to get/set data filter mode, add/remove data restrict filters.
Its API can used per data call or globally to apply the same changes to all the underlying
currently up data call. It also has listener interface for notifications for data filter status update.
Application will get the Data Filter manager object from data factory.
The application can register a listener for data filter mode change updates.

Call flow to Set/Get data filter mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/get_and_set_data_filter_mode_call_flow.png

1. Application requests data factory for data connection manager object.
2. Data factory returns IDataConnectionManager object to application.
3. Application requests data factory for data filter manager object.
4. Data factory returns IDataFilterManager object to application.
5. Application requests for start data call and optionally gets asynchronous response using startDataCallback.
6. Application receives the status i.e. either SUCCESS or FAILED based on the execution of startDataCall.
7. Optionally, the application gets asynchronous response for startDataCall using startDataCallback.
8. Application requests for set data filter mode to enable and optionally gets asynchronous response using ResponseCallback.
9. Application receives the status i.e. either SUCCESS or FAILED based on the execution of setDataRestrictMode.
10. Optionally, the application gets asynchronous response for setDataRestrictMode using ResponseCallback.
11. Application requests for get data filter mode and optionally gets asynchronous response using DataRestrictModeCb.
12. Application receives the status i.e. either SUCCESS or FAILED based on the execution of requestDataRestrictMode.
13. Optionally, the application gets asynchronous response for requestDataRestrictMode using DataRestrictModeCb.

Call flow to Add data restrict filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/add_data_filter_call_flow.png

1. Application requests data factory for data connection manager object.
2. Data factory returns IDataConnectionManager object to application.
3. Application requests data factory for data filter manager object.
4. Data factory returns IDataFilterManager object to application.
5. Application requests for start data call and optionally gets asynchronous response using startDataCallback.
6. Application receives the status i.e. either SUCCESS or FAILED based on the execution of startDataCall.
7. Optionally, the application gets asynchronous response for startDataCall using startDataCallback.
8. Application requests for add data filter and optionally gets asynchronous response using ResponseCallback.
9. Application receives the status i.e. either SUCCESS or FAILED based on the execution of addDataRestrictFilter.
10. Optionally, the application gets asynchronous response for addDataRestrictFilter using ResponseCallback.

Call flow to Remove data restrict filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/remove_data_filter_call_flow.png

1. Application requests data factory for data connection manager object.
2. Data factory returns IDataConnectionManager object to application.
3. Application requests data factory for data filter manager object.
4. Data factory returns IDataFilterManager object to application.
5. Application requests for start data call and optionally gets asynchronous response using startDataCallback.
6. Application receives the status i.e. either SUCCESS or FAILED based on the execution of startDataCall.
7. Optionally, the application gets asynchronous response for startDataCall using startDataCallback.
8. Application requests for add data filter and optionally gets asynchronous response using ResponseCallback.
9. Application receives the status i.e. either SUCCESS or FAILED based on the execution of removeAllDataRestrictFilters.
10. Optionally, the application gets asynchronous response for removeAllDataRestrictFilters using ResponseCallback.

Data Networking Call Flow
~~~~~~~~~~~~~~~~~~~~~~~~~

Application will get the following manager objects from data factory to configure networking.
IVlanManager is used to access all VLAN APIs.
INatManager is used to access all Static NAT APIs.
IFirewallManager is used to access all Firewall APIs.

Create VLAN and Bind it to PDN in data vlan manager call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_create_and_bind_vlan_call_flow.png

1. Application requests data factory for data IVlanManager object.
   Application can optionally provide callback to be called when manager initialization is completed.

   a. If IVlanManager object does not exist, data factory will create new object.

2. Data factory returns shared pointer to IVlanManager object to application.
3. Application request current service status of vlan manager returned by data factory
4. The application receives the Status i.e. either true or false to indicate whether sub-system is ready or not.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1
   b. Vlan manager calls application callback with initialization result (success/failure).

5. On success, application calls IVlanManager::createVlan with assigned id, interface, and acceleration type.
6. Application receives synchronous Status which indicates if the IVlanManager::createVlan request was sent successfully.
7. Application is notified of the Status of the IVlanManager::createVlan request (either SUCCESS or FAILED) via the application-supplied callback.
8. Application calls IVlanManager::bindWithProfile with Vlan id and profile id.
9. Application receives synchronous Status which indicates if the IVlanManager::bindWithProfile request was sent successfully.
10. Application is notified of the Status of the IVlanManager::bindWithProfile request (either SUCCESS or FAILED) via the application-supplied callback.

LAN-LAN VLAN Configuration from EAP usecase call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_lan2lan_vlan_config_from_eap.png

1. Application requests data factory for data IVlanManager for local operation object.
   Aplication can optionally provide callback to be called when manager initialization is completed.
2. Data factory returns shared pointer to local vlan manager object to application.
3. Application request current service status of local vlan manager returned by data factory.
4. Vlan manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Vlan manager calls application callback with initialization result (success/failure).

5. On success, application calls IVlanManager::createVlan with USB interface, Vlan id 1 and no acceleration.
6. Vlan manager returns synchronous response to application (success/fail).
7. Vlan manager calls application provided callback in step 5 with createVlan results
8. Application requests data factory for data IVlanManager for remote operation object.
   Aplication can optionally provide callback to be called when manager initialization is completed.
9. Data factory returns shared pointer to remote vlan manager object to application.
10. Application request current service status of remote vlan manager returned by data factory.
11. Vlan manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Vlan manager calls application callback with initialization result (success/failure).

12. On success, application calls IVlanManager::createVlan with USB interface, Vlan id 1 and acceleration.
13. Vlan manager returns synchronous response to application (success/fail).
14. Vlan manager calls application provided callback in step 12 with createVlan results.
15. Application calls IVlanManager::createVlan with ETH interface, Vlan id 1 and acceleration.
16. Vlan manager returns synchronous response to application (success/fail).
17. Vlan manager calls application provided callback in step 15 with createVlan results.

LAN-LAN VLAN Configuration from A7 usecase call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_lan2lan_vlan_config_from_a7.png

1. Application requests data factory for data IVlanManager for local operation object.
   Aplication can optionally provide callback to be called when manager initialization is completed.
2. Data factory returns shared pointer to local vlan manager object to application.
3. Application request current service status of local vlan manager returned by data factory.
4. Vlan manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Vlan manager calls application callback with initialization result (success/failure).

5. On success, application calls IVlanManager::createVlan with USB interface, Vlan id 1 and acceleration.
6. Vlan manager returns synchronous response to application (success/fail).
7. Vlan manager calls application provided callback in step 5 with createVlan results.
8. Application calls IVlanManager::createVlan with ETH interface, Vlan id 1 and acceleration.
9. Vlan manager returns synchronous response to application (success/fail).
10. Vlan manager calls application provided callback in step 8 with createVlan results.

LAN-WAN VLAN Configuration from EAP usecase call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_lan2wan_vlan_config_from_eap.png

1. Application requests data factory for local data vlan manager object.
2. Data factory returns shared pointer to local vlan manager to application.
3. Application request current service status of local vlan manager returned by data factory.
4. Vlan manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Vlan manager calls application callback with initialization result (success/failure).

5. On success, application calls IVlanManager::createVlan with USB interface, Vlan id 1 and no acceleration.
6. Vlan manager returns synchronous response to application (success/fail).
7. Vlan manager calls application provided callback in step 5 with createVlan results.
8. Application requests data factory for remote data vlan manager object.
9. Data factory returns shared pointer to remote vlan manager to application.
10. Application request current service status of remote vlan manager returned by data factory.
11. Vlan manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 8.
   b. Vlan manager calls application callback with initialization result (success/failure).

12. On success, application calls IVlanManager::createVlan with USB interface, Vlan id 1 and acceleration.
13. Vlan manager returns synchronous response to application (success/fail).
14. Vlan manager calls application provided callback in step 12 with createVlan results.
15. Application calls IVlanManager::createVlan with ETH interface, Vlan id 1 and acceleration.
16. Vlan manager returns synchronous response to application (success/fail).
17. Vlan manager calls application provided callback in step 15 with createVlan results.
18. Application calls IVlanManager::bindWithProfile with profile id to bind with.
19. Vlan manager returns synchronous response to application (success/fail).
20. Vlan manager calls application provided callback in step 18 with bindWithProfile results.

If DMZ is needed:

21. Application requests data factory for firewall manager object.
22. Data factory returns shared pointer to firewall manager to application.
23. Application request current service status of firewall manager returned by data factory.
24. Firewall manager returns current service status.

    a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
       provided in step 21.
    b. Firewall manager calls application callback with initialization result (success/failure).

25. Application calls firewall manager enableDmz with profile id and local address to be enable Dmz on.
26. Firewall manager returns synchronous response to application (success/fail).
27. Firewall manager calls application provided callback in step 25 with enableDmz results.
28. Application requests data factory for data connection manager object.
29. Data factory returns shared pointer to data connection manager to application.
30. Application request current service status of data connection manager returned by data factory.
31. Data connection manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 28.
   b. Data connection manager calls application callback with initialization result (success/failure).

32. Application can call data Connection manager startDataCall with profile id to start data call on, Ip Family type, operation Type and APN Name.
33. Data connection Manager returns synchronous response to application (success/fail).
34. Data connection Manager returns notification to application with data call details.

LAN-WAN VLAN Configuration from A7 usecase call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_lan2wan_vlan_config_from_a7.png

1. Application requests data factory for local data vlan manager object.
2. Data factory returns shared pointer to local vlan manager to application.
3. Application request current service status of local vlan manager returned by data factory.
4. Vlan manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Vlan manager calls application callback with initialization result (success/failure).

5. On success, application calls IVlanManager::createVlan with USB interface, Vlan id 1 and acceleration.
6. Vlan manager returns synchronous response to application (success/fail).
7. Vlan manager calls application provided callback in step 5 with createVlan results.
8. Application calls IVlanManager::createVlan with ETH interface, Vlan id 1 and acceleration.
9. Vlan manager returns synchronous response to application (success/fail).
10. Vlan manager calls application provided callback in step 8 with createVlan results.
11. Application calls IVlanManager::bindWithProfile with profile id to bind with.
12. Vlan manager returns synchronous response to application (success/fail).
13. Vlan manager calls application provided callback in step 11 with bindWithProfile results.

If DMZ is needed:

14. Application requests data factory for firewall manager object.
15. Data factory returns shared pointer to firewall manager to application.
16. Application request current service status of firewall manager returned by data factory.
17. Firewall manager returns current service status.

    a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
       provided in step 14.
    b. Firewall manager calls application callback with initialization result (success/failure).

18. Application calls firewall manager enableDmz with profile id and local address to be enable Dmz on.
19. Firewall manager returns synchronous response to application (success/fail).
20. Firewall manager calls application provided callback in step 25 with enableDmz results.
21. Application requests data factory for data connection manager object.
22. Data factory returns shared pointer to data connection manager to application.
23. Application request current service status of data connection manager returned by data factory.
24. Data connection manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 21.
   b. Data connection manager calls application callback with initialization result (success/failure).

25. Application can call data Connection manager startDataCall with profile id to start data call on, Ip Family type, operation Type and APN Name.
26. Data connection Manager returns synchronous response to application (success/fail).
27. Data connection Manager returns notification to application with data call details.

Create Static NAT entry in data Static NAT manager call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_create_snat_entry_call_flow.png

1. Application requests data factory for data nat manager object.
   a. If nat manager object does not exist, data factory will create new object.
2. Data factory returns shared pointer to nat manager object to application.
3. Application request current service status of nat manager returned by data factory.
4. Nat manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Nat manager calls application callback with initialization result (success/failure).

5. On success, application calls nat manager addStaticNatEntry with profileId, private IP address port, private port, global port and IP Protocol.
6. Application receives synchronous Status which indicates if the nat manager addStaticNatEntry request was sent successfully.
7. Application is notified of the result of the nat manager addStaticNatEntry request (either SUCCESS or FAILED) via the application-supplied callback.

Firewall Enablement in data Firewall manager call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_enable_disable_firewall_call_flow.png

1. Application requests data factory for data firewall manager object.

   a. If firewall manager object does not exist, data factory will create new object.

2. Data factory returns shared pointer to firewall manager object to application.
3. Application request current service status of firewall manager returned by data factory.
4. Firewall manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Firewall manager calls application callback with initialization result (success/failure).

5. On success, application calls firewall manager setFirewall with enable/disable and allow/drop packets.
6. Application receives synchronous Status which indicates if the firewall manager setFirewall request was sent successfully.
7. Application is notified of the Status of the firewall manager setFirewall request (either SUCCESS or FAILED) via the application-supplied callback.

Add Firewall Entry in data Firewall manager call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_add_firewall_entry_call_flow.png

1. Application requests data factory for data firewall manager object.

   a. If firewall manager object does not exist, data factory will create new object.

2. Data factory returns shared pointer to firewall manager object to application.
3. Application request current service status of data profile manager returned by data factory.
4. Firewall manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Firewall manager calls application callback with initialization result (success/failure).

5. On success, application calls firewall manager getNewFirewallEntry to get FirewallEntry object.
6. Application receives Firewall Entry object.
7. Using Firewall Entry object, application calls IFirewallEntry::getIProtocolFilter to get protocol filter object
8. Application receives IpFilter object.
9. Application populates FirewallEntry and IpFilter objects.
10. Application calls IFirewallManager::addFirewallEntry with profileId and FirewallEntry to add firewall entry
11. Application receives synchronous Status which indicates if addFirewallEntry was sent successfully
12. Application is notified of the Status of the IFirewallManager::addFirewallEntry request (either SUCCESS or FAILED) via the application-supplied callback.


Set Firewall DMZ in data Firewall manager call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_create_firewall_dmz_call_flow.png

1. Application requests data factory for data firewall manager object.

   a. If firewall manager object does not exist, data factory will create new object.

2. Data factory returns shared pointer to firewall manager object to application.
3. Application request current service status of firewall manager returned by data factory.
4. Firewall manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Firewall manager calls application callback with initialization result (success/failure).

5. On success, application calls firewall manager enableDmz with profileId and IP Address.
6. Application receives synchronous Status which indicates if the firewall manager enableDmz request was sent successfully.
7. Application is notified of the Status of the firewall manager enableDmz request (either SUCCESS or FAILED) via the application-supplied callback.

Socks Enablement in data Socks manager call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_enable_disable_Socks_call_flow.png

1. Application requests data factory for data socks manager object.

   a. If socks manager object does not exist, data factory will create new object.

2. Data factory returns shared pointer to socks manager object to application.
3. Application request current service status of socks manager returned by data factory.
4. Socks manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Socks manager calls application callback with initialization result (success/failure).

5. On success, application calls socks manager enableSocks with enable/disable.
6. Application receives synchronous Status which indicates if the socks manager enableSocks request was sent successfully.
7. Application is notified of the Status of the socks manager enableSocks request (either SUCCESS or FAILED) via the application-supplied callback.

L2TP Enablement and Configuration in data L2TP manager call flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_enable_disable_l2tp_and_create_tunnel_session.png

1. Application requests data factory for data l2tp manager object.

   a. If l2tp manager object does not exist, data factory will create new object.

2. Data factory returns shared pointer to l2tp manager object to application.
3. Application request current service status of l2tp manager returned by data factory.
4. L2tp manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
       provided in step 1.
   b. L2tp manager calls application callback with initialization result (success/failure).

5. On success, application calls l2tp manager setConfig with enable/disable, enable/disable Mss, enable/disable MTU size and MTU size
6. Application receives synchronous Status which indicates if the l2tp manager setConfig request was sent successfully.
7. Application is notified of the Status of the l2tp manager setConfig request (either SUCCESS or FAILED) via the application-supplied callback.
8. Application calls l2tp manager setConfig with all required configurations to setup tunnel and session
9. Application receives synchronous Status which indicates if the l2tp manager setConfig request was sent successfully.
10. Application is notified of the Status of the l2tp manager setConfig request (either SUCCESS or FAILED) via the application-supplied callback.

Call flow to add and enable software bridge
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_software_bridge_add_enable.png

1. Application requests data factory for bridge manager object.
2. Data factory returns shared pointer to bridge manager object to application.
3. Application request current service status of bridge manager returned by data factory.
4. Bridge manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Bridge manager calls application callback with initialization result (success/failure).

5. On success, application requests to add software bridge configuration for an interface, providing an optional asynchronous response callback using addBridge API.
6. Application receives the synchronous status i.e. either SUCCESS or FAILED which indicates if the request was sent successfully.
7. Optionally, the application gets asynchronous response for addBridge via the application-supplied callback.
8. If the software bridge management is not enabled already, application requests to enable it, providing an optional asynchronous response callback using enableBridge API.

   .. note::
      Please note that this step affects all the software bridges configured in the system.

9. Application receives the status i.e. either SUCCESS or FAILED which indicates if the request was sent successfully.
10. Optionally, the application gets asynchronous response for enableBridge via the application-supplied callback.

Call flow to remove and disable software bridge
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_software_bridge_remove_disable.png

1. Application requests data factory for bridge manager object.
2. Data factory returns shared pointer to bridgeManager object to application.
3. Application request current service status of bridge manager returned by data factory.
4. Bridge manager returns current service status.

   a. If status returned is SERVICE_UNAVAILABLE (manager is not ready), application should wait for init callback
      provided in step 1.
   b. Bridge manager calls application callback with initialization result (success/failure).

5. On success, application requests to get the list of software bridge configurations, providing an asynchronous response callback using requestBridgeInfo API.
6. Application receives the synchronous status i.e. either SUCCESS or FAILED which indicates if the request was sent successfully.
7. The application gets asynchronous response for requestBridgeInfo via the application-supplied callback.
8. Application requests to remove software bridge configuration for an interface, providing an optional asynchronous response callback using removeBridge API.
9. Application receives the synchronous status i.e. either SUCCESS or FAILED which indicates if the request was sent successfully.
10. Optionally, the application gets asynchronous response for removeBridge via the application-supplied callback.
11. If the software bridge management needs to be disabled, application requests to disable it, providing an optional asynchronous response callback using enableBridge API.

    .. note::
       Please note that this step affects all the software bridges configured in the system.

12. Application receives the status i.e. either SUCCESS or FAILED which indicates if the request was sent successfully.
13. Optionally, the application gets asynchronous response for enableBridge via the application-supplied callback.

Call flow to enable ip passthrough in peer nad
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_enable_ip_pass_through_call_flow.png

1. Client of NAD-1 and NAD-2 requests the vlan manager and data settings manager object.
   Additionally, NAD-2 also requests the data connection manager object.
2. The client of NAD-1 registers as listener to get notifications for data call change.
3. When the subsystem is ready, the client of NAD-1 requests NAD-2's client to establish a VLAN for
   a LAN network type which is acting as a gateway.
4. The client of NAD-1 requests to start data call in NAD-2 and may optionally recieve an
   asynchronous response using a callback. When the data call is connected, TelSDK(NAD-2) notifies
   its client(NAD-2).
5. The NAD-1 client requests the NAD-2 client to bind the data call profile id with the vlan id.
6. A request to enable IP passthrough is being sent from the NAD-1 client to NAD-2. At this point,
   the client has successfully started a data call and enabled a IP passthrough configuration in
   NAD-2.
7. NAD-1 creates vlan for a LAN network. type that is connected to the main unit.
8. NAD-1 creates another vlan for a WAN network. type that is connected to the ETH backhaul.
9. NAD-1 client calls IVlanManager::bindToBackhaul API to bind both LAN and WAN type of vlans and
   the data call in NAD-2 is routed through the NAD-2 vlan(which is act as gateway), NAD-1 WAN vlan
   (which is connected to the ETH backhaul) and NAD-1 LAN vlan(which is connected to the main unit).
10. The client of NAD-1 enables the IP address configuration to its WAN vlan that is connected to
    the ETH backhaul that allows main unit to access data call running in NAD-2.

Call flow to set data stall parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/data_set_data_stall_parameters_call_flow.png

1. Application requests a data factory for the data control manager object.
2. Data factory returns a shared pointer to the data control manager object to the application.
3. Application requests the current service status of the data control manager returned by the data factory.
4. Data control manager returns current service status.

   a. If the status returned is SERVICE_UNAVAILABLE (manager is not ready), the application should wait for the init callback provided in step 1.
   b. Data control manager calls the application callback with the initialization result (success/failure).

5. On success, the application sets data stall parameters for a specific slot ID.

Call flow to set Ethernet data link state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/set_eth_datalink_state.png

1. Application requests a data factory for the data link manager object.
2. Data factory returns a shared pointer to the data link manager object to the application.
3. When the subsystem is ready, application registers as listener to receive notifications for Ethernet data link state changes.
4. Application brings up the Ethernet data link state.
5. The change in Ethernet data link state (UP) is notified to the application.
6. Application brings down the ethernet data link state.
7. The change in Ethernet data link state (DOWN) is notified to the application.

C-V2X
-----

Applications need to have "radio" Linux group permissions to be able to operate successfully with underlying services.

Retrieve/Update C-V2X Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_config_call_flow.png

   **Retrieve/Update C-V2X Configuration Call Flow - C++ Version**

This call flow diagram describes the sequence of steps for retrieving or updating C-V2X configuration file using C++ version APIs.

1. Application requests for a ICv2xFactory instance.
2. Reference to singleton ICv2xFactory is returned to application.
3. Application requests C-V2X factory for a ICv2xConfig instance.
4. C-V2X factory creates ICv2xConfig object and calls init() method of ICv2xConfig.
5. C-V2X config starts initialization asynchronously.
6. C-V2X factory return ICv2xConfig object to application.
7. C-V2X factory is asynchronously notified of the readiness status of the C-V2X config via the initialization callback.
8. C-V2X factory calls application-supplied callback to notify the readiness status of C-V2X config (either SERVICE_AVAILABLE or SERVICE_FAILED). If the status is SERVICE_AVAILABLE, application can then request to retrieve or update C-V2X configuration.
9. Application requests to retrieve C-V2X configuration by calling retrieveConfiguration and supplying it with a path for the storing of config XML file.
10. C-V2X config sends request to modem and waits for response asynchronously.
11. Application receives synchronous status.
12. Application is asynchronously notified of the status of the request (either SUCCESS or FAILED) via the application-supplied callback.
13. Application requests to update C-V2X configuration by calling updateConfiguration and supplying it with a path to the new config XML file.
14. C-V2X config sends request to modem and waits for response asynchronously.
15. Application receives synchronous status.
16. Application is asynchronously notified of the status of the request (either SUCCESS or FAILED) via the application-supplied callback.
17. Application registers ICv2xConfigListener to get notification of C-V2X configuration events if needed.
18. Application receives synchronous status.
19. Application gets notification of C-V2X configuration events if the C-V2X configuration being used in the system has been changed or expired.
20. Application deregisters ICv2xConfigListener to stop listening to C-V2X configuration events.
21. Application receives synchronous status.


.. figure:: /../images/cv2x_config_call_flow_c.png

   **Retrieve/Update C-V2X Configuration Call Flow - C Version**

This call flow diagram describes the sequence of steps for retrieving or updating C-V2X configuration file using C version APIs.

1. Application requests to retrieve C-V2X configuration by calling v2x_retrieve_configuration and supplying it with a path for the storing of config XML file.
2. Application receives synchronous status.
3. Application requests to update C-V2X configuration by calling v2x_update_configuration and supplying it with a path to the new config file.
4. Application receives synchronous status.
5. Application registers cv2x_config_event_listener to get notification of C-V2X configuration events if needed.
6. Application receives synchronous status.
7. Application gets notification of C-V2X configuration events if the C-V2X configuration being used in the system has been changed or expired.

Start/Stop C-V2X Mode
~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_start_stop_call_flow.png

   **Start/Stop C-V2X Mode Call Flow - C++ Version**


This call flow diagram describes the sequence of steps for starting or stopping C-V2X mode using C++ version APIs.
Application must perform C-V2X radio manager initialization before calling any methods of ICv2xRadioManager. In normal operation, applications do not need to start or stop C-V2X mode. The system is configured by default to start C-V2X mode at boot. We include the call flow below for the sake of completeness.

1. Application requests to put modem into C-V2X mode using startCv2x method.
2. Application receives synchronous status which indicates if the start request was sent successfully.
3. Application is notified of the status of the start request (either SUCCESS or FAILED) via the application-supplied callback.
4. Application requests to disable C-V2X mode using stopCv2x method.
5. Application receives synchronous status which indicates if the stop request was sent successfully.
6. Application is asynchronously notified of the status of the stop request (either SUCCESS or FAILED) via the application-supplied callback.

.. figure:: /../images/cv2x_start_stop_call_flow_c.png

   **Start/Stop C-V2X Mode Call Flow - C Version**

This call flow diagram describes the sequence of steps for starting or stopping C-V2X mode using C version APIs.

1. Application requests to put modem into C-V2X mode using start_v2x_mode method.
2. Application receives synchronous status which indicates if the operation was successful.
3. Application requests to disable C-V2X mode using stop_v2x_mode method.
4. Application receives synchronous status which indicates if the operation was successful.


C-V2X Radio Control Flow
~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_radio_ctrl_flow.png

   **C-V2X Radio Control Flow - C++ Version**

This call flow diagram describes the sequence of steps for overall C-V2X radio control flow using C++ version APIs.

1. Application performs C-V2X radio manager initialization and waits for the readiness. See steps 1~8 in @ref fig_cv2x_radio_initialization_callflow.
2. Application registers ICv2xListener to get C-V2X status update notification and requests for the initiate C-V2X status. See steps 1~6 in @ref fig_cv2x_get_status_callflow.
3. Application waits for C-V2X status to go to SUSPEND/ACTIVE.
4. Application performs C-V2X radio initialization and waits for the readiness. See steps 9~17 in @ref fig_cv2x_radio_initialization_callflow.
5. Application registers C-V2X radio listener to get C-V2X radio related notifications (L2 address update, SPS offset update, SPS scheduling update, C-V2X radio capabilities update) and requests for the initial C-V2X radio capabilities if needed. See steps 1~6 in @ref fig_cv2x_get_capabilities_callflow.
6. Application registers all Tx/Rx flows. See @ref fig_cv2x_radio_rx_sub_callflow/@ref fig_cv2x_radio_event_flow_callflow/@ref fig_cv2x_radio_sps_flow_callflow.
7. Application monitors C-V2X status change during operation.

   - 7.1 If C-V2X Tx/Rx status is active, application performs transmitting/receiving.
   - 7.2 If C-V2X Tx/Rx status goes to SUSPEND, application should suspend transmitting/receiving, log the event and resume operation when C-V2X Tx/Rx status goes to ACTIVE again.
   - 7.3 If C-V2X Tx/Rx status goes to INACTIVE, application should stop transmitting/receiving, log the event and go back to step 3 for recovery.

8. Application should handle all trappable signals like SIGINT/SIGHUP/SIGTERM, all Tx/Rx flows must be deregistered before exiting.

.. figure:: /../images/cv2x_radio_ctrl_flow_c.png

   **C-V2X Radio Control Flow - C Version**


This call flow diagram describes the sequence of steps for overall C-V2X radio control flow using C version APIs.

1. Application performs C-V2X radio initialization and waits for the readiness in application provided callback. See @ref fig_cv2x_radio_initialization_callflow_c. Application should check the C-V2X radio initialization status, retry or exit if failing.
2. Application registers v2x_ext_radio_status_listener to get C-V2X status update notification and requests for the initiate C-V2X status. See steps 1~5 in @ref fig_cv2x_get_status_callflow_c.
3. Application waits for C-V2X status to go to SUSPEND/ACTIVE.
4. Application requests for the initial C-V2X radio capabilities if needed. See @ref fig_cv2x_get_capabilities_callflow_c.
5. Application registers all Tx/Rx flows. See @ref fig_cv2x_radio_rx_sub_callflow_c/@ref fig_cv2x_radio_event_flow_callflow_c/@ref fig_cv2x_radio_sps_flow_callflow_c.
6. Application monitors C-V2X status change during operation.

   - 6.1 If C-V2X Tx/Rx status is active, application performs transmitting/receiving.
   - 6.2 If C-V2X Tx/Rx status goes to SUSPEND, application should suspend transmitting/receiving, log the event and resume operation when C-V2X Tx/Rx status goes to ACTIVE again.
   - 6.3 If C-V2X Tx/Rx status goes to INACTIVE, application should stop transmitting/receiving, log the event, wait for C-V2X Tx/Rx status to go to SUSPEND or ACTIVE and then go back to step 1 for recovery.

7. Application should handle all trappable signals like SIGINT/SIGHUP/SIGTERM, all Tx/Rx flows must be deregistered before exiting.


C-V2X Radio Initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_radio_init_call_flow.png

   **C-V2X Radio Initialization Call Flow - C++ Version**


This call flow diagram describes the sequence of steps for initializing the ICv2xRadioManager and the ICv2xRadio object using C++ version APIs. Applications must initialize ICv2xRadioManager/ICv2xRadio object and wait for the readiness before calling any other methods on the objects.

1. Application requests for a ICv2xFactory instance.
2. Reference to singleton ICv2xFactory is returned to application.
3. Application requests C-V2X factory for an ICv2xRadioManager instance.
4. C-V2X factory creates ICv2xRadioManager object and calls init() method of ICv2xRadioManager.
5. C-V2X radio manager starts initialization asynchronously.
6. C-V2X factory returns ICv2xRadioManager object to application.
7. C-V2X factory is asynchronously notified of the readiness status of the C-V2X radio manager via the initialization callback.
8. C-V2X factory calls application-supplied callback to notify the readiness status of C-V2X radio manager (either SERVICE_AVAILABLE or SERVICE_FAILED).
9. Application requests C-V2X Radio from ICv2xRadioManager.
10. C-V2X radio manager creates ICv2xRadio object and calls method init().
11. C-V2X radio starts initialization asynchronously.
12. C-V2X radio manager returns ICv2xRadio object to application.
13. C-V2X radio manager is asynchronously notified of the readiness status of the C-V2X radio via the initialization callback.
14. C-V2X radio manager calls application-supplied callback to notify the readiness status of C-V2X radio (either SERVICE_AVAILABLE or SERVICE_FAILED).

.. figure:: /../images/cv2x_radio_init_call_flow_c.png

   **C-V2X Radio Initialization Call Flow - C Version**


This call flow diagram describes the sequence of steps for initialing C-V2X radio using C version APIs.
Applications must initialize C-V2X radio and wait for the readiness before calling any other methods of C-V2X radio.

1. Application calls v2x_radio_init_v3 to initialize C-V2X radio manager and radio, provides callback functions as needed. Callback function v2x_radio_init_complete is mandatory for getting the C-V2X radio initialization status.
2. Application gets return value (0 on success or negative value on error). If the return value is success, the handles of C-V2X IP and non-IP radio interface are provided via the application-supplied pointers ip_handle_p and non_ip_handle_p. The interface handles can be used to specify IP or non-IP traffic type when registering C-V2X radio Tx/Rx flows.
3. Application gets the notification of C-V2X radio initialization status (V2X_STATUS_SUCCESS or V2X_STATUS_FAIL) via the application-supplied v2x_radio_init_complete callback function if the return value of v2x_radio_init_v3 is successful.

Get C-V2X Status
~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_get_status_call_flow.png

   **Get C-V2X Status Call Flow - C++ Version**


This call flow diagram describes the sequence of steps for getting C-V2X radio status using C++ version APIs.
Application must perform C-V2X radio manager initialization before calling any methods of ICv2xRadioManager.

1. Application registers a listener for getting notifications of C-V2X status update.
2. Status of register listener (either SUCCESS or FAILED) will be returned to the application.
3. Application requests the initial C-V2X status using requestCv2xStatus method.
4. Application receives synchronous status (either SUCCESS or FAILED) which indicates if the request was sent successfully.
5. Application is asynchronously notified of the status of the request (either SUCCESS or FAILED) via the application-supplied callback. If success, current C-V2X status is supplied via callback.
6. Application gets notification of C-V2X status update via method onStatusChanged of ICv2xRadioListener.
7. Application deregister the listener.
8. Status of deregistering listener (either SUCCESS or FAILED) will be returned to the application.

.. figure:: /../images/cv2x_get_status_call_flow_c.png

   **Get C-V2X Status Call Flow - C**

This call flow diagram describes the sequence of steps for getting C-V2X radio status using C version APIs.
Application must perform C-V2X radio initialization before calling any methods of C-V2X radio.

1. Application registers a listener for getting notifications of C-V2X status update.
2. Application gets the return value (V2X_STATUS_SUCCESS
   or V2X_STATUS_FAIL) which indicates if the operation was successfully performed. If it succeeded, the C-V2X overall status and per pool status are provided via the application-supplied pointer.
3. Application requests the initial C-V2X status using v2x_get_ext_radio_status method.
4. Application gets the return value (V2X_STATUS_SUCCESS
   or V2X_STATUS_FAIL) which indicates if the operation was successfully performed.
5. Application gets notification of C-V2X status update.
6. Application deregister the listener via setting a NULL listener.
7. Application gets the return value (V2X_STATUS_SUCCESS
   or V2X_STATUS_FAIL) which indicates if the operation was successfully performed.

Get C-V2X Capabilities
~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_get_capabilities_call_flow.png

   **Get C-V2X Capabilities Call Flow - C++ Version**

This call flow diagram describes the sequence of steps for getting C-V2X radio capabilities using C++ version APIs.
Application must perform C-V2X radio initialization before calling any methods of ICv2xRadio.

1. Application registers a listener for getting notifications of C-V2X capabilities update.
2. Status of register listener (either SUCCESS or FAILED) will be returned to the application.
3. Application requests the initial C-V2X capabilities using requestCapabilities method.
4. Application receives synchronous status (either SUCCESS or FAILED) which indicates if the request was sent successfully.
5. Application is asynchronously notified of the status of the request (either SUCCESS or FAILED) via the application-supplied callback. If success, current C-V2X capabilities is supplied via callback.
6. Application gets notification of C-V2X capabilities update via method onCapabilitiesChanged of ICv2xRadioListener.
7. Application deregister the listener.
8. Status of deregistering listener (either SUCCESS or FAILED) will be returned to the application.

.. figure:: /../images/cv2x_get_capabilities_call_flow_c.png

   **Get C-V2X Capabilities Call Flow - C**

This call flow diagram describes the sequence of steps for getting C-V2X radio capabilities using C version APIs. Application must perform C-V2X radio initialization and provide C-V2X capabilities callback to get C-V2X radio capabilities update notification.

1. Application requests the initial C-V2X capabilities using v2x_radio_query_parameters method.
2. Application gets the return value (V2X_STATUS_SUCCESS or V2X_STATUS_FAIL) which indicates if the operation was
   successfully performed. If it succeeded, C-V2X capabilities are provided via the application-supplied pointer.
3. Application gets notification of C-V2X capabilities update via application-supplied callback v2x_radio_capabilities_listener.


C-V2X Radio RX Subscription
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_radio_rx_sub_call_flow.png

   **C-V2X Radio RX Subscription Call Flow - C++ Version**

This call flow diagram describes the sequence of steps for registering or deregistering C-V2X Rx flows using C++ version APIs.
Application must perform C-V2X radio initialization before calling any methods of ICv2xRadio.

There are three Rx modes supported for non-IP traffic,
- Wildcard Rx: Receive all service IDs on a single port. This mode could break catchall Rx and specific service ID Rx.
- Catchall Rx: Receive the specified service IDs on a single port. This mode can be used for Rx-only devices for which no Tx flow need to be created.
- Specific service ID Rx: Receive a specified service ID on a single port. Application can receive different service IDs on different ports by registering a pair of Tx and Rx flows for each service ID.

.. note::
  For catchall or specific service ID Rx mode, filtering for received broadcast packets is implemented in low layer based on the service ID and its mapped destination L2 address, assuming that each service ID is mapped to a unique destination L2 address for all Tx and Rx devices. If this assumption is not valid, for example, in ETSI standards all broadcast service IDs are using same destination L2 address 0xFFFFFFFF, then application should choose wildcard Rx method and do filtering in application layer.

1. Application requests to enable wildcard Rx mode by specifying no service ID when creating Rx flow.
2. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request for creating Rx flow was sent successfully.
3. C-V2X radio sends asynchronous notification via the callback function on the status of the request for creating Rx flow. If SUCCESS, the RX flow is returned in the callback.
4. Application requests to enable catchall Rx mode by specifying a valid service ID list when creating Rx flow.
5. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request for creating Rx flow was sent successfully.
6. C-V2X radio sends asynchronous notification via the callback function on the status of the request for creating Rx flow. If SUCCESS, the RX flow is returned in the callback.
7. Application requests to create a Tx event flow before creating Rx flow to enable specific service ID Rx mode. The service ID and the port number of Tx and Rx flow should be the same.
8. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request for creating Tx flow was sent successfully.
9. C-V2X radio sends asynchronous notification via the callback function on the status of creating Tx flow. If SUCCESS, the TX event flow is returned in the callback.
10. Application requests to enable specific service ID Rx mode if the corresponding Tx flow has been registered successfully.
11. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request for creating Rx flow was sent successfully.
12. C-V2X radio sends asynchronous notification via the callback function on the status of the request for creating Rx flow. If SUCCESS, the RX flow is returned in the callback.
13. Application requests to closes the Tx event flow registered for specific service ID Rx mode.
14. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request for closing Tx flow was sent successfully.
15. C-V2X radio sends asynchronous notification via the callback function on the status of closing Tx flow.
16. Application requests to close the RX flow.
17. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request for closing Rx flow was sent successfully.
18. C-V2X radio sends asynchronous notification via the callback (if a callback was specified) indicating the status of closing Rx flow.


.. figure:: /../images/cv2x_radio_rx_sub_call_flow_c.png

   **C-V2X Radio RX Subscription Call Flow - C Version**

This call flow diagram describes the sequence of steps for registering or deregistering C-V2X Rx flows using C version APIs.
Application must perform C-V2X radio initialization before calling any methods of C-V2X radio.

1. Application requests to enable wildcard Rx mode by specifying no service ID when creating Rx flow.
2. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the Rx flow creation was successful.
3. Application requests to enable catchall Rx mode by specifying a valid service ID list when creating Rx flow.
4. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the Rx flow creation was successful.
5. Application registers a Tx event flow before creating Rx flow to enable specific service ID Rx mode. The service ID and the port number of Tx and Rx flow should be the same.
6. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the Tx event flow registration was successful.
7. Application requests to enable specific service ID Rx mode if the corresponding Tx flow has been registered successfully.
8. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the Rx flow creation was successful.
9. Application requests to closes the Tx event flow registered for specific service ID Rx mode.
10. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the Tx flow has been closed successfully.
11. Application requests to close the Rx flow.
12. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the Rx flow has been closed successfully.

C-V2X Radio TX Event Flow
~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_radio_event_flow_call_flow.png

   **C-V2X Radio TX Event Flow Call Flow - C++ Version**

This call flow diagram describes the sequence of steps for registering or deregistering C-V2X Tx event flows using C++ version APIs.
Application must perform C-V2X radio initialization before calling any methods of ICv2xRadio.

1. Application requests a new TX event flow from the C-V2X radio using createTxEventFlow method.
2. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
3. C-V2X radio sends asynchronous notification via the callback function on the status of the request. If SUCCESS, the TX event flow is returned in the callback.
4. Application requests to close the TX event flow.
5. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
6. C-V2X radio sends asynchronous notification via the callback (if a callback was specified) indicating the status of the request.

.. figure:: /../images/cv2x_radio_event_flow_call_flow_c.png

   **C-V2X Radio TX Event Flow Call Flow - C Version**

This call flow diagram describes the sequence of steps for registering or deregistering C-V2X Tx event flows using C version APIs.
Application must perform C-V2X radio initialization before calling any methods of C-V2X radio.

1. Application requests a new TX event flow from the C-V2X radio using v2x_radio_tx_event_sock_create_and_bind_v3 method and specifies the IP or non-IP traffic type, C-V2X service ID, port number, event flow information and pointers that that point to the created socket and socket address on success.
2. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the operation was successfully performed. If the return value is 0, application gets the socket value from the application-supplied pointer and then starts sending C-V2X data packets using the socket.
3. Application requests to close the RX subscription by calling method v2x_radio_sock_close and supplying with a pointer that points to the TX socket created.
4. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the operation was successfully performed

C-V2X Radio TX SPS flow
~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_radio_sps_flow_call_flow.png

   **C-V2X Radio SPS Flow Call Flow - C++ Version**

This call flow diagram describes the sequence of steps for registering or deregistering C-V2X Tx SPS flows using C++ version APIs.
Application must perform C-V2X radio initialization before calling any methods of ICv2xRadio. Only 2 Tx SPS flows are allowed in maximum in the system.

1. Application requests a new TX SPS flow from the C-V2X radio using createTxSPSFlow method. The application can also specify an optional Tx event flow.
2. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
3. C-V2X radio sends asynchronous notification via the callback function. The callback will return the Tx SPS flow and its status as well as the optional Tx event flow and its status.
4. Application requests to change the SPS parameters using the changeSpsFlowInfo method.
5. Application received synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
6. C-V2X radio sends asynchronous notification via the callback (if callback was specified) indicating the status of the request.
7. Application requests to close the SPS flow.
8. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
9. C-V2X radio sends asynchronous notification via the callback (if a callback was specified) indicating the status of the request.
10. Application requests to close optional Tx event flow (if one was created).
11. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
12. C-V2X radio sends asynchronous notification via the callback (if a callback was specified) indicating the status of the request.

.. figure:: /../images/cv2x_radio_sps_flow_call_flow_c.png

   **C-V2X Radio SPS Flow Call Flow - C Version**

This call flow diagram describes the sequence of steps for registering or deregistering C-V2X Tx SPS flows using C version APIs.
Application must perform C-V2X radio initialization before calling any methods of C-V2X radio. Only 2 Tx SPS flows are allowed in maximum in the system.

1. Application requests a new TX SPS flow from the C-V2X radio using v2x_radio_tx_sps_sock_create_and_bind_v2 method. The application can also specify an optional Tx event flow.
2. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the sps flow was created successfully.
3. Application requests to change the SPS parameters using the v2x_radio_tx_reservation_change_v2 method.
4. Application received synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the sps flow was updated successfully.
5. Application requests to close the SPS flow.
6. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the sps flow was closed successfully.
7. Application requests to close optional Tx event flow (if one was created).
8. Application receives synchronous status (either 0 on SUCCESS or negative values if FAILED) indicating whether the event flow was closed successfully.


C-V2X Throttle Manager Filer rate adjustment notification flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/tm_filter_rate_adjustment_notification.png

1. Application requests C-V2X factory for a C-V2X Throttle Manager.
2. C-V2X factory return ICv2xThrottleManager object to application.
3. Application register a listener for getting notifications of filter rate update.
4. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
5. Application will get filter rate updates, positive value indicates to the application to filter more messages, and negative value indicates to the application to filter less messages
6. Application deregister the listener.
7. Status of deregistering listener, i.e. either SUCCESS or FAILED will be returned.

C-V2X Throttle Manager set verification load flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/tm_set_verification_load.png

1. Application requests C-V2X factory for a C-V2X Throttle Manager.
2. C-V2X factory return ICv2xThrottleManager object to application.
3. Application set verification load using setVerificationLoad method.
4. Application receives synchronous status which indicates if the request was sent successfully.
5. Application is notified of the status of the request (either SUCCESS or FAILED) via the application-supplied callback.

C-V2X TX Status Report
~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_tx_report_call_flow.png

   **C-V2X TX Status Report Call Flow - C++ Version**

This call flow diagram describes the sequence of steps for getting C-V2X Tx Status report per transport block using C++ version APIs.
Application must perform C-V2X radio initialization before calling any methods of ICv2xRadio.

1. Application requests to register a listener for Tx status report, providing the interested port number(portA) of Tx status reports, the listener for receiving the reports and the callback function that used for notification of the result. C-V2X radio enables Tx status report in low layer if its the first request for registering Tx status report.
2. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
3. C-V2X radio sends asynchronous notification via the callback function indication the status of the request.
4. Tx status report is generated for each transport block transmitted, the portB included in Tx status report indicates which source port the specific transport block is sent from, it can be used to associate the Tx status report with a Tx SPS/Event flow. C-V2X radio filters received Tx status reports based on the port of the listener (portA) and the port included in Tx status report (portB):

   - If portA is 0, no filtering to the Tx status reports, application gets Tx status reports for all transport blocks.
   - If portA is not 0 and it equals to portB, application only gets Tx status reports for transport blocks being sent from the specified port.
   - In other cases, Tx status reports are not sent to application.

5. Application deregister listener for Tx status report. If it is the last listener for Tx status report in the system, C-V2X radio disables Tx status report in low layer.
6. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
7. C-V2X radio sends asynchronous notification via the callback (if callback was specified) indicating the status of the request.

.. figure:: /../images/cv2x_tx_report_call_flow_c.png

   **C-V2X TX Status Report Call Flow - C Version**

This call flow diagram describes the sequence of steps for getting C-V2X Tx Status report per transport block using C version APIs.
Application must perform C-V2X radio initialization before calling any methods of C-V2X radio.

1. Application requests to register a listener for Tx status report, providing the interested port number(portA) of Tx status reports and the listener for receiving the reports. C-V2X radio enables Tx status report in low layer if it is the first request for registering Tx status report.
2. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was performed successfully.
3. Tx status report is generated for each transport block transmitted, the portB included in Tx status report indicates which source port the specific transport block is sent from, it can be used to associate the Tx status report with a Tx SPS/Event flow. C-V2X radio filters received Tx status reports based on the port of the listener (portA) and the port included in Tx status report (portB):

   - If portA is 0, no filtering to the Tx status reports, application gets Tx status reports for all transport blocks.
   - If portA is not 0 and it equals to portB, application only gets Tx status reports for transport blocks being sent from the specified port.
   - In other cases, Tx status reports are not sent to application.

4. Application deregister listener for Tx status report. If it is the last listener for Tx status report in the system, C-V2X radio disables Tx status report in low layer.
5. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was performed successfully.

C-V2X RX Meta Data
~~~~~~~~~~~~~~~~~~

.. figure:: /../images/cv2x_rx_meta_data_call_flow.png

   **C-V2X RX Meta Data Call Flow - C++ Verson**

This call flow diagram describes the sequence of steps for enabling C-V2X Rx meta data per packet for non-IP traffic using C++ version APIs.
Application must perform C-V2X radio initialization before calling any methods of ICv2xRadio.

1. Application requests to enable Rx meta data, providing the traffic type (only Non-IP is supported for Rx meta data), the interested service IDs and the callback function that used for notification of the result.
2. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
3. C-V2X radio sends asynchronous notification via the callback function indication the status of the request.
4. After the enable of Rx meta data, application receives data packet which includes raw data along with SFN and subchannel index information via the Rx socket returned from Rx flow registration.
5. Application calls method getRxMetaDataInfo to get the raw data, SFN and subchannel index information from received data packet.
6. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the data packet was parsed successfully.
7. Application receives Rx meta data packet via the Rx socket returned from Rx flow registration, one Rx meta data packet may include Rx meta data for multiple received data packets.
8. Application calls method getRxMetaDataInfo to get Rx meta data information from received Rx meta data packet. The information of SFN and subchannel index included in Rx meta data can be used to associate the Rx meta data with the received data packet.
9. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the Rx meta data packet was parsed successfully.
10. Application requests to disable Rx meta data, providing the traffic type (only Non-IP is supported for Rx meta data), the registered service IDs and the callback function that used for notification of the result.
11. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was sent successfully.
12. C-V2X radio sends asynchronous notification via the callback function indication the status of the request.

.. figure:: /../images/cv2x_rx_meta_data_call_flow_c.png

   **C-V2X RX Meta Data Call Flow - C Version**

This call flow diagram describes the sequence of steps for enabling C-V2X Rx meta data per packet for non-IP traffic using C version APIs.
Application must perform C-V2X radio initialization before calling any methods of C-V2X radio.

1. Application requests to enable Rx meta data, providing the traffic type (only Non-IP is supported for Rx meta data) and the interested service IDs.
2. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was performed successfully.
3. After the enable of Rx meta data, application receives data packet which includes raw data along with SFN and subchannel index information via the Rx socket returned from Rx flow registration.
4. Application calls method v2x_parse_rx_meta_data to get the raw data, SFN and subchannel index information from received data packet.
5. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the data packet was parsed successfully.
6. Application receives Rx meta data packet via the Rx socket returned from Rx flow registration, one Rx meta data packet may include Rx meta data for multiple received data packets.
7. Application calls method v2x_parse_rx_meta_data to get Rx meta data information from received Rx meta data packet. The information of SFN and subchannel index included in Rx meta data can be used to associate the Rx meta data with the received data packet.
8. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the Rx meta data packet was parsed successfully.
9. Application requests to disable Rx meta data, providing the traffic type (only Non-IP is supported for Rx meta data), the registered service IDs and the callback function that used for notification of the result.
10. Application receives synchronous status (either SUCCESS or FAILED) indicating whether the request was performed successfully.


Audio
-----

Audio Manager API call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_manager_api_call_flow.png

1. Application requests Audio factory for an Audio Manager and passes callback pointer.
2. Audio factory return IAudioManager object to application.
3. Application can use IAudioManager::getServiceStatus to determine the state of sub system.
4. The application receives the ServiceStatus of sub system which indicates the state of service.
5. AudioManager notifies the application when the subsystem is ready through the callback mechanism.
6. On Readiness, Application requests supported device types using getDevices method.
7. Application receives synchronous Status which indicates if the getDevices request was sent successfully.
8. Application is notified of the Status of the getDevices request (either SUCCESS or FAILED) via the application-supplied callback, with array of supported device types.
9. Application requests supported stream types using getStreamTypes method.
10. Application receives synchronous Status which indicates if the getStreamTypes request was sent successfully.
11. Application is notified of the Status of the getStreamTypes request (either SUCCESS or FAILED) via the application-supplied callback, with array of supported stream types.
12. Application requests create audio stream using createStream method.
13. Application receives synchronous Status which indicates if the createStream request was sent successfully.
14. Application is notified of the Status of the createStream request (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface.
15. Application requests delete audio stream using deleteStream method.
16. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
17. Application is notified of the Status of the deleteStream request (either SUCCESS or FAILED) via the application-supplied callback.

Audio Voice Call Start/Stop call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_voicecall_start_stop_call_flow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests create audio voice stream using createStream method with streamType as VOICE_CALL.
4. Application receives synchronous Status which indicates if the createStream request was sent successfully.
5. Application is notified of the Status of the createStream request (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioVoiceStream.
6. Application requests start audio stream using startAudio method on IAudioVoiceStream.
7. Application receives synchronous Status which indicates if the startAudio request was sent successfully.
8. Application is notified of the Status of the startAudio request (either SUCCESS or FAILED) via the application-supplied callback.
9. Application requests stop audio stream using stopAudio method on IAudioVoiceStream.
10. Application receives synchronous Status which indicates if the stopAudio request was sent successfully.
11. Application is notified of the Status of the stopAudio request (either SUCCESS or FAILED) via the application-supplied callback.
12. Application requests delete audio stream using deleteStream method.
13. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
14. Application is notified of the Status of the deleteStream request (either SUCCESS or FAILED) via the application-supplied callback.

Audio Voice Call Device Switch call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_voicecall_device_switch_call_flow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests create audio voice stream using createStream method with streamType as VOICE_CALL.
4. Application receives synchronous Status which indicates if the createStream request was sent successfully.
5. Application is notified of the Status of the createStream request (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioVoiceStream.
6. Application requests start audio stream using startAudio method on IAudioVoiceStream.
7. Application receives synchronous Status which indicates if the startAudio request was sent successfully.
8. Application is notified of the Status of the startAudio request (either SUCCESS or FAILED) via the application-supplied callback.
9. Application requests new device routing of stream using setDevice method on IAudioVoiceStream.
10. Application receives synchronous Status which indicates if the setDevice request was sent successfully.
11. Application is notified of the Status of the setDevice request (either SUCCESS or FAILED) via the application-supplied callback.
12. Application query device stream routed to using getDevice method on IAudioVoiceStream.
13. Application receives synchronous Status which indicates if the getDevice request was sent successfully.
14. Application is notified of the Status of the getDevice request (either SUCCESS or FAILED) via the application-supplied callback, along with device types.
15. Application requests stop audio stream using stopAudio method on IAudioVoiceStream.
16. Application receives synchronous Status which indicates if the stopAudio request was sent successfully.
17. Application is notified of the Status of the stopAudio request (either SUCCESS or FAILED) via the application-supplied callback.
18. Application requests delete audio stream using deleteStream method.
19. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
20. Application is notified of the Status of the deleteStream request (either SUCCESS or FAILED) via the application-supplied callback.

Audio Voice Call Volume/Mute control call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_voicecall_volume_mute_control_call_flow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests create audio voice stream using createStream method with streamType as VOICE_CALL.
4. Application receives synchronous Status which indicates if the createStream request was sent successfully.
5. Application is notified of the Status of the createStream request (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioVoiceStream.
6. Application requests start audio stream using startAudio method on IAudioVoiceStream.
7. Application receives synchronous Status which indicates if the startAudio request was sent successfully.
8. Application is notified of the Status of the startAudio request (either SUCCESS or FAILED) via the application-supplied callback.
9. Application requests new volume on stream using setVolume method on IAudioVoiceStream for specified direction.
10. Application receives synchronous Status which indicates if the setVolume request was sent successfully.
11. Application is notified of the Status of the setVolume request (either SUCCESS or FAILED) via the application-supplied callback.
12. Application query volume on stream using getVolume method on IAudioVoiceStream for specified direction.
13. Application receives synchronous Status which indicates if the getVolume request was sent successfully.
14. Application is notified of the Status of the getVolume request (either SUCCESS or FAILED) via the application-supplied callback for specified direction with volume details.
15. Application requests new mute on stream using setMute method on IAudioVoiceStream for specified direction.
16. Application receives synchronous Status which indicates if the setMute request was sent successfully.
17. Application is notified of the Status of the setMute request (either SUCCESS or FAILED) via the application-supplied callback.
18. Application query mute details on stream using getMute method on IAudioVoiceStream for specified direction.
19. Application receives synchronous Status which indicates if the getMute request was sent successfully.
20. Application is notified of the Status of the getMute request (either SUCCESS or FAILED) via the application-supplied callback for specified direction with mute details.
21. Application requests stop audio stream using stopAudio method on IAudioVoiceStream.
22. Application receives synchronous Status which indicates if the stopAudio request was sent successfully.
23. Application is notified of the Status of the stopAudio request (either SUCCESS or FAILED) via the application-supplied callback.
24. Application requests delete audio stream using deleteStream method.
25. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
26. Application is notified of the Status of the deleteStream request (either SUCCESS or FAILED) via the application-supplied callback.

Call flow to play DTMF tone
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_voicecall_dtmf_play_call_flow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to create a voice stream with streamType as VOICE_CALL.
4. Application receives synchronous status which indicates if the createStream request was sent successfully.
5. Application is notified of the createStream request status (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioVoiceStream.
6. Application requests to start voice session using startAudio method on IAudioVoiceStream.
7. Application receives synchronous status which indicates if the startAudio request was sent successfully.
8. Application is notified of the startAudio request status (either SUCCESS or FAILED) via the application-supplied callback.
9. Application requests to play a DTMF tone associated with the voice session
10. Application receives synchronous status which indicates if the playDtmfTone request was sent successfully.
11. Application is notified of the playDtmfTone request status (either SUCCESS or FAILED) via the application-supplied callback.
12. Application can optionally stop the DTMF tone being played, before its duration expires.
13. Application receives synchronous status which indicates if the stopDtmfTone request was sent successfully.
14. Application is notified of the stopDtmfTone request status (either SUCCESS or FAILED) via the application-supplied callback.
15. Application requests to stop the voice session using stopAudio method on IAudioVoiceStream.
16. Application receives synchronous Status which indicates if the stopAudio request was sent successfully.
17. Application is notified of the stopAudio request status(either SUCCESS or FAILED) via the application-supplied callback.
18. Application requests delete audio stream using deleteStream method.
19. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
20. Application is notified of the deleteStream request status(either SUCCESS or FAILED) via the application-supplied callback.

Call flow to detect DTMF tones
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_voicecall_dtmf_detect_call_flow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to create a voice stream with streamType as VOICE_CALL.
4. Application receives synchronous status which indicates if the createStream request was sent successfully.
5. Application is notified of the createStream request status (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioVoiceStream.
6. Application requests to start voice session using startAudio method on IAudioVoiceStream.
7. Application receives synchronous status which indicates if the startAudio request was sent successfully.
8. Application is notified of the startAudio request status (either SUCCESS or FAILED) via the application-supplied callback.
9. Application registers a listener for getting notifications when DTMF tones are detected
10. Application receives synchronous status which indicates if the registerListener request was sent successfully.
11. Application is notified of the registerListener request status (either SUCCESS or FAILED) via the application-supplied callback.
12. Application receives onDtmfToneDetection notification when a DTMF tone is detected in the active voice call session
13. Application deregisters a listener to stop getting notifications
14. Application receives synchronous status which indicates if the deRegisterListener request was sent successfully.
15. Application requests to stop the voice session using stopAudio method on IAudioVoiceStream.
16. Application receives synchronous Status which indicates if the stopAudio request was sent successfully.
17. Application is notified of the stopAudio request status(either SUCCESS or FAILED) via the application-supplied callback.
18. Application requests delete audio stream using deleteStream method.
19. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
20. Application is notified of the deleteStream request status(either SUCCESS or FAILED) via the application-supplied callback.

Audio Playback call flow
~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_playback_call_flow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests create audio playback stream using createStream method with streamType as PLAY.
4. Application receives synchronous Status which indicates if the createStream request was sent successfully.
5. Application is notified of the Status of the createStream request (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioPlayStream.
6. Application requests stream buffer#1 using getStreamBuffer method on IAudioPlayStream.
7. Application receives IStreamBuffer if Success.
8. Application requests stream buffer#2 using getStreamBuffer method on IAudioPlayStream.
9. Application receives IStreamBuffer if Success.
10. Application writes audio samples on buffer#1 using getRawBuffer method on IStreamBuffer.
11. Application writes buffer#1 on Playback session using write method on IAudioPlayStream.
12. Application receives synchronous Status which indicates if the write request was sent successfully.
13. Application writes audio samples on buffer#2 using getRawBuffer method on IStreamBuffer.
14. Application writes buffer#2 on Playback session using write method on IAudioPlayStream.
15. Application receives synchronous Status which indicates if the write request was sent successfully.
16. Application is notified of the buffer#1 write Status (either SUCCESS or FAILED) via the application-supplied write callback with successful bytes written.
17. Application is notified of the buffer#2 write Status (either SUCCESS or FAILED) via the application-supplied write callback with successful bytes written.
18. Application requests delete audio stream using deleteStream method.
19. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
20. Application is notified of the Status of the deleteStream request (either SUCCESS or FAILED) via the application-supplied callback.

Audio Capture call flow
~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_capture_call_flow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests create audio capture stream using createStream method with streamType as CAPTURE.
4. Application receives synchronous Status which indicates if the createStream request was sent successfully.
5. Application is notified of the Status of the createStream request (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioCaptureStream.
6. Application requests stream buffer#1 using getStreamBuffer method on IAudioCaptureStream.
7. Application receives IStreamBuffer if Success.
8. Application requests stream buffer#2 using getStreamBuffer method on IAudioCaptureStream.
9. Application receives IStreamBuffer if Success.
10. Application decides read sample size.
11. Application issue read audio samples on buffer#1 using read method on IAudioCaptureStream.
12. Application receives synchronous Status which indicates if the read request was sent successfully.
13. Application issue read audio samples on buffer#2 using read method on IAudioCaptureStream.
14. Application receives synchronous Status which indicates if the read request was sent successfully.
15. Application is notified of the buffer#1 write Status (either SUCCESS or FAILED) via the application-supplied read callback with successful bytes read.
16. Application is notified of the buffer#2 write Status (either SUCCESS or FAILED) via the application-supplied read callback with successful bytes read.
17. Application requests delete audio stream using deleteStream method.
18. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
19.  Application is notified of the Status of the deleteStream request (either SUCCESS or FAILED) via the application-supplied callback.

Audio Tone Generator call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_tone_generation_callflow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to create a tone generator stream with streamType as TONE_GENERATOR.
4. Application receives synchronous status which indicates if the createStream request was sent successfully.
5. Application is notified of the createStream request status (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioToneGeneratorStream.
6. Application requests to play tone using playTone method on IAudioToneGeneratorStream.
7. Application receives synchronous status which indicates if the playTone request was sent successfully.
8. Application is notified of the playTone request status (either SUCCESS or FAILED) via the application-supplied callback.
9. Application can optionally stop the tone being played, before its duration expires.
10. Application receives synchronous status which indicates if the stopTone request was sent successfully.
11. Application is notified of the stopTone request status (either SUCCESS or FAILED) via the application-supplied callback.
12. Application requests delete audio stream using deleteStream method.
13. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
14. Application is notified of the deleteStream request status(either SUCCESS or FAILED) via the application-supplied callback.

Audio Loopback call flow
~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_loopback_callflow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to create a loopback stream with streamType as LOOPBACK.
4. Application receives synchronous status which indicates if the createStream request was sent successfully.
5. Application is notified of the createStream request status (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioLoopbackStream.
6. Application requests to start loopback using startLoopback method on IAudioLoopbackStream.
7. Application receives synchronous status which indicates if the startLoopback request was sent successfully.
8. Application is notified of the startLoopback request status (either SUCCESS or FAILED) via the application-supplied callback.
9. Application requests to stop loopback using stopLoopback method on IAudioLoopbackStream.
10. Application receives synchronous status which indicates if the stopLoopback request was sent successfully.
11. Application is notified of the stopLoopback request status (either SUCCESS or FAILED) via the application-supplied callback.
12. Application requests delete audio stream using deleteStream method.
13. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
14. Application is notified of the deleteStream request status(either SUCCESS or FAILED) via the application-supplied callback.

Compressed audio format playback call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/compressed_audio_format_playback_callflow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to create a play stream with streamType as PLAY.
4. Application receives synchronous status which indicates if the createStream request was sent successfully.
5. Application is notified of the createStream request status (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioPlayStream.
6. Application requests to write buffer using write method on IAudioPlayStream.
7. Application receives synchronous status which indicates if the write request was sent successfully.
8. Application is notified of the write request status (either SUCCESS or FAILED) via the application-supplied callback along with number of bytes written.
9. Application is notified of when pipeline is ready to accept new buffer if callback returns with error that number of bytes written are not equal to bytes requested.
10. Application send request to stop playback using stopAudio method of IAudioPlayStream.
11. Application receives synchronous status which indicates if the stopAudio request was sent successfully.
12. Application is notified of the stopAudio request status (either SUCCESS or FAILED) via the application-supplied callback.
13. Appication is notified via indication that playback is stopped if StopType is STOP_AFTER_PLAY.
14. Application requests delete audio stream using deleteStream method.
15. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
16. Application is notified of the deleteStream request status(either SUCCESS or FAILED) via the application-supplied callback.

Audio Transcoding Operation Callflow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_transcoding_callflow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to create a transcoder.
4. Application receives synchronous status which indicates if the createTranscoder request was sent successfully.
5. Application is notified of the createTranscoder request status (either SUCCESS or FAILED) via the application-supplied callback, with pointer to transcoder interface refering to ITranscoder.
6. Application requests to read buffer using read method on ITranscoder.
7. Application receives synchronous status which indicates if the read request was sent successfully.
8. Application is notified of the read request status (either SUCCESS or FAILED) via the application-supplied callback along with isLastBuffer flag which indicates whether the buffer is last buffer to read or not.
9. Application requests to write buffer using write method on ITranscoder.
10. Application receives synchronous status which indicates if the write request was sent successfully.Application need to mark the isLastBuffer flag, whenever it is providing the last buffer to be write.
11. Application is notified of the write request status (either SUCCESS or FAILED) via the application-supplied callback along with number of bytes written.
12. Application is notified of when pipeline is ready to accept new buffer if callback returns with error that number of bytes written are not equal to bytes requested.
13. Once transcoding done, Application requests to tearDown transcoder as transcoder can not be used for multiple transcoding operations.
14. Application receives synchronous status which indicates if the tearDown request was sent successfully.
15. Application is notified of the tearDown request status (either SUCCESS or FAILED) via the application-supplied callback.

Compressed audio format playback on Voice Paths Callflow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/compressed_audio_format_playback_on_voice_paths_callflow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to create a play stream with streamType as PLAY, voicePaths direction as TX or RX and no device is selected.
4. Application receives synchronous status which indicates if the createStream request was sent successfully.
5. Application is notified of the createStream request status (either SUCCESS or FAILED) via the application-supplied callback, with pointer to stream interface refering to IAudioPlayStream.
6. Application requests to write buffer using write method on IAudioPlayStream. It needs an active voice session to play over voice paths, refer IAudioVoiceStream for more details on how to create voice stream.
7. Application receives synchronous status which indicates if the write request was sent successfully.
8. Application is notified of the write request status (either SUCCESS or FAILED) via the application-supplied callback along with number of bytes written.
9. Application is notified of when pipeline is ready to accept new buffer if callback returns with error that number of bytes written are not equal to bytes requested.
10. Application send request to stop playback using stopAudio method of IAudioPlayStream.
11. Application receives synchronous status which indicates if the stopAudio request was sent successfully.
12. Application is notified of the stopAudio request status (either SUCCESS or FAILED) via the application-supplied callback.
13. Appication is notified via indication that playback is stopped if StopType is STOP_AFTER_PLAY.
14. Application requests delete audio play stream using deleteStream method.
15. Application receives synchronous Status which indicates if the deleteStream request was sent successfully.
16. Application is notified of the deleteStream request status(either SUCCESS or FAILED) via the application-supplied callback.

Audio Subsystem Restart Callflow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_ssr_callflow.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On Readiness, Application requests to register listener to IAudioManager object.
4. Application receives synchronous status which indicates registerListener request was sent successfully.
5. IAudioManager is notified that audio service is unavailable.
6. Application receives a notification from the IAudioManager regarding service status as unavailable.
   Application is supposed to relealse references to all the IAudioStream/ITranscoder objects and related resources.
   Application should not send any new request to IAudioManager or IAudioStream/ITranscoder objects.
7. IAudioManager is notified that audio service is available.
8. Application receives a notification from the IAudioManager regarding service status as available.
   IAudioManager object is now ready to accept new requests from application.

Audio calibration configuration status
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_get_cal_config.png

1. Application requests Audio factory for an Audio Manager.
2. Audio factory return IAudioManager object to application.
3. On sub system Readiness, application requests for calibration init status using getCalibrationInitStatus API to IAudioManager.
4. Application receives synchronous status which indicates if the getCalibrationInitStatus request was sent successfully.
5. Application is notified with CalibrationInitStatus and suitable error code via the application supplied callback.

Playing a file a specific number of times
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_endless_count_callflow.png

1. Application request an AudioFactory instance from AudioFactory.
2. AudioFactory instance is returned to the application.
3. From AudioFactory, application request an IAudioPlayer instance.
4. IAudioPlayer instance is returned to the application.
5. Application defines audio stream configuration, files to play and number of times it should be played. It then calls startPlayback() on the IAudioPlayer instance.
6. Success/failure errorcode is returned to the application to indicate if startPlayback() could start playback or not.
7. When the IAudioPlayer starts playback, onPlaybackStarted() callback is called on an instance of the class implementing IPlayListListener.
8. The onFilePlayed() is invoked for the 1st file. Every time a file is played, onFilePlayed() callback is called on an instance of the class implementing IPlayListListener to indicate that the file has been played. This is called every time a file is played irrespective of the number of times it will be played.
9. The onFilePlayed() is invoked for the 2nd file.
10. When all files have been played completely, onPlaybackFinished() callback is invoked. This marks completion of the repeated playback use case.

    i. If an error occurs for example audio stream can not be created or audio sample can not be played, onError() callback is called.
    ii.. Following the onError() callback invocation, onPlaybackStopped() is called to inform application that the playback is terminated.

Playing a file indefinitely
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/audio_endless_infinite_callflow.png

1. Application request an AudioFactory instance from AudioFactory.
2. AudioFactory instance is returned to the application.
3. From AudioFactory, application request an IAudioPlayer instance.
4. IAudioPlayer instance is returned to the application.
5. Application defines audio stream configuration, files to play and infinite repeat type. It then calls startPlayback() on the IAudioPlayer instance.
6. Success/failure errorcode is returned to the application to indicate if startPlayback() could start playback or not.
7. When the IAudioPlayer starts playback, onPlaybackStarted() callback is called on an instance of the class implementing IPlayListListener.
8. The onFilePlayed() is invoked to indicate file has been played 1st time. Every time a file is played, onFilePlayed() callback is called on an instance of the class implementing IPlayListListener to indicate that the file has been played. This is called every time the file is played.
9. The onFilePlayed() is invoked to indicate file has been played 2nd time.
10. The onFilePlayed() is invoked to indicate file has been played 3rd time.
11. When the use case is complete, application calls stopPlayback() to explicitly stop the playback.
12. Success/failure errorcode is returned to the application to indicate if stopPlayback() could stop playback or not.
13. When the playback is stopped, onPlaybackStopped() callback is called. This marks completion of the repeated playback use case.

    i. If an error occurs for example audio stream can not be created or audio sample can not be played, onError() callback is called.
    ii.. Following the onError() callback invocation, onPlaybackStopped() is called to inform application that the playback is terminated.

Thermal
-------

Thermal Manager API call flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Thermal manager provides APIs to get list of thermal zones and cooling devices. It also contains APIs
to get a particular thermal zone and a particular cooling device details with the given Id.

.. figure:: /../images/thermal_manager_api_call_flow.png

1. Application requests for an instance of thermal factory.
2. Thermal factory instance is received by the application.
3. Application prepares the callback which would be called on initialization of thermal manager.
4. Application request for thermal manager from thermal factory.
5. Thermal factory returns an instance of the thermal manager object.
6. Application waits for initalization callback to be called.
7. Thermal factory invokes the callback once initialization completes.
8. Application sends request to get all thermal zones using IThermalManager object.
9. Thermal manager returns the list of thermal zones to the application.
10. Application requests for a particular thermal zone details by mentioning the thermal zone Id.
11. Application receives thermal zone details with the given Id from thermal manager.
12. Application sends request to get all cooling devices using IThermalManager object.
13. Thermal manager returns the list of cooling devices to the application.
14. Application requests for a particular cooling device details by passing the cooling device Id.
15. Thermal Manager sends cooling device details with the given Id to the application.

Call flow to register/remove listener for Thermal manager notifications
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Thermal listeners provide a set of listeners on which the app is notified when certain
events occur. It also allows an application to register some or all types of notifications.

.. figure:: /../images/thermal_manager_notifications_callflow.png

The thermal sub-system should have been initialized successfully with SERVICE_AVAILABLE as a
pre-requisite for any thermal notifications and a valid ThermalManager object is available.

1. Application should create a thermal listening object.
2. Application can register a listener for specific or all type of notifications by providing
   a different mask value. The default value of mask is 0xFFFF which indicates for all type of
   notifications. The SSR notification is registered by default, whether it provides mask values
   or not.
3. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
4. Application is notified when service status changes.
5. Application registered for this notification will be notified when a trip event occurs for
   any thermal zone.
6. Application registered for this notification will be notified when a cooling device changes
   its states.
7. Application can de-register a listener for specific or all type of notifications by providing
   a different mask value. The default value of mask is 0xFFFF which indicates for all type of
   notifications including SSR. The SSR notification will not be deregistered if the request
   provided a mask value.
8. Status of de-register listener i.e. either SUCCESS or FAILED will be returned to the application.

Thermal shutdown management
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Thermal shutdown manager provides APIs to set/get auto thermal shutdown modes. It also has
listener interface for notifications. Application will get the Thermal-shutdown manager object
from thermal factory. The application can register a listener for updates in thermal auto
shutdown modes and its management service status. Also there is provision to set the desired
thermal auto-shutdown mode.

When application is notifed of service being unavailable, the thermal auto-shutdown mode updates
are inactive. After service becomes available, the existing listener registrations will be
maintained.

As a reference, auto-shutdown management in an eCall application is described in the below
sections.

Call flow to register/remove listener for Thermal auto-shutdown mode updates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/thermal_shutdown_mode_notification_callflow.png

1. Application requests for an instance of thermal factory.
2. Thermal factory instance is received by the application.
3. Application prepares the callback which would be called on initialization of thermal shutdown manager.
4. Application request for thermal shutdown manager from thermal factory.
5. Thermal factory returns an instance of the thermal shutdown manager object.
6. Application waits for initalization callback to be called.
7. Thermal factory invokes the callback once initialization completes.
8. Application can register a listener for getting notifications on Thermal auto-shutdown mode updates.
9. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
10. Application receives a notification that thermal auto-shutdown mode is disabled.
11. Application receives a notification that thermal auto-shutdown mode is going to enabled soon. The exact duration is also recieved as part of notification.
12. Application receives a notification that thermal auto-shutdown mode is enabled.
13. Application can remove listener.
14. Status of remove listener i.e. either SUCCESS or FAILED will be returned to the application.

Call flow to set/get the Thermal auto-shutdown mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/thermal_shutdown_mode_command_callflow.png

1. Application requests for an instance of thermal factory.
2. Thermal factory instance is received by the application.
3. Application prepares the callback which would be called on initialization of thermal shutdown manager.
4. Application request for thermal shutdown manager from thermal factory.
5. Thermal factory returns an instance of the thermal shutdown manager object.
6. Application waits for initalization callback to be called.
7. Thermal factory invokes the callback once initialization completes.
8. Application can query the thermal auto-shutdown mode.
9. Application receives synchronous status which indicates if the request was sent successfully.
10. Application receives the auto-shutdown mode asynchronously.
11. Application can set the thermal auto-shutdown mode to ENABLE or DISABLE.
12. Application receives synchronous status which indicates if the request was sent successfully.
13. Optionally, the response to setAutoShutdownMode request can be received by the application.

Call flow to manage thermal auto-shutdown from an eCall application
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/thermal_shutdown_mode_eCall_app_callflow.png

1. Application requests for an instance of thermal factory.
2. Thermal factory instance is received by the application.
3. Application prepares the callback which would be called on initialization of thermal shutdown manager.
4. Application request for thermal shutdown manager from thermal factory.
5. Thermal factory returns an instance of the thermal shutdown manager object.
6. Application waits for initalization callback to be called.
7. Thermal factory invokes the callback once initialization completes.
8. Application can register a listener for getting notifications on Thermal auto-shutdown mode updates.
9. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
10. Application disables auto-shutdown using setAutoShutdownMode API, to prevent a possible thermal auto-shutdown during eCall.
11. Application receives synchronous status which indicates if the request was sent successfully.
12. Optionally, the response to setAutoShutdownMode request can be received by the application.
13. Application receives a notification that thermal auto-shutdown mode is disabled.
14. Application receives an imminent auto-shutdown enable notification and system will attempt to enable auto-shutdown after a certain period.
    This notification is received if application does not enable auto-shutdown due to an active eCall.
15. If the eCall is still active, the application disables auto-shutdown before it gets enabled automatically.
16. Application receives synchronous status which indicates if the request was sent successfully.
17. Optionally, the response to setAutoShutdownMode request can be received by the application.
18. Application receives a notification that thermal auto-shutdown mode is disabled.
    Steps 14 to 18 are repeated as long as the eCall is active.
19. When the eCall is completed, the application immediately enables auto-shutdown using setAutoShutdownMode API.
20. Application receives synchronous status which indicates if the request was sent successfully.
21. Optionally, the response to setAutoShutdownMode request can be received by the application.
22. Application receives a notification that thermal auto-shutdown mode is enabled.
23. Application can remove listener.
24. Status of remove listener i.e. either SUCCESS or FAILED will be returned to the application.


TCU Activity Management
-----------------------

An application can get the appropriate TCU-activity manager (i.e. slave or master) object from the power factory.
The TCU-activity manager configured as the master is responsible for triggering state transitions.
TCU-activity manager configured as a slave is responsible for listening to state change indications and acknowledging when it performs necessary tasks and prepares for the state transition.
A machine in this power management framework represents an application processor subsystem or a host/guest virtual machine on hypervisor based platforms.

- Only one master is allowed in the system, and currently we only support allowing the master on the primary/host machine and not on the guest virtual machine.
- It is expected that all processes interested in a TCU-activity state change should register as slaves.
- When the master changes the TCU-activate state, slaves connected to the impacted machine are notified.
- Master can trigger the TCU-activity state change of a specific machine or all machines at once.
- If the slave wants to differentiate between a state change indication that is the result of a trigger for all machines or a trigger for its specific machines, it can be detected using the machine name provided in the listener API.
- When the master triggers an all machines TCU-activity state change, only the machines that are not in the desired state will undergo the state transition, and the slaves to those machines will be notified.
- In the case of 

  - suspend or shutdown trigger: 

    - After becoming ready for state change, all slave clients should acknowledge back.
      - The master client will get notification about the consolidated acknowledgement status of all slave clients.
      - On getting a successful consolidated acknowledgement from all the slaves for the suspend trigger, the power framework allows the respective machine to suspend. On getting a successful consolidated acknowledgement from all the slaves for the shutdown trigger, the power framework triggers the respective machine shutdown without waiting further.
      - If the slave client sends a NACK to indicate that it is not ready for state transition or fails to acknowledge before the configured time, then the master will get to know via a consolidated acknowledgement / slave acknowledgement status notification.
      - In such failed cases, if the master wants to stop the state transition considering the information in the consolidated acknowledgement, then the master is allowed to trigger a new TCU-activity state change, or else the state transition will proceed after the configured timeout.

   - resume trigger:

      - Power framework will prevent the respective machine from going into suspend.
      - No acknowledgement will be required from slave clients and the master will not be getting consolidated acknowledgement / slave acknowledgement as machine will be already resumed. 

 When the application is notified about the service being unavailable, the TCU-activity state notifications will be inactive. After the service becomes available, the existing listener registrations will be maintained.


Call flow to register/remove listener for TCU-activity manager
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/tcuactivity_manager_listener_callflow.png

1. Application requests power factory for TCU-activity manager object, with clientType as slave. slave clients set up to listen to their LOCAL_MACHINE. Also, it is recommended to give a unique name to each slave client so that client can be identified later in case of failure.
2. Power factory returns ITcuActivityManager object using which application will register or remove a listener.
3. Wait for the TCU-activity management services to be ready.
4. Application can register a listener for getting notifications on TCU-activity state updates on the machine. Some of the listener APIs are designed for slave clients and others for master clients. Listener APIs intended for the master client will not be sent to the slave client, and listner APIs intended for the slave client will not be sent for the master client.
5. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application.
6. Application will get TCU-activity state notifications like suspend, resume and shutdown.
7. Application will send one acknowledgement, after processing(save any required information) suspend/shutdown notifications. This indicates the readiness of application for state-transition. However the TCU-activity management service doesn't wait for acknowledgement indefinitely, before performing the state transition. In the acknowledgement, slave can specify the machine name received in the notification in the previous step.
8. Application receives synchronous status which indicates if the acknowledgement was sent successfully.
9. Application can remove listener.
10. Status of remove listener i.e. either SUCCESS or FAILED will be returned to the application.

Call flow to set the TCU-activity state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/tcuactivity_manager_commands_callflow.png

1. Application requests power factory for TCU-activity manager object, with clientType as master.
2. Power factory returns ITcuActivityManager object using which application will set the TCU-activity state.
3. Wait for the TCU-activity management services to be ready.
4. Application can register a listener for getting notifications on TCU-activity state.
5. Status of register listener i.e. either SUCCESS or FAILED will be returned to the application
6. Get a list of available machine names in the power framework if the user is interested in the state transition of a specific machine.
7. Application can set the TCU-activity state to suspend, resume or shutdown.
8. Application receives synchronous status which indicates if the request was sent successfully.
9. Optionally, the response to setActivityState request can be received by the application.
10. The application waits for consolidated acknowledgement status and analyzes the response. If status is not SUCCESS and the master expects to stop state transition considering the provided information, then call setActivityState and revert state, or else state transition will proceed after the configured timeout in /etc/power_state.conf.
11. Application can remove listener.
12. Status of remove listener i.e. either SUCCESS or FAILED will be returned to the application.

Modem Config
------------

Modem Config manager provides APIs to request all configs from modem, load/delete modem config files
from modem's storage, activate/deactivate a modem config file, get the active config details, set
and get auto config selection mode. It also has listener interface for notifications for config
activation update status. Application will get the Modem Config manager object from config factory.
The application can register a listener for updates reagrding modem config activation.

Call flow to load and activate a modem config file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/modem_config_load_and_activate_call_flow.png

1. Application requests Config factory for ModemConfig Manager and passes callback pointer.
2. Config factory return IModemConfigManager object to application.
3. Application can use IModemConfigManager::getServiceStatus to determine the state of sub system.
4. The application receives the ServiceStatus of sub system which indicates the state of service.
5. IModemConfigManager notifies the application when the subsystem is ready through the callback mechanism.
6. Application sends a request to load config file in modem's storage.
7. Application receives synchronous Status which indicates if the request to load config file was sent successfully.
8. Application is notified of the Status of the loadConfigFile request (either SUCCESS or FAILED) via the application-supplied callback.
9. Application sends a request to get list of all modem configs from modem's storage.
10. Application receives synchronous Status which indicates if the request to get config list was sent successfully.
11. Application is notified of the Status of the requestConfigList request (either SUCCESS or FAILED) via the application-supplied callback along with list of modem configs.
12. Application sends a request to activate config file.
13. Application receives synchronous Status which indicates if the request to activate config file was sent successfully.
14. Application is notified of the Status of the activateConfig request (either SUCCESS or FAILED) via the application-supplied callback.

Call flow to deactivate and delete a modem config file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/deactivate_and_delete_modem_config_call_flow.png

1. Application requests Config factory for ModemConfig Manager and passes callback pointer.
2. Config factory return IModemConfigManager object to application.
3. Application can use IModemConfigManager::getServiceStatus to determine the state of sub system.
4. The application receives the ServiceStatus of sub system which indicates the state of service.
5. IModemConfigManager notifies the application when the subsystem is ready through the callback mechanism.
6. Application sends a request to deactivate config file.
7. Application receives synchronous Status which indicates if the request to deactivate config file was sent successfully.
8. Application is notified of the Status of the deactivateConfig request (either SUCCESS or FAILED) via the application-supplied callback.
9. Application sends a request to get list of all modem configs from modem's storage.
10. Application receives synchronous Status which indicates if the request to get config list was sent successfully.
11. Application is notified of the Status of the requestConfigList request (either SUCCESS or FAILED) via the application-supplied callback along with list of modem configs.
12. Application sends a request to delete config file.
13. Application receives synchronous Status which indicates if the request to delete config file was sent successfully.
14. Application is notified of the Status of the deleteConfig request (either SUCCESS or FAILED) via the application-supplied callback.

Call flow to set and get config auto selection mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/modem_config_set_and_get_auto_selection_mode_call_flow.png

1. Application requests Config factory for ModemConfig Manager and passes callback pointer.
2. Config factory return IModemConfigManager object to application.
3. Application can use IModemConfigManager::getServiceStatus to determine the state of sub system.
4. The application receives the ServiceStatus of sub system which indicates the state of service.
5. IModemConfigManager notifies the application when the subsystem is ready through the callback mechanism.
6. Application sends a request to set config auto selection mode.
7. Application receives synchronous Status which indicates if the request to set config auto selection mode was sent successfully.
8. Application is notified of the Status of the request setAutoSelectionMode (either SUCCESS or FAILED) via the application-supplied callback.
9. Application sends a request to get config auto selection mode.
10. Application receives synchronous Status which indicates if the request to get config auto selection mode was sent successfully.
11. Application is notified of the Status of the request setAutoSelectionMode (either SUCCESS or FAILED) via the application-supplied callback, along with mode and slot id.

Sensor
------

The sensor sub-system provides APIs to configure and acquire continuous stream of data from an
underlying sensor, create multiple clients for a given sensor, each of which can have their own
configuration (sampling rate, batch count) for data acquisition.

The sensor sub-system APIs are synchronous in nature.

Call flow for sensor sub-system start-up
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/sensor_subsystem_startup_call_flow.png

1. Get the reference to the SensorFactory, with which we can further acquire other sensor sub-system
   objects.
2. Prepare an initialization callback method or lambda which will be called by the sensor sub-system
   once the initialization is complete.
3. Request for the SensorManager from SensorFactory and provide the initalization callback. Retain
   the SensorManager shared_ptr as long as necessary. SensorFactory does not hold on to the returned
   instance. If the received shared_ptr is released, SensorManager would be destroyed and requesting
   SensorFactory for SensorManager again would result in the creation of a new instance. Similar is the
   approach for SensorFeatureManager.
4. Wait for the initialization callback to be invoked.
5. The sensor sub-system invokes the callback once the underlying sub-system and sensor framework is
   available for usage. If the service status is notified as SERVICE_FAILED, retry intitialization
   starting with step (3). If the service status is notified as SERVICE_AVAILABLE, the sensor
   sub-system is ready for usage.

Call flow for sensor data acquisition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/sensor_data_acquisition_call_flow.png

The sensor sub-system should have been initialized successfully with SERVICE_AVAILABLE as a
pre-requisite for sensor data acquisition and a valid SensorManager object is available.

1. Get the information about available sensors from ISensorManager. The information will be provided
   in the sensorInfo parameter that would be passed by reference.
2. Given the information about different sensors, identify the required sensor (name of the sensor)
   using the provided attributes - type, name, vendor or required sampling frequency. Having identified the required sensor,
   request for the sensor object from ISensorManager with the required name of the sensor. If the request
   was successful, the provided reference to shared_ptr<ISensorClient> would be set by the ISensorManager
   which can be used to further configure and acquire data from the sensor. The ownership of the
   sensor object is with the application. SensorManager does not hold a reference to the returned
   instance. So, requesting ISensorManager for a sensor with the same name would result in a new
   sensor object being returned.
3. Create a listener of type ISensorEventListener which would receive notifications about updates
   to sensor configuration and sensor events. Register the created listener with the sensor object.
4. Create the desired sensor configuration. For continous data acquisition samplingRate and
   batchCount are necessary attributes to be set in the configuration. Be sure to set the validityMask
   in the SensorConfiguration structure.
5. On successful configuration, a notification is sent to the registered listeners indicating the
   configuration set.
6. Activate the sensor for acquiring sensor data.
7. When the sensor is activated successfully, the sensor data is sent to the registered listeners.

Call flow for sensor reconfiguration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/sensor_reconfiguration_call_flow.png

When the sensor sub-system has been initialized successfully with SERVICE_AVAILABLE and is already
activated, reconfiguring the sensors involves the following steps.

1. Deactivate the sensor. This will stop the notifications about sensor events to the registered
   listeners.
2. Configure the sensor with the required attributes. Be sure to set the validityMask for the all
   required attributes in SensorConfiguration.
3. The underlying sub-system notifies the registered listeners about the new configuration set.
4. Activate the sensor.
5. When the sensor is activated successfully, the sensor data is sent to the registered listeners
   as per the new configuration.

Call flow for sensor sub-system cleanup
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/sensor_subsystem_cleanup_call_flow.png

When the sensor sub-system has been initialized successfully with SERVICE_AVAILABLE and sensor
objects have been created, the following steps ensure a cleanup of the sensor sub-system.

1. Deactivate all the sensors. With this, the registered listeners will no longer be notified of the
   sensor events.
2. Release all the sensor objects created by setting them to nullptr. Since the application owns the
   objects, this would result in all the sensor objects getting destroyed.
3. Disable all the sensor features that were previously enabled.
4. Release the instance of ISensorManager by setting it to nullptr. Since the application owns the
   object, this would result in the sensor manager getting destroyed.
5. Release the instance of ISensorFeatureManager by setting it to nullptr. Since the application
   owns the object, this would result in the sensor feature manager getting destroyed.

Call flow for sensor power control
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/sensor_power_control_call_flow.png

The below points are to be noted for sensor power control

a) Power control is not offered by all sensor manufacturers. If the underlying hardware sensor does
not support power control, the power control APIs fail.
b) Enabling or disabling low power mode for the sensor is only possible when the sensor is not
activated.

For achieving power control, the following steps are to be followed

1. Deactivate the sensor. This will stop the notifications about sensor events to the registered
   listeners.
2. Perform the required power control by enabling or disabling low power mode for the sensor.
3. Activate the sensor.
4. When the sensor is activated successfully, the sensor data is sent to the registered listeners.

Call flow for sensor feature control
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/sensor_feature_control_call_flow.png

The sensor sub-system offers certain features in addition to the data acquisition and these could be
features offered by the underlying sensor hardware or the sensor software framework. If there are no
features offered collectively, the sensor feature manager initialization would fail.

1. Retrieve a list of features offered by the sensor sub-system and identify the required feature
   that needs to be enabled. If the feature that needs to be enabled is known, this step is optional.
2. Create a sensor feature event listener, which would be notified of the different events that
   occur. Register this listener with the sensor feature manager.
3. Enable the required feature.
4. If system is not in sleep mode, Once the feature is enabled and an event related to the feature occurs, the listener is notified.
5. If system is in sleep mode, Once the feature is enabled and an event related to the feature occurs, the listener is notified.

Platform
--------

The platform sub-system provides APIs to configure and control platform functionalities.
This sub-system provides notifications about certain system related events, for instance filesystem
events such as EFS restore and backup events.

Call flow for EFS restore notification registration and handling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/platform_efs_backup_restore_call_flow.png

1. Get the reference to the PlatformFactory, with which we can further acquire other sub-system
   objects.
2. Prepare an initialization callback method or lambda which will be called by the platform
   sub-system once the initialization is complete.
3. Request for the IFsManager (filesystem manager) object from PlatformFactory and provide the
   initalization callback. Retain the IFsManager shared_ptr as long as necessary. PlatformFactory does
   not hold on to the returned instance. If the received shared_ptr is released, FsManager would be
   destroyed and requesting PlatformFactory for FsManager again would result in the creation of a new
   instance.
4. Wait for the initialization callback to be invoked. The platform sub-system invokes the callback
   once the underlying sub-system is available for usage. If the service status is notified as
   SERVICE_FAILED, retry intitialization starting with step (3). If the service status is notified as
   SERVICE_AVAILABLE, the filesystem manager is ready for usage.
5. Create an listener object of type IFsListener and register with IFsManager for notifications.
   Once registered, the listener receives service status notifications and EFS restore event
   notifications.
6. If a service status notification with status SERVICE_UNAVAILABLE, the application should wait
   for service to be re-initialized and once done, SERVICE_AVAILABLE will be notified. If the service
   fails, SERVICE_FAILED is notified and the IFsManager object held is no longer usable. A new object
   of type IFsManager needs to be re-acquired from the PlatformFactory.
7. When the application finds it appropriate to trigger a EFS backup, startEfsBackup should be
   invoked, which would return immediately indicating the status of the request
8. Once EFS backup starts and completes, the notifications are sent out to the application.
   EFS restoration is controlled by the internal components based on several factors like
   identification of critical filesystem errors which result in failure to boot. When such conditions
   are identified, EFS restore events would be received by the application via the OnEfsRestoreEvent
   method and the application can make use of the information appropriately. The EFS restore
   notification also has information if the restore started or ended and if the restore was successful
   or a failure.
9. Once clean-up is necessary, deregister the registered listener, set all shared pointers to
   nullptr. This will make the underlying sub-system relinquish resources that are no longer
   necessary.

Call flow of control filesystem for eCall operation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/control_filesystem_for_ecall_start_end_call_flow.png

The platform sub-system should have been initialized successfully with SERVICE_AVAILABLE as a
pre-requisite for any filesystem operations and a valid FilesystemManager object is available.

1. Before initiate an eCall, the application should prepare the filesystem for eCall operation,
   prepareForEcall should be invoked. If the status of this request fails, the client could re-invoke
   during an eCall ongoing.
2. The status of the request would return immediately indicating the preparation of the filesystem
   operation.
3. The client should start an eCall immediately even if the prepare for eCall request failed.
4. The filesystem manager shall notify the application when filesystem operation is about to resume.
5. If the client wants to suspend the filesystem operation to continue the eCall, they should invoke
   the prepareForEcall API.
6. Once an eCall completes, the client should notify eCall completion to the filesystem manager.
   eCallCompleted should be invoked.
7. The status of the request would return immediately indication the completion of the filesystem
   operation.

Call flow of control filesystem for OTA operation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/control_filesystem_for_ota_start_end_call_flow.png

The platform sub-system should have been initialized successfully with SERVICE_AVAILABLE as a
pre-requisite for any filesystem operations and a valid FilesystemManager object is available.

1. Before initiate an OTA update, the application should prepare the filesystem for the OTA
   operation, prepareForOta should be invoked, which would return immediately indicating the
   status of the request.
2. Once the filesystem has prepared for the OTA operation, the filesystem manager would invoke the
   callback which indicates the response of the API.
3. Once the filesystem has prepared for the OTA operation, the client could initiate an OTA update.
4. On OTA update completion, the client should notify the filesystem manager of the OTA
   completion status, otaCompleted should be invoked, which would return immediately indicating
   the status of the request.
5. The filesystem manager would intern update the OTA status, the callback would invoked
   which indicates the response of the API.
6. If the client decides to mirror the system, startAbSync should be invoked which would return
   immediately indicating the status of the request.
7. Once the filesystem partition sync opration complete, the filesystem manager would invoke the
   callback which indicates the response of the API.

Call flow for sensor self test
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/sensor_self_test_call_flow.png

Certain sensors offer self test feature which can be invoked whenever needed by the application
using the selfTest API. The sensor sub-system should have been initialized successfully with
SERVICE_AVAILABLE as a pre-requisite for sensor data acquisition and a valid SensorManager object
is available.

1. Get the information about available sensors from ISensorManager. The information will be provided
   in the sensorInfo parameter that would be passed by reference.
2. Given the information about different sensors, identify the required sensor (name of the sensor)
   using the provided attributes - type, name, vendor or required sampling frequency. Having identified the required sensor,
   request for the sensor object from ISensorManager with the required name of the sensor. If the request
   was successful, the provided reference to shared_ptr<ISensorClient> would be set by the ISensorManager
   which can be used to further configure and acquire data from the sensor. The ownership of the
   sensor object is with the application. SensorManager does not hold a reference to the returned
   instance. So, requesting ISensorManager for a sensor with the same name would result in a new
   sensor object being returned.
3. Trigger the self test with the require self test type and provide a callback that would be
   invoked by the sensor framework once the self test is completed.
4. Once the self test is completed, the callback gets invoked indicating the result of the self
   test.

Security Management
-------------------

Crypto
~~~~~~

The CryptoManager class provides APIs to generate, import, export and upgrade key based on various cryptographic algorithms. Data can be signed and verified using this key. Further data can be encrypted and decrypted with the key.

Call flow to generate and export key
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/crypto_gen_key_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoManager.
4. An instance of ICryptoManager is received by the application.
5. Application creates an instance of ICryptoParam using CryptoParamBuilder to define input parameters for the key.
6. Application calls generateKey() API of ICryptoManager. Now, the application has a key blob which represents the crypto key. Application should use this key blob for signing, verification, encryption and decryption operations.
7. For a given use-case, if the application wants to extract public key out of the key blob, it can do so by calling export key.
8. ICryptoManager returns key in the format requested for ex; X509.

Call flow to sign and verify data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/crypto_sign_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoManager.
4. Application creates an instance of ICryptoParam using CryptoParamBuilder to define input parameters for how the signing should occur.
5. Application calls signData() API of ICryptoManager passing the key blob, data to be signed and input parameters.
6. ICryptoManager returns signature corresponding to the inputs given.
7. Application creates an instance of ICryptoParam using CryptoParamBuilder to define input parameters for how the verification should occur.
8. Application calls verifyData() API of ICryptoManager passing the key blob, signature, data to be verified and input parameters.
9. ICryptoManager returns success if the verification succeeds (valid data and signature) otherwise appropriate error code.

Call flow to encrypt and decrypt data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/crypto_enc_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoManager.
4. Application creates an instance of ICryptoParam using CryptoParamBuilder to define input parameters for how the encryption should occur.
5. Application calls encryptData() API of ICryptoManager passing the key blob, data to be encrypted and input parameters.
6. ICryptoManager returns encrypted data corresponding to the inputs given.
7. Application creates an instance of ICryptoParam using CryptoParamBuilder to define input parameters for how the decryption should occur.
8. Application calls decryptData() API of ICryptoManager passing the key blob, encrypted data and input parameters.
9. ICryptoManager returns decrypted data otherwise appropriate error code.

Crypto Accelerator
~~~~~~~~~~~~~~~~~~

The ICryptoAcceleratorManager provides APIs to verify signature and do ECQV calculation based on elliptic-curve cryptography.

Call flow for signature verification synchronous mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/ecc_verify_sync_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoAcceleratorManager.
4. An instance of ICryptoAcceleratorManager is received by the application.
5. Application defines input parameters for verification.
6. Application calls eccVerifyDigest() API of ICryptoAcceleratorManager.
7. ICryptoAcceleratorManager returns success if verification passed otherwise appropriate error code.

Call flow for signature verification asynchronous poll mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/ecc_verify_async_poll_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoAcceleratorManager.
4. An instance of ICryptoAcceleratorManager is received by the application.
5. Application defines input parameters for verification.
6. Application calls eccPostDigestForVerification() API of ICryptoAcceleratorManager.
7. ICryptoAcceleratorManager returns success if data is sent for verification.
8. Application calls getAsyncResults() API of ICryptoAcceleratorManager to get the results.
9. ICryptoAcceleratorManager returns verification data obtained from crypto accelerator.
10. Application calls various APIs of ResultParser to get exact verification result.

Call flow for signature verification asynchronous listener mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/ecc_verify_async_listener_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoAcceleratorManager.
4. An instance of ICryptoAcceleratorManager is received by the application.
5. Application defines input parameters for verification.
6. Application calls eccPostDigestForVerification() API of ICryptoAcceleratorManager.
7. ICryptoAcceleratorManager returns success if data is sent for verification.
8. Application receives result in method onVerificationResult() of class implementing ICryptoAcceleratorListener interface.

Call flow for ECQV calculation synchronous mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/ecqv_calc_sync_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoAcceleratorManager.
4. An instance of ICryptoAcceleratorManager is received by the application.
5. Application defines input parameters for calculation.
6. Application calls ecqvPointMultiplyAndAdd() API of ICryptoAcceleratorManager.
7. ICryptoAcceleratorManager returns success if calculation result otherwise appropriate error code.

Call flow for ECQV calculation asynchronous poll mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/ecqv_calc_async_poll_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoAcceleratorManager.
4. An instance of ICryptoAcceleratorManager is received by the application.
5. Application defines input parameters for calculation.
6. Application calls ecqvPostDataForMultiplyAndAdd() API of ICryptoAcceleratorManager.
7. ICryptoAcceleratorManager returns success if data is sent for calculation.
8. Application calls getAsyncResults() API of ICryptoAcceleratorManager to get the results.
9. ICryptoAcceleratorManager returns calculation data obtained from crypto accelerator.
10. Application calls various APIs of ResultParser to get exact calculation result.

Call flow for ECQV calculation asynchronous listener mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/ecqv_calc_async_listener_call_flow.png

1. Application request an instance of SecurityFactory.
2. An instance of SecurityFactory is received by the application.
3. From the SecurityFactory, application request an instance of ICryptoAcceleratorManager.
4. An instance of ICryptoAcceleratorManager is received by the application.
5. Application defines input parameters for calculation.
6. Application calls ecqvPostDataForMultiplyAndAdd() API of ICryptoAcceleratorManager.
7. ICryptoAcceleratorManager returns success if data is sent for calculation.
8. Application receives result in method onCalculationResult() of class implementing ICryptoAcceleratorListener interface.

Cellular Connection Security
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ICellularSecurityManager provides support for detecting, monitoring and generating security
threat scan report for cellular connections.

Call flow to register listener and receive reports (Cellular)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/cellular_consec_listener.png

1. Application request an instance of ConnectionSecurityFactory.
2. An instance of ConnectionSecurityFactory is received by the application.
3. From the ConnectionSecurityFactory, application request an instance of ICellularSecurityManager.
4. An instance of ICellularSecurityManager is received by the application.
5. Application registers CellSecurityReportListener listener with ICellularSecurityManager.
6. Application receives security reports in onScanReportAvailable() callback method in CellSecurityReportListener listener.
7. When use-case is complete application deregisters CellSecurityReportListener listener.

Wi-Fi connection security
~~~~~~~~~~~~~~~~~~~~~~~~~

The IWiFiSecurityManager provides support for detecting, monitoring and generating security
threat report for WiFi connections.

Call flow to register listener and receive reports (Wi-Fi)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: /../images/wifi_consec_listener.png

1. Application request an instance of ConnectionSecurityFactory.
2. An instance of ConnectionSecurityFactory is received by the application.
3. From the ConnectionSecurityFactory, application request an instance of IWiFiSecurityManager.
4. An instance of IWiFiSecurityManager is received by the application.
5. Application waits for the service to become available.
6. When service is ready, application registers IWiFiReportListener listener with IWiFiSecurityManager.
7. Application receives security reports in onReportAvailable() callback method in IWiFiReportListener listener.
8. When use-case is complete application deregisters IWiFiReportListener listener.

WLAN
----

Call flow to modify WLAN configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/wlan_modify_config.png

1. Application requests IWlanDeviceManager object from WLAN factory.
2. WLAN factory returns a shared pointer to the IWlanDeviceManger object to the application.
3. Application can use IWlanDeviceManager::getServiceStatus to determine if the system is ready.
4. The application receives the status, i.e., SERVICE_AVAILABLE or SERVICE_UNAVAILABLE, to indicate if the subsystem is ready.

   a. If the subsystem is not ready, the application can wait for the callback provided in Step 1 for subsystem initialization status.
   b. Application provided callback is invoked with subsystem status (SERVICE_AVAILABLE/SERVICE_FAILED).

5. When the subsystem is ready, application calls IWlanDeviceManager::getStatus to get WLAN enablement status.
6. Application receives WLAN enablement status.

   a. If WLAN is enabled, application calls IWlanDeviceManager::enable(false) to disable WLAN.
   b. Application receives WLAN disablement response and waits for WLAN status indication.
   c. Application receives WLAN status indication IWlanDeviceManager::onEnableChanged(false) to indicate WLAN is disabled.

7. Application sets desired configuration using IWlanDeviceManager::setMode to set desired number of access points and stations to be enabled.
8. Application receives response.
9. Application calls IWlanDeviceManager::enable(true) to enable WLAN.
10. Application receives WLAN enablement response and waits for WLAN status indication.
11. Application receives WLAN status indication IWlanDeviceManager::onEnableChanged(true) to indicate WLAN is enbaled.

Call flow to modify WLAN station configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/wlan_modify_station_config.png

1. Application requests IWlanDeviceManager object from WLAN factory.
2. WLAN factory returns shared pointer to IWlandeviceManager to the application.
3. Application can use IWlanDeviceManager::getServiceStatus to determine if the system is ready.
4. The application receives the Status i.e. either SERVICE_AVAILABLE or SERVICE_UNAVALABLE to indicate whether sub-system is ready or not.

   a. If subsystem is not ready, then the application could wait for callback provided in step 1 for subsystem initialization status.
   b. Application pprovided callback is invoked with subsystem status (SERVICE_AVAILABLE/SERVICE_FAILED).

5. On Readiness, requests IStaInterfaceManager object from WLAN factory.
6. WLAN factory returns shared pointer to IStaInterfaceManager to the application.
7. Application calls IStaInterfaceManager::setIpConfig to set desired IP configurations.
8. Application receives response set IP configuration.
9. Application calls IStaInterfaceManager::managerStaService to restart wpa_supplicant daemon.
10. Application receives response to restart wpa_supplicant daemon and station configuration shall be active at this stage.

Call flow to Modify WLAN Access Point Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/wlan_modify_ap_config.png

1. Application requests IWlanDeviceManager object from WLAN factory.
2. WLAN factory returns shared pointer to IWlandeviceManager to the application.
3. Application can use IWlanDeviceManager::getServiceStatus to determine if the system is ready.
4. The application receives the Status i.e. either SERVICE_AVAILABLE or SERVICE_UNAVALABLE to indicate whether sub-system is ready or not.

   a. If subsystem is not ready, then the application could wait for callback provided in step 1 for subsystem initialization status.
   b. Application pprovided callback is invoked with subsystem status (SERVICE_AVAILABLE/SERVICE_FAILED).

5. On Readiness, requests IApInterfaceManager object from WLAN factory.
6. WLAN factory returns shared pointer to IApInterfaceManager to the application.
7. Application calls IApInterfaceManager::setConfig to set desired IP configurations.
8. Application receives response set configuration.
9. Application calls IApInterfaceManager::managerApService to restart hostapd daemon.
10. Application receives response to restart hostapd daemon and access point configuration shall be active at this stage.

SATCOM
------

Call flow to enable NTN and send non-IP data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/ntn_enable_and_send.png

1. Application requests INtnManager object from Satcom factory.
2. Satcom factory returns shared pointer to INtnManager to the application.
3. Application can use INtnManager::getServiceStatus to determine if the system is ready.
4. The application receives the status, i.e., either SERVICE_AVAILABLE or SERVICE_UNAVALABLE to indicate whether the subsystem is ready or not.
5. If the subsystem is not ready, then the application could wait for callback provided in step 1 for subsystem initialization status.
6. Application provided callback is invoked with subsystem status (SERVICE_AVAILABLE/SERVICE_FAILED).
7. Application registers listener for notifications related to NTN state/data etc.
8. Application receives the status (SUCCESS or suitable failure) based on registration of listener to NtnManager.
9. Application optionally sends SFL configuration to be used by the modem.(mcc, mnc, bands and earfcns)
10. Application receives the result (SUCCESS or suitable failure) of SFL configuration request.
11. Application enables NTN.
12. Application receives the NTN state change notification when NTN is enabled and ready to be used to send non-IP data.
13. Application also receives the signal strength change notification indicating the new signal strength.
14. Application requests the capabilities such as maximum data size for the connected NTN network.
15. Application receives the capabilities result.
16. Application sends non-IP data.
17. Application receives transaction ID of the send request. This can be used to map the data acknowledgement that could come later.
18. Application receives the L2 acknowledgement for the sent data packet.
19. Application disables the NTN.
20. Application receives the result of the request to disable the NTN.
21. Application receives the state change notification.

Diagnostics
-----------

Call flow to collect logs using file method
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/diag_file_method.png

1. Application requests IDiagLogManager object from Diagnostics factory.
2. Diagnostics factory returns a shared pointer to the Diagnostics factory object to the application.
3. Application shall wait for init callback
4. Application shall ensure subsystem is ready before proceeding
5. Application can use IDiagLogManager::getStatus to determine the current system status.
6. Application can check returned status to ensure diag logging is not already in progress.
7. Application can call IDiagLogManager::setConfig to set desired configurations.
8. Application must ensure the return status is Success.
9. Application can call IDiagLogManager::startLogCollection to start logging.
10. Application must ensure the return status is Success.
11. Application can IDiagLogManager::stopLogCollection to stop logging.
12. Application must ensure the return status is Success.

Call flow to collect logs using callback method
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /../images/diag_callback_method.png

1. Application requests IDiagLogManager object from Diagnostics factory.
2. Diagnostics factory returns a shared pointer to the Diagnostics factory object to the application.
3. Application shall wait for init callback
4. Application shall ensure subsystem is ready before proceeding
5. Application can use IDiagLogManager::getStatus to determine the current system status.
6. Application can check returned status to ensure diag logging is not already in progress.
7. Application can call IDiagLogManager::setConfig to set desired configurations.
8. Application must ensure the return status is Success.
9. Application can call IDiagLogManager::startLogCollection to start logging.
10. Application must ensure the return status is Success.
11. TelSDK will invoke callback provided by the application whenever new log entry is availabe.
12. Application can IDiagLogManager::stopLogCollection to stop logging.
13. Application must ensure the return status is Success.
