// Copyright 2024 Craig Petchell

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <string.h>

#include <secrets.h>

AsyncWebServer server(946);
AsyncWebSocket ws("/");

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void onConnection(DynamicJsonDocument& input, DynamicJsonDocument& output) {
    output["type"] = "auth_required";
    output["model"] = "UCD2";
    output["revision"] = "5.4";
    output["version"] = "0.6.0";
}

void setDefaultResponseFields(DynamicJsonDocument& input, DynamicJsonDocument& output, int code = 200, boolean reboot = false) {
    output["type"] = input["type"];
    output["req_id"] = input["id"];
    output["msg"] = input["command"];
    output["code"] = code;
    output["reboot"] = false;
}

void onDockMessage(DynamicJsonDocument& input, DynamicJsonDocument& output) {
    const char* type = input["type"];
    // Order most common -> least common
    if(!strcmp("dock", type)) {
        const char* command = input["command"];
        if (!strcmp("ir_send", command)) {
            setDefaultResponseFields(input, output);
            // TODO send IR

        } else if (!strcmp("ir_stop", command)) {
            setDefaultResponseFields(input, output);
            // TODO stop any active IR
        } else if (!strcmp("get_sysinfo", command)) {
            setDefaultResponseFields(input, output);
            output["name"] = "ESP32-S2";
            output["hostname"] = "blah.local";
            output["model"] = "ESP32-S2";
            output["revision"] = "5.4";
            output["version"] = "0.4.0";
            output["serial"] = "666";
            output["ir_learning"] = false; 
        } else if (!strcmp("set_config", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("identify", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("ir_receive_on", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("ir_receive_off", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("remote_charged", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("remote_lowbattery", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("remote_normal", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("reboot", command)) {
            setDefaultResponseFields(input, output, 200, true);
            ESP.restart();
        } else if (!strcmp("reset", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("set_brightness", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else if (!strcmp("set_logging", command)) {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        } else {
            Serial.printf("Unknown command %s\n", command);
            setDefaultResponseFields(input, output, 400);
        }
    } else if(!strcmp("auth", type)) {
        Serial.printf("Received auth message type\n");
        output["type"] = input["type"];
        output["msg"] = "authentication";
        output["code"] = 200;    
    } else {
        Serial.printf("Unknown type %s\n", type);
        setDefaultResponseFields(input, output, 400);
    }
}

void onWSEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) 
{
    DynamicJsonDocument input(JSON_OBJECT_SIZE(10));
    DynamicJsonDocument output(JSON_OBJECT_SIZE(10));
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            onConnection(input, output);
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            Serial.printf("Raw json Message %.*s\n", len, data);
            deserializeJson(input, data, len);
            if(!input.isNull()) {
                onDockMessage(input, output);
            } else {
                Serial.print("WebSocket no JSON document sent\n");
            }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }    
    if(!output.isNull()) {
        // send document back
        size_t len = measureJson(output);
        AsyncWebSocketMessageBuffer* buf = ws.makeBuffer(len);
        serializeJson(output, buf->get(), len);
        Serial.printf("Raw json response %.*s\n", len, buf->get());
        client->text(buf);
    }
}


void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    ws.onEvent(onWSEvent);
    server.addHandler(&ws);

    server.begin();
}

void loop() {
}