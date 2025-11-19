#!/usr/bin/env python3

#-------------------------------------------------------
# Check Space by Bob Hood (A Dash'd Sensor)
#
# Monitor the availble free space on a partitiion.  It
# expects the partition to be labeled in a way that
# makes it discoverable without user interaction.
#
# This script can be run at regular intervals from cron,
# or it can run continuously in "watchdog" mode (e.g. as
# a systemd service) using the --watchdog option.  If you
# run it in "watchdog" mmode, be sure you disable the
# --detect-offline option in the Controller servicing this
# script because it will only send events as changes
# occur on the file system, which is likely to be
# irregular intervals.
#
# No AI was used in the creation of this script.
#-------------------------------------------------------

import os
import sys
import json
import signal
import argparse
import subprocess

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from queue import Queue

# Typing support
from typing import Iterator, Tuple
from collections.abc import Mapping

g_watchdog_queue: Queue | None = None
try:
    from watchdog.observers import Observer                             # type: ignore
    from watchdog.events import FileSystemEvent, FileSystemEventHandler # type: ignore
    g_watchdog_queue = Queue()  # communication queue for signal handler to interrupt watchdog function
except ModuleNotFoundError:
    g_watchdog_queue = None

class SensorStates:
    NAMES=["healthy", "poor", "critical", "deceased", "offline"]
    HEALTHY, POOR, CRITICAL, DECEASED, OFFLINE = range(5)

def signal_handler(signum, frame) -> None:
    global g_watchdog_queue

    if signum == signal.SIGTERM or signum == signal.SIGINT:
        if g_watchdog_queue:
            g_watchdog_queue.put('exit')

def construct_report(device: str, state: int, msg: str = "") -> Tuple[str, Mapping[str, str]]:
    """ Construct the Sensor event report file """
    sensor_name = device
    sensor_filename = f"{device.replace('/', '_')}.json"
    if sensor_filename.startswith('_'):
        sensor_filename = sensor_filename[1:]
    assert len(sensor_filename) > 0, "ERROR: Sensor file name is empty!"

    return (sensor_filename, { "sensor_name" : sensor_name,
            "sensor_state" : SensorStates.NAMES[state],
            "sensor_message" :  msg })

def calc_free_space(part: str) -> float:
    """ Calculate the available free space for the indicated partition """
    result = os.statvfs(part)
    total = result.f_frsize * result.f_blocks
    free = result.f_frsize * result.f_bavail
    return free / total

def locate_partitions() -> Iterator[str]:
    """ Locate all partitions on the system that contain the 'dashd' value in their labels """
    output = subprocess.Popen(['lsblk', '--output', 'mountpoint,label,partlabel'], stdout=subprocess.PIPE).communicate()[0].decode('utf-8')
    for line in output.split('\n'):
        line = line.strip()
        items = line.split()
        if len(items) > 1:
            # we have one or more labels
            hit = [val for val in items[1:] if 'dashd' in val.lower()]
            if len(hit):
                yield items[0]

def process_partition(part: str) -> Tuple[str, Mapping[str, str]]:
    """ Calculate the free space on the indicated partition """
    free = calc_free_space(part)
    state = SensorStates.HEALTHY
    if free == 0.0:
        state = SensorStates.DECEASED
    elif free <= 0.1:
        state = SensorStates.CRITICAL
    elif free <= 0.2:
        state = SensorStates.POOR

    msg = f"{part}: {int(free * 100.0)}% free space remaining."
    return construct_report(part, state, msg)

def watchdog(options: argparse.Namespace) -> int:
    """ Run continuously while monitoring tagged file systems for changes """
    global g_watchdog_queue

    # If we reach this code path, the watchdog module has been imported

    class MyEventHandler(FileSystemEventHandler):               # type: ignore
        """ Simple event handler for file system notifications """
        def __init__(self, options: argparse.Namespace, part: str) -> None:
            super(MyEventHandler, self).__init__()
            self.options = options
            self.part = part

            # Emit an initial event report; no telling when we'll be doing the next.
            report_data = process_partition(part)
            self.report_file = os.path.join(self.options.sensor_data, report_data[0])
            self.send_report(report_data)

        def cleanup(self) -> None:
            if os.path.exists(self.report_file):
                os.remove(self.report_file)

        def send_report(self, report_data: Tuple[str, Mapping[str, str]]) -> None:
            """ Send the provided event report data to the correct receiver """
            if self.options.test:
                print(f"{report_data[0]}: {json.dumps(report_data[1])}")
            else:
                with open(os.path.join(self.options.sensor_data, report_data[0]), 'w') as f:
                    f.write(json.dumps(report_data[1]))

        def on_any_event(self, event: FileSystemEvent) -> None: # type: ignore
            """ Callback to react to an event on the monitored file system """
            if event.is_directory:                              # type: ignore
                report_data = process_partition(self.part)
                self.send_report(report_data)

    parts: list[str] = []
    for part in locate_partitions():
        parts.append(part)

    if len(parts):
        observers: list[Observer] = []                          # type: ignore
        event_handlers: list[MyEventHandler] = []
        for part in parts:
            event_handlers.append(MyEventHandler(options, part))
            observers.append(Observer())                        # type: ignore
            observers[-1].schedule(event_handlers[-1], part, recursive=True)
            observers[-1].start()

        signal.signal(signal.SIGTERM, signal_handler)
        signal.signal(signal.SIGINT, signal_handler)

        # Pass our cycles back to the system until a signal terminates us
        assert g_watchdog_queue is not None, "ERROR: Failed to initialize g_watchdog_queue!"
        g_watchdog_queue.get()

        for observer in observers:
            observer.stop()
            observer.join()

        for handler in event_handlers:
            handler.cleanup()

    return 0

def oneshot(options: argparse.Namespace) -> int:
    """ Do a once-through of all tagged file systems and report free space """
    for part in locate_partitions():
        report_data = process_partition(part)

        if options.test:
            print(f"{report_data[0]}: {json.dumps(report_data[1])}")
        else:
            with open(os.path.join(options.sensor_data, report_data[0]), 'w') as f:
                f.write(json.dumps(report_data[1]))

    return 0

if __name__ == "__main__":
    parser = ArgumentParser(description="A Dash'd sensor to monitor free space", formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument('--sensor-data', type=str, metavar='DIR', default=None, help="Directory for Dash'd sensor data.")
    parser.add_argument('--watchdog', action="store_true", default=False, help="Run continuously and monitor file systems for changes.")
    parser.add_argument('--test', action="store_true", default=False, help="Print Sensor data to console and exit.")
    options, args = parser.parse_known_args()

    if options.watchdog and (g_watchdog_queue is None):
        print("ERROR: Watchdog mode cannot be used without the watchdog module. (Is it installed?)")
        sys.exit(1)

    if not options.test:
        if options.sensor_data is None:
            print("ERROR: A valid sensor path must be specified.")
            sys.exit(1)
        elif not os.path.exists(options.sensor_data):
            print("ERROR: The provided sensor path does not exist. (Is the Dash'd Collector running on this system?)")
            sys.exit(1)

    sys.exit(watchdog(options) if options.watchdog else oneshot(options))
