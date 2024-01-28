// Copyright 2024 Craig Petchell

#ifndef BLASTER_CONFIG_H_
#define BLASTER_CONFIG_H_

// Max pin # is 31 due to 32-bit mask

/* Define which Pins are associated. Put 0 if the PIN is not supported by your board
BLASTER_PIN_INDICATOR = led indicator to show IR activity, just for information, pin number or 0 if none
BLASTER_PIN_IR_INTERNAL = pin number for main IR Blaster
BLASTER_PIN_IR_OUT_1 = pin number or 0 if none for external IR Blaster 1
BLASTER_PIN_IR_OUT_2 = pin number or 0 if none for external IR Blaster 2
*/

#ifndef BLASTER_PIN_INDICATOR
#ifdef BOARD_OLIMEX_FLAG
#define BLASTER_PIN_INDICATOR 0
#else
#define BLASTER_PIN_INDICATOR 5
#endif
#endif

#ifndef BLASTER_PIN_IR_INTERNAL
#ifdef BOARD_OLIMEX_FLAG
// Pin association for Olimex ESP32 with IR module connected through UEXT connector
#define BLASTER_PIN_IR_INTERNAL GPIO_NUM_4
#else
#define BLASTER_PIN_IR_INTERNAL 12
#endif
#endif

#ifndef BLASTER_PIN_IR_OUT_1
#ifdef BOARD_OLIMEX_FLAG
#define BLASTER_PIN_IR_OUT_1 0
#else
#define BLASTER_PIN_IR_OUT_1 13
#endif
#endif

#ifndef BLASTER_PIN_IR_OUT_2
#ifdef BOARD_OLIMEX_FLAG
#define BLASTER_PIN_IR_OUT_2 0
#else
#define BLASTER_PIN_IR_OUT_2 14
#endif
#endif

#endif