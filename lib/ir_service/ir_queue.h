// Copyright 2024 Craig Petchell

#ifndef IR_QUEUE_H_
#define IR_QUEUE_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// 1 for current IR code; 1 for stop; don't allow more than one IR code at a time.
#define IR_QUEUE_SIZE 2

#ifdef __cplusplus
extern "C" {
#endif

extern QueueHandle_t irQueueHandle;

#ifdef __cplusplus
}
#endif

#endif // 
