// Copyright 2024 Craig Petchell
// Contributions by Alex Koessler

#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include <mdns_service.h>
#include <api_service.h>
#include <libconfig.h>

#include "web_task.h"
#include "blaster_config.h"


boolean identifying = false;

void notFound(AsyncWebServerRequest *request)
{
    Serial.printf("404 for: %s\n", request->url());
    request->send(404, "text/plain", "Not found");
}

void onWSEvent(AsyncWebSocket *server,
               AsyncWebSocketClient *client,
               AwsEventType type,
               void *arg,
               uint8_t *data,
               size_t len)
{
    JsonDocument input;
    JsonDocument output;
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        client->keepAlivePeriod(1);
        api_buildConnectionResponse(input, output);
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        Serial.printf("Raw json Message %.*s\n", len, data);
        deserializeJson(input, data, len);
        if (!input.isNull())
        {
            api_processData(input, output);
        }
        else
        {
            Serial.print("WebSocket no JSON document sent\n");
        }
    case WS_EVT_PONG:
        Serial.print("WebSocket Event PONG\n");
        break;
    case WS_EVT_ERROR:
        Serial.printf("WebSocket client #%u error #%u: %s\n", client->id(), *((uint16_t*)arg), (char*)data);
        break;
    }
    if (!output.isNull())
    {
        // send document back
        size_t len = measureJson(output);
        AsyncWebSocketMessageBuffer *buf = server->makeBuffer(len);
        serializeJson(output, buf->get(), len);
        Serial.printf("Raw json response %.*s\n", len, buf->get());
        client->text(buf);

        //check if we have to close the ws connection (failed auth)
        String responseMsg = output["msg"].as<String>();
        int responseCode = output["code"].as<int>();
        if((responseMsg == "authentication") && (responseCode == 401)){
            //client ->close();
        }
        
        //check if we have to reboot
        if(output["reboot"].as<boolean>()){
            Serial.println(F("Rebooting..."));
            delay(500);
            ESP.restart();
        }
    }
}



void TaskWeb(void *pvParameters)
{
    Serial.printf("TaskWeb running on core %d\n", xPortGetCoreID());

    AsyncWebServer server(Config::getInstance()->API_port);
    AsyncWebSocket ws("/");

    // server.
    ws.onEvent(onWSEvent);
    server.addHandler(&ws);
    server.onNotFound(notFound);

    server.begin();

    for (;;)
    {
        MDNSService::getInstance()->loop();
        if(identifying) {
            for (int i = 0; i < 10; i++) // 5 seconds (10*0.5)
            {
                digitalWrite(BLASTER_PIN_INDICATOR, 1);
                digitalWrite(BLASTER_PIN_IR_INTERNAL, 1);
                vTaskDelay(250 / portTICK_PERIOD_MS);
                digitalWrite(BLASTER_PIN_INDICATOR, 0);
                digitalWrite(BLASTER_PIN_IR_INTERNAL, 0);
                vTaskDelay(250 / portTICK_PERIOD_MS);
            }
            identifying = 0;
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        
    }
}
