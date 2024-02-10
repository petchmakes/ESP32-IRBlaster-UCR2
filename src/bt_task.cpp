// Copyright 2024 Alex Koessler

#include <freertos/FreeRTOS.h>
#include <Arduino.h>

#include <bt_task.h>
#include <bt_service.h>

void TaskBT(void *pvParameters)
{
    Serial.printf("TaskBT running on core %d\n", xPortGetCoreID());

    BluetoothService* btService = new BluetoothService();

    //initialize Bluetooth
    btService->init();

    for (;;)
    {
        btService->handle();
        //TODO: blink the led in the discovery pattern!
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}