# CV2X CONFIG APP

The cv2x_config_app is an open source C-V2X software tool that can be used for testing
the following functionalities.

- Retrieving the C-V2X configuration being used from the lower layer
- Updating the C-V2X configuration
- Enforcing C-V2X configuration expiration
- Enabling or disabling SLSS

Note that the XML file containing the C-V2X configruation is not maintained or readable
in the Linux file-system, it has to be retrieved out of the EFS using TelSDK API. The
exact name and path of the file is not significant, but corresponding SELINUX rules need
to be added for the APP for file read or write permission.

## Retrieve the C-V2X configuration

Call the ICv2xConfig::retrieveConfiguration() TelSDK API to retrieve the current C-V2X
configuration from the lower layer and store it to a user-specified XML file.

## Update the C-V2X configuration

Calling the ICv2xConfig::updateConfiguration() TelSDK API to send the contents of the
user-specicifed XML file to the lower layer. The new C-V2X configuration will be used validation
succeeds. C-V2X status will transition to SUSPEND or INACTIVE and then back to ACTIVE once
the C-V2X radio is ready for transmission or reception.

## Enforce C-V2X configuration expiration

Add an <Expiration> leaf with a future UTC timestamp in seconds to the C-V2X configuration
XML file and send the XML file to the lower layer to have the new C-V2X configuration used
in the lower layer and invalidated at the expiration timestamp. After that, the C-V2X radio will
transition to INACTIVE if there is no C-V2X preconfig from hardware MBN, otherwise it will fallback
to the C-V2X preconfig.

## Enable or disable SLSS

A UE out of GNSS coverage can sync to another UE that has good timing through SLSS, if both UEs
support SLSS. When SLSS is enabled, WWAN time tagging and Tunnel mode will be impossible and
GNSS will always have priority when available.

To enable SLSS in the lower layer, the following requirements must be met.

- The <RadioParametersContents> of the C-V2X configuration XML file must support SLSS according to
     either of the following variants (need ASN.1 decode and encode).

    - 3GPP SLSS variant - v2x-CommPreconfigSync-r14 is valid

    - non-3GPP SLSS variant – sl-Subframe-r14 is 100 bitmap

