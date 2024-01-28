#ifndef GLOBAL_CONFIG_H_
#define GLOBAL_CONFIG_H_

#include <ETH.h>
#define HOSTNAME "UCDockCustom"
#define DEVICE_NAME "UC-ESP32-IRBlaster"
#define CONFIG_FILE_REMOTE "/config.json"

// Pin out for Ethernet clock
#ifndef ETH_CLOCK_GPOUT
#ifdef BOARD_OLIMEX_FLAG
#define ETH_CLOCK_GPOUT ETH_CLOCK_GPIO0_OUT
#else
#define ETH_CLOCK_GPOUT ETH_CLOCK_GPIO17_OUT
#endif
#endif

#endif