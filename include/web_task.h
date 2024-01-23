// Copyright 2024 Craig Petchell

#ifndef WEB_TASK_H_
#define WEB_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>

#define MAX_IR_TEXT_CODE_LENGTH 2048
#define MAX_IR_FORMAT_TYPE 50

void TaskWeb(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif