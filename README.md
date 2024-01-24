# ESP32 IR blaster for the Unfolded Circle Remote Two.

This project implements the [Unfolded Circle Remote Two](https://www.unfoldedcircle.com/) dock API to add additional IR blasters
to your remote setup.

This allows the installation of an IR blaster in an independant location from where your
remote and charging dock are located. For example, in an equipment rack or cupboard or for
multi-room setups.

## Requirements

1. An [Unfolded Circle Remote Two](https://www.unfoldedcircle.com/).
1. An ESP32 dev board
1. Platform.io to build and upload firmware
1. Some basic soldering / breadboarding skills and basic understanding of electronic.
1. Some basic electronic parts and IR led.

## Getting Started

1. Copy ``example-secrets.h`` to ``secrets.h`` and add the required secrets (e.g. wifi SSID and password).
1. Add your specific board to ``platformio.ini``. Feel free to contribute this back as a pull-request. 
1. Modify pin mappings, if required in ``blaster_config.h``.

## Electronics

At least one IR led + transistors and resistors is required for this to operate.

Schematic:

![Serving suggestion](https://github.com/petchmakes/ESP32-IRBlaster-UCR2/blob/main/kicad/schematic.png?raw=true)

# Integrating with the remote

In the remote web interface:

1. Select *Integrations & docks*
1. Under *Docks* click *+*
1. Select *Manual setup*
1. Specify *Name* and *ip address or hostname*
1. Click *Next*
1. The blaster should be added as a new dock. If there were issues, then download the recent log files in
*Settings* / *Development* / *Logs*. There should be a reason for any failures logged by the remote.

## Caveats

The Unfolded Circle Remote Two API, while [documented](https://github.com/unfoldedcircle/core-api/blob/main/dock-api/README.md),
the API could change with a remote update before this library is updated. 

This project is not from Unfolded Circle, so do not ask for support there.

This is just a minimal implementation, and doesn't not support OTA updates, web config, etc at this stage.

The Unfolded Circle web interface will report an error when attempting to check for firmware updates. This is normal as
the remote does not know about this particular dock.
