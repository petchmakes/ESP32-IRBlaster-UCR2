// Copyright 2024 Craig Petchell
// Contributions by Alex Koessler

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include <WiFi.h>
#include <IRsend.h>

#include <libconfig.h>
#include <mdns_service.h>

#include <ir_message.h>
#include <ir_queue.h>

#include "ir_task.h"
#include "web_task.h"
#include "bt_task.h"
#include "led_task.h"


QueueHandle_t irQueueHandle;

Config *config;
MDNSService *myMdns;

char deviceSerialNo[20] = {0};
char wifihostname[50] = {0};

const unsigned long oneMinute = 1 * 60 * 1000UL;
unsigned long previousMillis = 0;

void setup()
{
    Serial.begin(115200);

    config = new Config();
    myMdns = new MDNSService();

    if (config->getWifiSsid() != "")
    {
        Serial.println(F("SSID present in config."));
        WiFi.enableSTA(true);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        Serial.print("Setting Wifi Hostname: ");
        strcpy(wifihostname, config->getHostName().c_str());
        Serial.println(wifihostname);
        WiFi.setHostname(wifihostname);

        Serial.println(F("Connecting to Wifi ..."));
        WiFi.begin(config->getWifiSsid().c_str(), config->getWifiPassword().c_str());
        // try to connect to wifi for 30 secs, otherwise fall back to bluetooth.
        if (WiFi.waitForConnectResult(30000) != WL_CONNECTED)
        {
            Serial.printf("Starting WiFi Failed! Falling back to Bluetooth discovery.\n");
            // turn off wifi! nasty aborts will happen when accessing bluetooth if you don't.
            WiFi.mode(WIFI_OFF);
            delay(500);
        }
        else
        {
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString());
            previousMillis = millis();
        }
    }
    else
    {
        Serial.println(F("Booting without Wifi. SSID not found in config."));
    }

    strcpy(deviceSerialNo, WiFi.macAddress().c_str());
    Serial.printf("MAC address: %s\n", deviceSerialNo);

    //Task for controlling the LED is always required
    const BaseType_t ledTaskHandle = xTaskCreatePinnedToCore(
        TaskLed, "Task Led Control",
        32768, NULL, 2, NULL, 0);



    if (WiFi.isConnected())
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
            32768, deviceSerialNo, 2, NULL, 0);
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
    unsigned long currentMillis = millis();
    //check if Wifi is active
    if(WiFi.getMode() == WIFI_STA){
        // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
        if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= oneMinute)) {
            //Serial.print(millis());
            Serial.println("Wifi connection lost. Reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();
            previousMillis = currentMillis;
            if(WiFi.status() != WL_CONNECTED){
                Serial.println("Reconnection failed. Trying again in 1 minute.");
            }else{
                Serial.println("Reconnection successful.");
            }
        }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}