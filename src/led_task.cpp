// Copyright 2024 Alex Koessler

#include <freertos/FreeRTOS.h>
#include <Arduino.h>

#include "led_task.h"
//#include <bt_service.h>



//normal: breath (white)

//remote_lowbattery: blink fast

//remote_charged: breath (white)

//charging --> on

//identify --> blink fast

//setup --> blink slow





void TaskLed(void *pvParameters)
{
    Serial.printf("TaskLed running on core %d\n", xPortGetCoreID());

    //BluetoothService* btService = new BluetoothService();

    //initialize Bluetooth
    //btService->init();

    for (;;)
    {
        //btService->handle();
        //TODO: blink the led in the discovery pattern!
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}




