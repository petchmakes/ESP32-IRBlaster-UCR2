// Copyright 2024 Craig Petchell

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <string.h>
#include <IRsend.h>

#include <blaster_config.h>
#include <ir_message.h>
#include <ir_queue.h>
#include <ir_task.h>
#include <secrets.h>
#include <web_task.h>

QueueHandle_t irQueueHandle;

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Create the queue which will have <QueueElementSize> number of elements, each of size `message_t` and pass the address to <QueueHandle>.
    irQueueHandle = xQueueCreate(IR_QUEUE_SIZE, sizeof(ir_message_t));

    // Check if the queue was successfully created
    if (irQueueHandle == NULL)
    {
        Serial.println("Queue could not be created. Halt.");
        while (1)
            delay(1000); // Halt at this point as is not possible to continue
    }

    // Set up two tasks to run independently.
    const BaseType_t  webTaskHandle = xTaskCreatePinnedToCore(
        TaskWeb, "Task Web/Websocket server",
        32768, NULL, 2, NULL, 0
    );

    const BaseType_t irTaskHandle = xTaskCreatePinnedToCore(
        TaskSendIR, "Task IR send task",
        32768, NULL, 3 /* highest priority */, NULL, 1
    );
}

void loop()
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}