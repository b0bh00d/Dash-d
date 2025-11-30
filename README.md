<p align="center">
    <img width="20%" alt="Dash'd" src="https://private-user-images.githubusercontent.com/4536448/511854353-264f2f78-17bd-44b4-b493-6052f21cdb4a.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjM1MTg0NTIsIm5iZiI6MTc2MzUxODE1MiwicGF0aCI6Ii80NTM2NDQ4LzUxMTg1NDM1My0yNjRmMmY3OC0xN2JkLTQ0YjQtYjQ5My02MDUyZjIxY2RiNGEucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MTExOSUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTExMTlUMDIwOTEyWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9OTQyM2ZmMTMxNmUyMjM1MmEwMDhlN2EwOGFiZTVhMTRlOGFiZmMyMDM4NmQ3NjE5ODZiZGZjYTg2ZDVlNDZlNSZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.QHxq0knetGExKYIzrDZVFe8CGBCdVc8hIpU34hS-KMs">
</p>

# Dash'd
A simple, annunciator-based dashboard for desktop systems.

## Summary
Dahs'd is a system that gathers and displays simple status information for monitored assets and resources.

## Design
Dash'd is composed of three logical components: Sensors, Collectors, and Dashboards.  The Collectors and Dashboards communicate using IPv4/IPv6 multicasting.

_**No AI was used in the creation of this project.**_

### Network
The Dash'd system utilizes multicasting, which creates a network "ring" where data is published without the need for active receivers.  Any number of senders can be connected to the multicast group, and any number of receivers can be listening on the "ring" entirely without knowledge of the presence of any other connection (classic "Observer" pattern).  Receivers in this case are Dashboards listening for Sensor data sent by Collectors.  There is no limit to the number senders and receivers that can exists in the multicast group.

<p align="center">
  <a href="https://www.smartdraw.com/cad/technical-drawing-software.htm"><img alt="multicast ring" src="https://private-user-images.githubusercontent.com/4536448/516005220-e9a96204-e144-452d-aef1-b40a1178e05c.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjQ0NzM5NzQsIm5iZiI6MTc2NDQ3MzY3NCwicGF0aCI6Ii80NTM2NDQ4LzUxNjAwNTIyMC1lOWE5NjIwNC1lMTQ0LTQ1MmQtYWVmMS1iNDBhMTE3OGUwNWMucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MTEzMCUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTExMzBUMDMzNDM0WiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9NTNiMjgwN2E2ODJiZWU3OWQzNzQ0NjgxNjQ0NTE4YjA4ODY5OTZlMjFhZjM3M2Q2ZmM4MWY5MmQ4OGMzNTVmNSZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.BHVJEnhAYv3p_NMoKg8vEBu9Lk2fIDUmL1eTqFnVPXQ"></a>
</p>

Multicasting in Dash'd is limited to the current subnet.  This means that domains to be monitored must have network "line-of-sight" to Dashboards prepared to display the state of their assets.  This is currently a hard-coded limitation within Dash'd, but it can be altered to allow the multicast group to be global (i.e., be visible on the Internet).

It is important to emphasize that the network parameters used by Collectors--IP adddress and port--must _exactly_ match those used by the Dashboards.  In this fashion, the multicast "ring" is established and data sent is successfully received.  Out of the box, both Collectors and Dashboards are coded to use the same default settings, so they will use the same "ring" when started.  You can override these settings of course, but be sure you apply the same settings to all processes.

### Sensor
A Sensor is actually not a direct component of the Dash'd project.  Rather, they are independent processes running on the domain that are actively monitoring one or more assets and/or resources.  Sensors are written by the user, and can be constructed in any language or fashion chosen by the user, as long as they can produce the expected output.

Each Sensor process will produce a report in the form of a JSON file, and then deposit that report into a pre-designated folder at reaonsable intervals.  The Collector process will monitor this folder for activity, and will ensure that that any Dashboard processes on the network "ring" will receive the information for display.

#### Sensor data
A Sensor will generate a JSON file to be deposited within view of the Collector.  This file represents some kind of "event" related to the asset. This Sensor event file can contain up to three elements that will be regarded by the Collector:

- sensor_name (required)
  - This is a name that is unique within the domain.  It should be descriptive enough that somebody viewing it will know without doubt what asset or resource it represents.
  - E.g., a Sensor that is monitoring the health of a RAID might name itself `raid_monitor_md0`.
  - The Collector will provide the domain name to listeners on the "ring" along with the Sensor data, so the Dashboard will include it in the display.  E.g., for a domain named "corrin", the Dashboard would display the Sensor name as `corrin::raid_monitor_md0`.
- sensor_state (required)
  - States are strings with simplified values:
    - `healthy`, `poor`, `critical` or `deceased`
  - Each of these has a visual representation in the Dashboard display.
