// Copyright 2024 Craig Petchell

#include <freertos/FreeRTOS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <string.h>

#include <ir_message.h>
#include <ir_queue.h>
#include <web_task.h>

// Current text ir code
char irCode[MAX_IR_TEXT_CODE_LENGTH] = "";
char irFormat[MAX_IR_FORMAT_TYPE] = "";

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void onConnection(JsonDocument& input, JsonDocument& output) {
    output["type"] = "auth_required";
    output["model"] = "UCD2";
    output["revision"] = "5.4";
    output["version"] = "0.6.0";
}

void setDefaultResponseFields(JsonDocument& input, JsonDocument& output, int code = 200, boolean reboot = false) {
    output["type"] = input["type"];
    output["req_id"] = input["id"];
    output["msg"] = input["command"];
    output["code"] = code;
    output["reboot"] = false;
}

void buildProntoMessage(ir_message_t &message) {
    const char* token;
    const char* code;
    const int codeLen = strlen(irCode);

    char workingCode[codeLen] = "";
    char *workingPtr;
    message.codeLen = (codeLen + 1) / 5;
    uint16_t offset = 0;
    
    strcpy(workingCode, irCode);
    char* hexCode = strtok_r(workingCode, ",", &workingPtr);
    while(hexCode) {
        message.code[offset++] = strtoul(hexCode, NULL, 16);
        hexCode = strtok_r(NULL, ",", &workingPtr);
    }

    message.format = pronto;
    message.action = send;
}

void buildHexMessage(ir_message_t &message) {
    const char* token;
    const char* code;
    const int codeLen = strlen(irCode);

    char workingCode[codeLen] = "";
    char *workingPtr;
    message.codeLen = (codeLen + 1) / 5;
    uint16_t offset = 0;
    
    strcpy(workingCode, irCode);
    char* hexCode = strtok_r(workingCode, ",", &workingPtr);
    while(hexCode) {
        message.code[offset++] = strtoul(hexCode, NULL, 16);
        hexCode = strtok_r(NULL, ",", &workingPtr);
    }

    message.format = hex;
    message.action = send;
}

void queueIRMessage(ir_message_t &message) {
    if(irQueueHandle != NULL) {
        int ret = xQueueSend(irQueueHandle, (void*) &message, 0);
        if(ret == pdTRUE){
          // The message was successfully sent.
          Serial.println("Action successfully send to the IR Queue");
        }else if(ret == errQUEUE_FULL){
          Serial.println("The `TaskWeb` was unable to send message to IR Queue");
        }
    } else {
        Serial.println("The `TaskWeb` was unable to send message to IR Queue; no queue defined");
    }
}

void queueIR(JsonDocument& input, JsonDocument& output) {
    const char *newCode = input["code"];
    const char *newFormat = input["format"];
    const uint16_t newRepeat = input["repeat"];
    const bool ir_internal = input["int_side"] || input["int_top"];
    const bool ir_ext1 = input["ext1"]; 
    const bool ir_ext2 = input["ext2"];
    ir_message_t message;

    message.action = send;
    message.ir_internal = ir_internal;
    message.ir_ext1 = ir_ext1;
    message.ir_ext2 = ir_ext2;
    message.repeat = newRepeat;

    if(uxQueueMessagesWaiting(irQueueHandle) != 0 && 
        irCode[0] && (strcmp(newCode, irCode) != 0) && (strcmp(newFormat, irFormat) != 0)) {
        // Still sending and different code - reject
        setDefaultResponseFields(input, output, 429);
        return;        
    }

    if(strlen(newCode) > sizeof(irCode)) {
        Serial.printf("Length of sent code is longer than allocated buffer. Length = %d; Max = %s\n", strlen(newCode), sizeof(irCode));
        setDefaultResponseFields(input, output, 400);
        return;
    }

    if(strlen(newFormat) > sizeof(irFormat)) {
        Serial.printf("Length of sent format is longer than allocated buffer. Length = %d; Max = %s\n", strlen(newFormat), sizeof(irFormat));
        setDefaultResponseFields(input, output, 400);
        return;
    }

    strcpy(irCode, newCode);
    strcpy(irFormat, newFormat);

    if(strcmp("hex", newFormat) == 0) {
        buildHexMessage(message);
        queueIRMessage(message);
        setDefaultResponseFields(input, output);
    } else if (strcmp("pronto", newFormat) == 0) {
        buildProntoMessage(message);
        queueIRMessage(message);
        setDefaultResponseFields(input, output);
    } else {
        Serial.printf("Unknown ir format %s\n", newFormat);
        setDefaultResponseFields(input, output, 400);
        irCode[0] = 0;
        irFormat[0] = 0;
    }
}

void stopIR(JsonDocument& input, JsonDocument& output) {
    ir_message_t message;
    message.action = stop;

    queueIRMessage(message);
    setDefaultResponseFields(input, output);
}

void onDockMessage(JsonDocument& input, JsonDocument& output) {
    const char* type = input["type"];
    // Order most common -> least common
    if(!strcmp("dock", type)) {
        const char* command = input["command"];
        if (!strcmp("ir_send", command)) {
            queueIR(input, output);
        } else if (!strcmp("ir_stop", command)) {
            stopIR(input, output);
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
    JsonDocument input;
    JsonDocument output;
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
        AsyncWebSocketMessageBuffer* buf = server->makeBuffer(len);
        serializeJson(output, buf->get(), len);
        Serial.printf("Raw json response %.*s\n", len, buf->get());
        client->text(buf);
    }
}

void TaskWeb(void *pvParameters) {
    Serial.printf("TaskWeb running on core %d\n", xPortGetCoreID());

    AsyncWebServer server(946);
    AsyncWebSocket ws("/");

    // server.
    ws.onEvent(onWSEvent);
    server.addHandler(&ws);

    server.begin();

    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
