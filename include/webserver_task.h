#ifndef WEBSERVER_TASK_H_
#define WEBSERVER_TASK_H_
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>
typedef struct {
    bool eth_connected = false;
    bool wifi_connected = false;
    JsonDocument config;
} global_context_t;
void TaskWebServer(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif