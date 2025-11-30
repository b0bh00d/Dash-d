#!/bin/bash

# Use this shell script as an interface for the systemd service
# file.  It sets up the Python virtual environment, and then runs
# the Sensor script in a way that allows it to receive the signals
# from systemd (instead of this bash script).

# The virtual environment folder that has watchdog installed
# (change the path here to match your system)
cd /home/bob/check-space

# Activeate the virtual environment
source bin/activate

# Use 'exec' to start the Python process so IT receives the signals,
# not the bash script
exec python3 ./check-space.py --sensor-data /tmp/dash-d --watchdog

# We actually won't get here (but it's here for completeness)
deactivate
exit 0
