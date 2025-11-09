<p align="center">
    <img width="20%" alt="Dash'd" src="https://private-user-images.githubusercontent.com/4536448/511854353-264f2f78-17bd-44b4-b493-6052f21cdb4a.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjI3MTQ1MTEsIm5iZiI6MTc2MjcxNDIxMSwicGF0aCI6Ii80NTM2NDQ4LzUxMTg1NDM1My0yNjRmMmY3OC0xN2JkLTQ0YjQtYjQ5My02MDUyZjIxY2RiNGEucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MTEwOSUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTExMDlUMTg1MDExWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9YmMwMWQ0OWFkYmZhZTljYjQ0OTJhMTgwN2Y5ZDI3Mzk3ZTdmYjc5OWE0MmM3MjNjNmRkOWQ5NzcxMjdhMDI2MCZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.AnX7GH8E-lkYHbSOj8TxARc4EkNoIsaKwcwW1NqIbfs">
</p>

# Dash'd
A simple, annunciator-based dashboard for desktop systems.

**No AI was used in the construction of this project.**

## Summary
Dahs'd is a system that gathers and displays simple status information for monitored assets and resources.

## Design
Dash'd is composed of three logical components: Sensors, Collectors, and Dashboards.  The Collectors and Dashboards communicate using IPv4/IPv6 multicasting.

### Network
The Dash'd system utilizes multicasting, which creates a network "ring" where data is published without the need for active receivers.  Any number of senders can be connected to the multicast group, and any number of receivers can be listening on the "ring" entirely without knowledge of the presence of any other connection (classic "Observer" pattern).  Receivers in this case are Dashboards listening for Sensor data sent by Collectors.  There is no limit to the number senders and receivers that can exists in the multicast group.

Multicasting in Dash'd is limited to the current subnet.  This means that domains to be monitored must have network "line-of-sight" to Dashboards prepared to display the state of their assets.  This is currently a hard-coded limitation within Dash'd, but it can be altered to allow the multicast group to be global (i.e., be visible on the Internet).

### Sensor
A Sensor is actually not a direct component of the Dash'd project.  Rather, they are independent processes running on the domain that are actively monitoring one or more assets and/or resources.  Sensors are written by the user, and can be constructed in any language or fashion chosen by the user, as long as they can produce the expected output.

Each Sensor process will produce a report in the form of a JSON file, and then deposit that report into a pre-designated folder at reaonsable intervals.  The Collector process will monitor this folder for activity, and will ensure that that any Dashboard processes on the "ring" will receive the information for display.

#### Sensor data
A Sensor will generate a JSON file to be deposited within view of the Collector.  This Sensor data file can contain up to three elements that will be regarded by the Collector:

- sensor_name
  - This is a name that is unique within the domain.  It should be descriptive enough that somebody viewing it will know without doubt what asset or resource it represents.
  - E.g., a Sensor that is monitoring the health of a RAID might name itself `raid_monitor_md0`.
  - The Collector will provide the domain name to listeners on the "ring" along with the Sensor data, so the Dashboard will include it in the display.  E.g., for a domain named "corrin", the Dashboard would display the Sensor name as `corrin::raid_monitor_md0`.
- sensor_state
  - States are strings with simplified values:
    - `healthy`, `poor`, `critical` or `deceased`
  - Each of these has a visual representation in the Dashboard display.
- sensor_message (optional)
  - The Sensor message is an optional value that should be used to report the rationale for the `sensor_state` value.
    - E.g., A `poor` state might be generated if disk space falls below some designated threshold.  A message might report that as `File system /media/data has less than 20% free space`.

### Collector
The Collector is a CLI process intneded to run as a service/daemon on a domain with assets and resources to be monitored.  It monitors a folder on the local domain where Sensors have been directed to deposit their updates.

### Dashboard
The Dashboard is the visual display of the status of one or more Sensor reports.
