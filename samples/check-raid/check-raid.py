#!/usr/bin/env python3

#-------------------------------------------------------
# Check RAID by Bob Hood (A Dash'd Sensor)
#
# Monitor the health of supported RAIDs on this system.
#
# Should be run from cron at regular intervals.
#
# No AI was used in the creation of this script.
#-------------------------------------------------------

import os
import re
import sys
import json
import subprocess

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from typing import Iterator, Tuple
from collections.abc import Mapping

class States:
    NAMES=["healthy", "poor", "critical", "deceased", "offline"]
    HEALTHY, POOR, CRITICAL, DECEASED, OFFLINE = range(5)

class Levels:
    PERSONALITIES=["linear", "raid0", "raid1", "raid4", "raid5", "raid6", "raid10", "multipath", "faulty", "container"]
    LINUEAR, RAID0, RAID1, RAID4, RAID5, RAID6, RAID10, MULTIPATH, FAULTY, CONTAINER = range(10)

    # The list of RAID types that this script does not support
    UNSUPPORTED_RAIDS=["linear", "raid4", "raid6", "multipath", "faulty", "container"]

class MdStat:
    STATE, PERSONALITY, MEMBERS = range(3)

def construct_report(device: str, state: str, msg: str = "") -> Tuple[str, Mapping[str, str]]:
    sensor_name = f"raid_monitor_{device}"
    sensor_filename = f"{device}.json"

    return (sensor_filename, { "sensor_name" : sensor_name,
            "sensor_state" : state,
            "sensor_message" :  msg })

def find_raids() -> Iterator[str]:
    data = open('/proc/mdstat', 'r').readlines()
    device_re = re.compile(r'^(md\d+) \: (.+)')

    for line in data[1:]:
        line = line.strip()
        result = device_re.search(line)
        if result:
            md_items = result.group(2).split()
            md_members: list[str] = []
            for member in md_items[MdStat.MEMBERS:]:
                result2 = re.search(r'(\w+)\[.+?\]', member)
                if result2:
                    md_members.append(result2.group(1))
            if (md_items[MdStat.STATE] == "active") and \
               (md_items[MdStat.PERSONALITY] not in Levels.UNSUPPORTED_RAIDS):
                yield (result.group(1), Levels.PERSONALITIES.index(md_items[MdStat.PERSONALITY]), md_members)

def process_raids(sensor_path: str, test_mode: bool) -> None:
    raid_devices_re = re.compile(r'Raid Devices : (\d+)')
    total_devices_re = re.compile(r'Total Devices : (\d+)')
    failed_devices_re = re.compile(r'Failed Devices : (\d+)')

    device_count = 0
    for device in find_raids():
        device_count += 1
        device_path = os.path.join("/", "dev", device[0])

        output = subprocess.Popen(['mdadm', '--detail', device_path], stdout=subprocess.PIPE).communicate()[0].decode('utf-8')

        raid_devices = 0
        result = raid_devices_re.search(output)
        assert result is not None, "ERROR: Failed to locate 'Raid devices' in output!"
        raid_devices = int(result.group(1))

        total_devices = 0
        result = total_devices_re.search(output)
        assert result is not None, "ERROR: Failed to locate 'Total devices' in output!"
        total_devices = int(result.group(1))

        failed_devices = 0
        if raid_devices != total_devices:
            result = failed_re.search(output)
            assert result is not None, "ERROR: Failed to locate 'Failed devices' in output!"
            failed_devices = int(result.group(1))

        raid = Levels.PERSONALITIES[device[1]].upper()

        report_data: Tuple[str, Mapping[str, str]] = ()
        if failed_devices > 0:
            msg = f"{raid} '{device[0]}' is reporting {failed_devices}/{raid_devices} failing RAID members."
            match device[1]:
                case Levels.RAID0:
                    # RAID0 is striped
                    # The loss of ANY member is fatal
                    report_data = construct_report(device[0], States.NAMES[States.DECEASED], msg)
                case Levels.RAID1:
                    # RAID1 is mirrored
                    # The loss of all but two members is poor; the loss of all but one is critical; the loss of all is fatal
                    if raid_devices == failed_devices:
                        state = States.NAMES[States.DECEASED]    # array is unusable
                    elif (raid_devices - failed_devices) >= 2:
                        state = States.NAMES[States.POOR]        # data is safe, mirroring can continue to occur
                    elif (raid_devices - failed_devices) == 1:
                        state = States.NAMES[States.CRITICAL]    # data is safe, but no mirroring is occuring
                    report_data = construct_report(device[0], state, msg)
                case Levels.RAID5:
                    # RAID5 is striped with parity
                    # The loss of any single member is critical (accurate data reads can still occur); the loss of 2> is fatal
                    report_data = construct_report(device[0],
                        States.NAMES[States.DECEASED] if failed_devices > 1 else States.NAMES[States.CRITICAL],
                         msg)
                case _:
                    assert false, "ERROR: Unsupported RAID type '{raid}'"
        else:
            # operating normally
            msg = f"{raid} '{device[0]}' reports {raid_devices}/{total_devices} members operating normally."
            report_data = construct_report(device[0], States.NAMES[States.HEALTHY], msg)

        assert len(report_data) > 0, "ERROR: Failed to construct report data!"

        if test_mode:
            print(f"{report_data[0]}.json: {json.dumps(report_data[1])}")
        else:
            with open(os.path.join(sensor_path, report_data[0]), 'w') as f:
                f.write(json.dumps(report_data[1]))

    return device_count

if __name__ == "__main__":
    parser = ArgumentParser(description="A Dash'd sensor to monitor RAID devices", formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument('--sensor-data', type=str, metavar='DIR', default=None, help="Directory for Dash'd sensor data.")
    parser.add_argument('--test', action="store_true", default=False, help="Print Sensor data to console and exit.")
    options, args = parser.parse_known_args()

    if not options.test:
        if options.sensor_data is None:
            print("ERROR: A valid sensor path must be specified.")
            sys.exit(1)
        elif not os.path.exists(options.sensor_data):
            print("ERROR: The provided sensor path does not exist. (Is the Dash'd Collector running on this system?)")
            sys.exit(1)

    if process_raids(options.sensor_data, options.test) == 0:
        print("ERROR: There appear to be no active RAID arrays on this system.")
        sys.exit(1)

    sys.exit(0)
