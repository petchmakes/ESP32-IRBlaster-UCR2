// Copyright 2024 Craig Petchell
// Contributions by Alex Koessler

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include <WiFi.h>
#include <IRsend.h>

#include <libconfig.h>
#include <mdns_service.h>
#include <wifi_service.h>

#include <ir_message.h>
#include <ir_queue.h>

#include "ir_task.h"
#include "web_task.h"
#include "bt_task.h"
#include "led_task.h"

QueueHandle_t irQueueHandle;

void setup()
{
    Serial.begin(115200);

    Config &config = Config::getInstance();
    WifiService &wifiSrv = WifiService::getInstance();

    wifiSrv.connect();

    // Task for controlling the LED is always required
    const BaseType_t ledTaskHandle = xTaskCreatePinnedToCore(
        TaskLed, "Task Led Control",
        8192, NULL, 2, NULL, 0);

    if (wifiSrv.isActive())
    {

        // Create the queue which will have <QueueElementSize> number of elements, each of size `message_t` and pass the address to <QueueHandle>.
        irQueueHandle = xQueueCreate(IR_QUEUE_SIZE, sizeof(ir_message_t));

        // Check if the queue was successfully created
        if (irQueueHandle == NULL)
        {
            Serial.println("Queue could not be created. Halt.");
            while (1)
                delay(1000); // Halt at this point as is not possible to continue
        }

        // tasks for controlling the ir output via wifi
        const BaseType_t webTaskHandle = xTaskCreatePinnedToCore(
            TaskWeb, "Task Web/Websocket server",
            16384, NULL, 2, NULL, 0);
        const BaseType_t irTaskHandle = xTaskCreatePinnedToCore(
            TaskSendIR, "Task IR send task",
            32768, NULL, 3 /* highest priority */, NULL, 1);
    }
    else
    {
        // fallback to bluetooth discovery
        const BaseType_t btTaskHandle = xTaskCreatePinnedToCore(
            TaskBT, "Task Bluetooth",
            32768, NULL, 2, NULL, 0);
    }
}

void loop()
{
    WifiService::getInstance().loop();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}