- SLSS is enabled using either of the following approaches.

    - Send an XML file that contains standalone SLSS configuration (format described in
      [Standalone SLSS configuration](#Standalone-SLSS-configuration)) to the lower layer.
      The standalone SLSS configuration will be invalid after device power-off.

    - Send an XML file that contains the traditional C-V2X configuration and the SLSS
      configuration (format described in
      [Traditional C-V2X configuration + SLSS configuration](#Traditional-C-V2X-configuration-+-SLSS-configuration))
      to the lower layer. The SLSS configuration can sustain across device power-cycle.

### SLSS configuration

The SLSS configuration applies to all the geopolygons and it's not affected by the expiration of
C-V2X configuration. The parameters of SLSS configuration are described in 5.2.

#### Standalone SLSS configuration

The following example illustrates a standalone SLSS configuration. The lower layer will combine the current
C-V2X configuration and the SLSS configuration to decide whether to enable or disable SLSS.

    <SLSSConfig config="SLSS">
      <Rel14>
        <TxEnabled>1</TxEnabled>
        <RxEnabled>1</RxEnabled>
        <SyncTxParameters>23</SyncTxParameters>
        <SyncTxThreshOoc>4</SyncTxThreshOoc>
        <FilterCoefficient>0</FilterCoefficient>
        <SyncRefMinHyst>1</SyncRefMinHyst>
        <SyncRefDiffHyst>1</SyncRefDiffHyst>
      </Rel14>
    </SLSSConfig>

#### Traditional C-V2X configuration + SLSS configuration

    The SLSS configuration is added in the <Ext> leaf at the bottom of a traditional C-V2X
    configuration XML file. Both C-V2X configuration and SLSS configuration will be used after
    sending this XML file to the lower layer.

    <V2X>
      <V2XoverPC5>
      ...
      </V2XoverPC5>
      <Ext>
        <SLSSConfig config="SLSS">
          <Rel14>
            <TxEnabled>1</TxEnabled>
            <RxEnabled>1</RxEnabled>
            <SyncTxParameters>23</SyncTxParameters>
            <SyncTxThreshOoc>4</SyncTxThreshOoc>
            <FilterCoefficient>0</FilterCoefficient>
            <SyncRefMinHyst>1</SyncRefMinHyst>
            <SyncRefDiffHyst>1</SyncRefDiffHyst>
          </Rel14>
        </SLSSConfig>
      </Ext>
    </V2X>

###  Mandatory SLSS parameters

#### TxEnabled

Parameter to enable or disable SLSS Tx. Set this value to 1 for RSUs capable of SLSS
and 0 for OBUs.

- 1 - Enable SLSS transmission
- 0 - Disable SLSS transmission; default

#### RxEnabled

Parameter to enable or disable SLSS Rx. Set this value to 1 for RSUs/OBUs capable of SLSS.

- 1 - Enable SLSS reception
- 0 - Disable SLSS reception; default

#### Optional SLSS parameters

The following SLSS parameters are optional. They can be used for the "non-3GPP SLSS variant" to
specify corresponding SLSS radio parameters. Default values will be used if they're absent.
Partial setting of these parameters is not supported. If a user needs to modify one of the
optional parameters, they need to set all of the optional parameters.

These parameters are defined in 3GPP 36.331.

##### SyncTxParameters

      SLSS transmission power.
      Value: [-126, 31] dBm
      Default value: 23

##### SyncTxThreshOoc

      SLSS transmission threshold.
      Value: [0,11] 0:-110dBm, 1:-105dBm...10:-60dBm, 11:infinity
      Default value: 4

##### FilterCoefficient

      RSRP filtering coefficient.
      Value: [0,19]
      Default value: 0

##### SyncRefMinHyst

      Hysteresis when evaluating a syncRefUE using absolute comparison.
      Value: [0,4] 0:0dB, 1:3dB, 2:6dB, 3:9dB, 4：12dB
      Default value: 1

##### SyncRefDiffHyst

      RSRP filtering coefficient.
      Value: [0,5] 0:0dB, 1:3dB, 2:6dB, 3:9dB, 4：12dB, 5:infinity
      Default value: 1

## Disable T5000 timer

T5000 timer is a configuration for the applicability of privacy for V2X communication over PC5,
indicating how often the UE shall change the source Layer-2 ID and source IP address (for IP
data) self-assigned by the UE for V2X communication over PC5. The value of T5000 timer is
configurable by setting the "TimerT5000" value under the "PrivacyConfig" in a v2x.xml, the
default value is 300, in units of seconds. But no matter what value the T5000 timer is set to,
the randomization of source Layer-2 ID will always happen on start-up.

T5000 timer should be disabled if
A/ ITS stack is changing the source Layer-2 ID via API, to coincide with other identifier changes
like the signing certs.
B/ RSUs don't want to change the source Layer-2 ID.
C/ UEs don't want to change the source Layer-2 ID while a unicast session is ongoing.

T5000 timer can be disabled either by removing the "PrivacyConfig" leaf or setting a value 0 to "TimerT5000".

    <PrivacyConfig>
      <TimerT5000>300</TimerT5000>
    </PrivacyConfig>

Note that even if T5000 timer is disabled, the randomization of source Layer-2 ID could still happen when
a Layer-2 ID collision is detected on the radio.

## Test tool options

  After executing "cv2x_config_app", user needs to input option 1/2/3 to select the next step.

- Option 1 – Retrieve C-V2X configuration and store to the specified XML file.
  In selinux permissive mode, the path must be "/var/tmp/", the file name doesn't matter.

- Option 2 - Send C-V2X configuration or SLSS configuration in the specified XML file to the low layer.
  In selinux permissive mode, restorn the XML file context by issuing
  "adb shell restorecon -F /var/tmp/v2x.xml" before updating C-V2X configuration using this tool.

- Option 3 - Retrieve C-V2X configuration from the low layer and store to a temporary XML file
  "/var/tmp/v2x.xml", generate a new XML file "/var/tmp/expiry.xml" based on "/var/tmp/v2x.xml"
  and the expiration UTC timestamp specified by user, and then send file "/var/tmp/expiry.xml"
  to the low layer.

### Test example

    / # cv2x_config_app
    ------------------------------------------------
                    Cv2x Config Menu
    ------------------------------------------------

        1 - Retrieve_Config
        2 - Update_Config
        3 - Enforce_Config_Expiration

        ? / h - help
        q / 0 - exit

    ------------------------------------------------

    config> 1
    CV2X config file will be stored in /var/tmp/
    Enter the XML file name(e.g., v2x.xml): v2x.xml
    Retrieving config file...
    Config file saved to /var/tmp/v2x.xml with success.
    ------------------------------------------------
                    Cv2x Config Menu
    ------------------------------------------------

        1 - Retrieve_Config
        2 - Update_Config
        3 - Enforce_Config_Expiration

        ? / h - help
        q / 0 - exit

    ------------------------------------------------

    config> 2
    Put the v2x configuration XML file under /var/tmp/
    Then enter the file name(e.g., v2x.xml): v2x.xml
    Updating config file...
    Update config file successfully.
    ------------------------------------------------
                Cv2x Config Menu
    ------------------------------------------------

        1 - Retrieve_Config
        2 - Update_Config
        3 - Enforce_Config_Expiration

        ? / h - help
        q / 0 - exit

    ------------------------------------------------

    config> 3
    Retrieving config file...
    Generating expiry config file...
    Current timestamp:1671437757
    Enter config expiry timestamp: 1671437765
    Current timestamp:1671437762
    Updating config file...
    Waiting for config expiry indication...
    Waiting for config changed indication...
    Enforce expiration of Cv2x config successfully.
    ------------------------------------------------
                Cv2x Config Menu
    ------------------------------------------------

        1 - Retrieve_Config
        2 - Update_Config
        3 - Enforce_Config_Expiration

        ? / h - help
        q / 0 - exit

    ------------------------------------------------

    config> 0
    / #
