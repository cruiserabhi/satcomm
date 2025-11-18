Sample Applications Configuration {#sample_apps_settings}
=========================================================

# Sample Applications Configuration

Telematics stand-alone applications like make_call_app, send_sms_app etc provides
flexibility to configure application config parameters using either user defined
config file (ex: appName.conf) or default config file (sample_app.conf).


Configuration file has application configurations in key-value pair.
For make_call_app sample application user can provide dial number in
the configuration file in order to make a voice call.

DIAL_NUMBER = +1234512345

For send_sms_app sample application user can provide receiver's phone
number and text message in the configuration file in order to send a SMS.

RECEIVER_NUMBER = +1234512345

MESSAGE = Text message

Below are the steps to run the sample application.
- Configure required parameters either in user defined config file (ex: appName.conf) or default config file.
- User provided config file should be placed where application is running.

- Execute below command to use user defined config

   ./make_call_app appName.conf

- Execute below command to leverage either default config file if present or use configuration defined in application itself.

   ./make_call_app