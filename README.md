# ESP32 based IR blaster that implements the Unfolded Circle Remote Two dock API.

## Requirements

1. An ESP32 dev board (initial development was with an ESP32-S2)
2. Platform.io to build and upload firmware
3. Some basic soldering / breadboarding skills and electronics basics.
4. Some basic electronic parts and IR led.


## Getting Started

1. Copy ``example-secrets.h`` to ``secrets.h`` and add the required secrets (e.g. wifi SSID and password).
2. Add your specific board to ``platformio.ini``. Feel free to contribute this back as a pull-request. 

## Electronics

At least one IR led + transistors and resistors is required for this to operate.

Instructions to follow.