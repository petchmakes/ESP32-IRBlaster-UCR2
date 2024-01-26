#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <webserver_task.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ETH.h>
#include "config_file.h"

const char* PARAM_WIFI_IP = "wifi_ip";
const char* PARAM_ETH_IP = "eth_ip";
const char* PARAM_WIFI_MAC = "wifi_mac";
const char* PARAM_ETH_MAC = "eth_mac";
const char* PARAM_CONNECTION_TYPE = "connection_type";
const char* PARAM_WIFI_SSID = "wifi_ssid";
const char* PARAM_WIFI_PASSWORD = "wifi_password";
global_context_t *pGlobalContext = NULL;

void notFoundW(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void TaskWebServer(void *pvParameters)
{
    pGlobalContext = (global_context_t*)pvParameters;
    Serial.printf("TaskWebserver running on core %d\n", xPortGetCoreID());
    if(!SPIFFS.begin())
    {
        Serial.println("SPIFFS error to access webserver files...");
        return;
    }
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file)
    {
        Serial.print("Webserver file found: ");
        Serial.println(file.name());
        file.close();
        file = root.openNextFile();
    }
    
    AsyncWebServer server(80);
    // server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/index.html", "text/html");
    });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/styles.css", "text/css");
    });
    server.on("/jquery.js", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/jquery.js", "text/javascript");
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/script.js", "text/javascript");
    });

    server.on("/uc_logo.svg", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/uc_logo.svg", "image/svg+xml");
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        if (pGlobalContext)
        {
            if (request->hasParam(PARAM_CONNECTION_TYPE)) {
                message = "";
                if (pGlobalContext->wifi_connected)
                {
                    message = "Wifi connected";
                }
                if (pGlobalContext->eth_connected)
                {
                    if (message.length() > 0) message += "\n<br>";
                    message += "Ethernet connected";
                }
            } else if (request->hasParam(PARAM_WIFI_IP)) {
                if (pGlobalContext->wifi_connected)
                {
                    message = WiFi.localIP().toString();
                }
                else
                {
                    message = "Not connected";
                }
            } else if (request->hasParam(PARAM_WIFI_MAC)) {
                if (pGlobalContext->wifi_connected)
                {
                    message = WiFi.macAddress();
                }
                else
                {
                    message = "Not connected";
                }
            } else if (request->hasParam(PARAM_ETH_IP)) {
                if (pGlobalContext->eth_connected)
                {
                    message = ETH.localIP().toString();
                }
                else
                {
                    message = "Not connected";
                }
            }
            else if (request->hasParam(PARAM_ETH_MAC)) {
                if (pGlobalContext->eth_connected)
                {
                    message = ETH.macAddress();
                }
                else
                {
                    message = "Not connected";
                }
            }
            else if (request->hasParam(PARAM_WIFI_SSID)) {
                const char *val = pGlobalContext->config[CONFIG_FILE_WIFI_SSID_ENTRY];
                message = String(val);
            } else if (request->hasParam(PARAM_WIFI_PASSWORD)) {
                const char *val = pGlobalContext->config[CONFIG_FILE_WIFI_PASSWORD_ENTRY];
                message = String(val);
            }
            else {
                message = "No message sent";
            }
            request->send(200, "text/plain", message);
        }        
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam(PARAM_WIFI_SSID, true)) {
             pGlobalContext->config[(const char*)CONFIG_FILE_WIFI_SSID_ENTRY] = request->getParam(PARAM_WIFI_SSID, true)->value().c_str();
        }
        if (request->hasParam(PARAM_WIFI_PASSWORD, true)) {
             pGlobalContext->config[CONFIG_FILE_WIFI_PASSWORD_ENTRY] = request->getParam(PARAM_WIFI_PASSWORD, true)->value().c_str();
        }
        if (saveConfig(pGlobalContext->config))
        {
            request->send(200, "text/plain", "Configuration file saved");
        }
        else
        {
            request->send(500, "text/plain", "Error while saving configuration file");
        }
    });

    server.onNotFound(notFoundW);

    server.begin();
    for (;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}