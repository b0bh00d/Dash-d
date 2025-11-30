## check-space

This sample Sensor is designed to monitor the available free space on one or more Linux
partitions.

### Partitition identification

The `check-space` script is designed to automatically identity mounted partitions that need
to be monitored.  This is accomplished by setting/adding the string `DASHD` to either a
patition's LABEL or PARTLABEL attribute.  This can be accomplished in several ways: parted,
gparted, e2label, etc.).  If both of these settings already have values, `DASHD` can be
added (e.g., given a "BOB" label, you can insert a separator to get "BOB|DASHD"), as long as
that doesn't break some other functionality that is depending on the existing label.

### Event-driven updates: watchdog

`check-space` can optionally use the `watchdog` module to monitor changes in the target file
system.  When enabled, instead of polling, the script run continuously, only generating a
Sensor report when an event happens on the file system.  If you do not enabled `watchdog`,
`check-space` will function as a singe-shot, existing after processing all RAID devices.

The `--sensor-data` option specifies where the Collector report folder is on the system.

Passing `--watchdog` will enable the use of the `watchdog` module, and send the script into
event mode--i.e., it will not return unless terminated.  The `watchdog` module should be
available within the python3 interpreter's current site packages.

#### systemd service

If you would like to employ the `watchdog` module, a systemd service file is included for
running the `check-space` Sensor in the background whenever the system starts up.

You have the option to create a virtual environment wherein you can install the `watchdog`
module.  A bash shell script, `check-raid.sh`, is included that will enter and activate
that virtual environment and then spawn the `check-space` Sensor.  This bash script is
intended to be the run target for the systemd service file.  On the other hand, if you
install `watchdog` into the system python3 site packages, you don't need a virtual
environment, and can alter the systemd service file to run the `check-space` script directly.

#### Collector monitoring

If you enable `watchdog`, Be sure to avoid using the `--detect-offline` option when
running the domain's Collector process.  The `check-space` sensor will not be sending
updates at predictable intervals, and as a result, the Collector may mistakenly determine
that it is offline.
