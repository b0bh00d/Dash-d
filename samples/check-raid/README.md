## check-raid

Using /proc/mdstat, this sample Sensor will automatically detect all configured RAID
devices on the system, and begin depositing regular reports about their health into the
defined folder for the Dash'd Collector.

The `--sensor-data` option specifies where the Collector report folder is on the system.

You can use the `--test` option to generate a one-off report to the console.  Handy for
checking what is being sent to the Collector.

On a Linux system, this script has no external dependencies, so it can be run directly from
a cron job at whatever interval makes you comfortable.  I run mine every 5 minutes:

    `*/5 * * * * /root/check-raid.py --sensor-data /tmp/dash-d`
