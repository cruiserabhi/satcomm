Remote SIM provisioning app {#rsp_reference_apps}
=================================================

TelSDK provides reference app rsp_httpd which helps in exploring various features of remote provisioning APIs.

SIM profile management operations such as add profile, delete profile, enable/disable profile
requires HTTP interaction with the cloud to synchronize the profile/s on the eUICC with the
SMDP+/SMDS server. When modem LPA/eUICC needs to reach SMDP+/SMDS server on the cloud for HTTP
transaction, the HTTP request is sent to Remote SIM provisioning service i.e RSP HTTP daemon running
on Application Processor. The RSP HTTP Daemon performs these HTTP transactions on behalf of modem
with SMDP+/SMDS server running on the cloud. The HTTP response from cloud is sent back to modem
LPA/eUICC to take appropriate action. Make sure there is internet connectivity available for
performing HTTP transaction either using WLAN, WWAN or ethernet.

### 1. Run the remote SIM HTTP daemon on the device

~~~~~~{.sh}
# rsp_httpd
~~~~~~

Use -h option to check for all the options supported and usage instructions.