- sensor_message (optional)
  - The Sensor message is an optional value that should be used to report the rationale for the `sensor_state` value.
    - E.g., A `poor` state might be generated if disk space falls below some designated threshold.  A message might report that as `File system /media/data has less than 20% free space`.

This detail will be shown as a tooltip when you hover over a Sensor display:
<p align="center">
    <img alt="tooltip" src="https://private-user-images.githubusercontent.com/4536448/513999473-4bca7e15-bb1c-4e90-a16e-aea3d61909a2.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjQ0NzM5NzQsIm5iZiI6MTc2NDQ3MzY3NCwicGF0aCI6Ii80NTM2NDQ4LzUxMzk5OTQ3My00YmNhN2UxNS1iYjFjLTRlOTAtYTE2ZS1hZWEzZDYxOTA5YTIucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MTEzMCUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTExMzBUMDMzNDM0WiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9MmNkMjcxODQzMDE2NTIxYWNhYjgxMjZiMDY5OGY5YmM1MTNiNjBhN2VjNDE1ODk0NmQzYmE1ZGU5NTgzMzAxMyZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.g-CA0ZNk5F8gUKpwcf_lhIMmMjMW9RS3052TkqLsf5c">
</p>

Sample Sensors (written in Python) are included for Linux that will monitor RAID health and disk space.

### Collector
The Collector is a CLI process intended to run as a service/daemon on a domain with assets and resources to be monitored.  It monitors a folder on the local domain where Sensors have been directed to deposit their updates.

The Collector queue works like this:
 - Sensors (which are any process in any language that monitor a system resource) will create a "report" file in the queue folder for each resource they are monitoring.  The file name is unimportant to the Collector; the extension must be ".json" in order to be regarded.
 - This file-per-resource-per-domain is persistent for the runtime of a Collector.  The Sensor process will update the report file, at an interval of its choosing, and the Collector will monitor the timestamp of the file.  When the timestamp changes, the Collector will re-load the file contents and send it on to the multicast group.  The `sensor_name` attribute within the JSON file should not be changed within the same persistent file.  If a Sensor process must change the sensor name, it should first remove the existing sensor data file, and then create a new one with the updated name.
 - If an existing report file disappears (perhaps the Sensor process gracefully goes offline), the Collector will remove it from its database, and notify the multicast group that the resource is no longer being monitored.

 A sample systemd service file is included in the Collector source folder that contains instructions for installation and activation.

### Dashboard
The Dashboard is the visual display of the status of one or more Sensor reports.  (The following is a state indicator test, not a live capture...)

<p align="center">
  <img src="https://private-user-images.githubusercontent.com/4536448/514512701-34854edc-c27f-48da-bcd2-5cc0a5262a68.gif?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjQ0NzM5NzQsIm5iZiI6MTc2NDQ3MzY3NCwicGF0aCI6Ii80NTM2NDQ4LzUxNDUxMjcwMS0zNDg1NGVkYy1jMjdmLTQ4ZGEtYmNkMi01Y2MwYTUyNjJhNjguZ2lmP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MTEzMCUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTExMzBUMDMzNDM0WiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9M2EzN2JlZmFhODc5NTI5NzI4N2M4ZTViYTc1YjQ3N2U0NTQ2YzljODZlZjRhMDZjMDZmZDE3NDE2YmRmMDY0MyZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.LYOWCDtLsBOaHlCan8RFV47CC6pd88gkBxc04o6-mjo">
</p>

A Dashboard will not be visible until at least one Sensor has reported in.  If a state degrades (e.g., `healthy` -> `poor`), there is a visual indicator that the state has degraded in order to gain attention (as shown above).

Dashboards currently extend horizontally to the right, or vertically downward, selectable in the settings.  (There's code to extend leftward for horizontal and upward for vertical, but needs more work for window positioning.)

A Dashboard can be moved to any location you wish on the screen by left-click-and-dragging on an area of the window that does not have a Sensor displayed.  Via the settings, you can also lock the Dashboard on top of all other windows on the sceen.

Sensor displays will appear as soon as a report is received; they may also disappear if the Sensor goes offline.  A Sensor can go offline gracefully, or the Dashboard may detect that a report from a Sensor is overdue and summarily deem that Sensor offline.

When a Sensor goes offline (indicated by the "X" display above), the display for that Sensor will remain in the Dashboard for some delayed amount of time to make sure it is noticed.  Once that delay expires, the Sensor display is automatically removed from the Dashboard.

### Dependencies

The Collector is a CLI process, so requires just the Qt 5 Networking module (and any secondary dependencies it has) be installed on the domain where it will run:

`sudo apt-get install libqt5network5`

By contrast, the Dashboard is a desktop application, so will require the default Qt 5 install in order to function:

`sudo apt-get install qt5-default`
