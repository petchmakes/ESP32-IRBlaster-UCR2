# ESP32 IR blaster for the Unfolded Circle Remote Two.

This project implements the [Unfolded Circle Remote Two](https://www.unfoldedcircle.com/) dock API to add additional IR blasters
to your remote setup.

This allows the installation of an IR blaster in an independant location from where your
remote and charging dock are located. For example, in an equipment rack or cupboard or for
multi-room setups.

## Requirements

1. An [Unfolded Circle Remote Two](https://www.unfoldedcircle.com/).
1. An ESP32 dev board (initial development was with an ESP32-S2)
1. Platform.io to build and upload firmware
1. Some basic soldering / breadboarding skills and electronics basics.
1. Some basic electronic parts and IR led.

## Getting Started

1. Copy ``example-secrets.h`` to ``secrets.h`` and add the required secrets (e.g. wifi SSID and password).
1. Add your specific board to ``platformio.ini``. Feel free to contribute this back as a pull-request. 

## Electronics

At least one IR led + transistors and resistors is required for this to operate.

Instructions to follow.

## Caveats

The Unfolded Circle Remote Two API, while [documented](https://github.com/unfoldedcircle/core-api/blob/main/dock-api/README.md),
the API could change with a remote update before this library is updated. 

This project is not from Unfolded Circle, so do not ask for support there.

This is just a minimal implementation, and doesn't not support OTA updates, etc.
