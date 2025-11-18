.. _remote-sim-reference-apps:

Using remote SIM reference apps 
============================================================

This section describes how to use the provided Remote SIM reference apps – remote-sim-daemon and
sap-card-provider. The remote-sim-daemon app will run on the device without a SIM, while the
sap-card-provider app will run on the device with a SIM. The two apps will communicate over a
standard IP Ethernet connection, providing the WWAN capabilities of the remote SIM card to the
device without a SIM inserted. Both devices are required to have support for the Telematics SDK and
to be connected to each other via Ethernet.

**Required Items**

It is assumed that both devices are configured to enable the necessary features to support Remote
SIM capabilities.

1. Set Up the Ethernet connection

  First, connectivity between the two devices needs to be established.

1.1 Disable Automatic Configuration IP Address

  Depending on the device type, it may be necessary to first disable the auto-config IP (169.254.x.x) address on both devices.

  .. code-block::

    brctl delif bridge0 eth0


1.2 Configure the IP Address on Both Devices

  .. code-block::

    ifconfig eth0 192.168.1.2

The device with a SIM can use 192.168.1.3.

NOTE: Depending on the device, it may be necessary to execute steps 1.1 and 1.2 again whenever
either device restarts or is disconnected, due to auto-config settings.

2. Run the Remote SIM Daemon in the Background on the Device Without a SIM

.. code-block::

  remote-sim-daemon &

The -d and -s flags can also be used for debugging purposes (use -h for usage instructions).

3. Run the Sap Card Provider on the Device with a SIM

.. code-block::

  sap-card-provider -i 192.168.1.2

Use the -i flag to provide the IP address of the device running remote-sim-daemon. The -d and -s
flags can also be used for debugging purposes (use -h for usage instructions).
