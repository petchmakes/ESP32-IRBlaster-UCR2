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
#include <log_service.h>

#include "web_task.h"
#include "blaster_config.h"

const String logModule="web";



#define SOCKET_DATA_SIZE 4096

char *socketData;
int currSocketBufferIndex;

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
        logOutput(LOG_INFO, logModule, "WebSocket client #" + String(client->id()) + " connected from " + client->remoteIP().toString());
        client->keepAlivePeriod(1);
        api_buildConnectionResponse(input, output);
        break;
    case WS_EVT_DISCONNECT:
        logOutput(LOG_INFO, logModule, "WebSocket client #" + String(client->id()) + " disconnected");
        break;
    case WS_EVT_DATA:
    {
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        if(info->final && info->index == 0 && info->len == len)
        {
            // the whole message is in a single frame and we got all of it's data
            Serial.printf("Raw json Message: %.*s\n", len, data);
            deserializeJson(input, data, len);
        }
        else
        {
            // message is comprised of multiple frames or the frame is split into multiple packets
			if (socketData == NULL) {
				// allocate memory for buffer on first call
				socketData  = (char *) malloc (SOCKET_DATA_SIZE);
			}
            for (size_t i = 0; i < len; i++)
            {
				// stop if message is bigger than buffer
				if (currSocketBufferIndex >= SOCKET_DATA_SIZE - 1) {
					Serial.printf("Raw json Message too big. Not processing.\n");
					break;
				}
                // copy data of each chunk into buffer
                socketData[currSocketBufferIndex] = data[i];
                currSocketBufferIndex++;
            }
            if(info->final && currSocketBufferIndex >= info->len)
            {
				Serial.printf("Raw json Message: %.*s\n", currSocketBufferIndex, socketData);
                // deserialize data after last chunk
                socketData[currSocketBufferIndex] = '\0';
                deserializeJson(input, socketData, currSocketBufferIndex);
                currSocketBufferIndex = 0;
            }
			else
			{
				break;
			}
        }
        if (!input.isNull())
        {
            api_processData(input, output);
        }
        else
        {
            Serial.print("WebSocket no JSON document sent\n");
        }
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
    logOutput(LOG_INFO, logModule, "TaskWeb running on core " + String(xPortGetCoreID()));

    AsyncWebServer server(Config::getInstance().API_port);
    AsyncWebSocket ws("/");

    // start websocket server.
    ws.onEvent(onWSEvent);
    server.addHandler(&ws);
    server.onNotFound(notFound);

    server.begin();

    MDNSService::getInstance().startService();

    for (;;)
    {
        MDNSService::getInstance().loop();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
