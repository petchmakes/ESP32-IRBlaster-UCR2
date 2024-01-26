// Copyright 2024 Craig Petchell

#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <string.h>
#include <IRutils.h>

#include <blaster_config.h>
#include <ir_message.h>
#include <ir_queue.h>
#include <web_task.h>
#include <global_config.h>

const char *deviceModel = "PetchMakesBlaster";
const char *deviceRevision = "1.0";
const char *firmwareVersion = "0.9.2";

char deviceSerialNo[50];

boolean identifying = false;

// Current text ir code
char irCode[MAX_IR_TEXT_CODE_LENGTH] = "";
char irFormat[MAX_IR_FORMAT_TYPE] = "";

void notFound(AsyncWebServerRequest *request)
{
    Serial.printf("404 for: %s\n", request->url());
    request->send(404, "text/plain", "Not found");
}

void onConnection(JsonDocument &input, JsonDocument &output)
{
    output["type"] = "auth_required";
    output["model"] = deviceModel;
    output["revision"] = deviceRevision;
    output["version"] = firmwareVersion;
}

void setDefaultResponseFields(JsonDocument &input, JsonDocument &output, int code = 200, boolean reboot = false)
{
    output["type"] = input["type"];
    output["req_id"] = input["id"];
    output["msg"] = input["command"];
    output["code"] = code;
    output["reboot"] = false;
}

void buildProntoMessage(ir_message_t &message)
{
    const int strCodeLen = strlen(irCode);

    char workingCode[strCodeLen + 1] = "";
    char *workingPtr;
    message.codeLen = (strCodeLen + 1) / 5;
    uint16_t offset = 0;

    strcpy(workingCode, irCode);
    char *hexCode = strtok_r(workingCode, ",", &workingPtr);
    while (hexCode)
    {
        message.code16[offset++] = strtoul(hexCode, NULL, 16);
        hexCode = strtok_r(NULL, ",", &workingPtr);
    }

    message.format = pronto;
    message.action = send;
}

void buildHexMessage(ir_message_t &message)
{
    // Split the UC_CODE parameter into protocol / code / bits / repeats
    char workingCode[MAX_IR_TEXT_CODE_LENGTH];
    char *parts[4];
    int partcount = 0;

    strcpy(workingCode, irCode);
    parts[partcount++] = workingCode;

    char *ptr = workingCode;
    while (*ptr && partcount < 4)
    {
        if (*ptr == ';')
        {
            *ptr = 0;
            parts[partcount++] = ptr + 1;
        }
        ptr++;
    }

    if (partcount != 4)
    {
        Serial.print("Unvalid UC code");
        return;
    }

    message.decodeType = strToDecodeType(parts[0]);

    switch (message.decodeType)
    {
    // Unsupported types
    case decode_type_t::UNUSED:
    case decode_type_t::UNKNOWN:
    case decode_type_t::GLOBALCACHE:
    case decode_type_t::PRONTO:
    case decode_type_t::RAW:
        Serial.print("The protocol specified is not supported by this program.");
        return;
    default:
        break;
    }

    uint16_t nbits = static_cast<uint16_t>(std::stoul(parts[2]));
    if (nbits == 0 && (nbits <= kStateSizeMax * 8))
    {
        Serial.printf("No of bits %s is invalid\n", parts[2]);
        return;
    }

    uint16_t stateSize = nbits / 8;

    uint16_t repeats = static_cast<uint16_t>(std::stoul(parts[3]));
    if (repeats > 20)
    {
        Serial.printf("Repeat count is too large: %d. Maximum is 20.\n", repeats);
        return;
    }

    if (!hasACState(message.decodeType))
    {
        message.code64 = std::stoull(parts[1], nullptr, 16);
        message.codeLen = nbits;
    }
    else
    {

        String hexstr = String(parts[1]);
        uint64_t hexstrlength = hexstr.length();
        uint64_t strOffset = 0;
        if (hexstrlength > 1 && hexstr[0] == '0' &&
            (hexstr[1] == 'x' || hexstr[1] == 'X'))
        {
            // Skip 0x/0X
            strOffset = 2;
        }

        hexstrlength -= strOffset;

        memset(&message.code8, 0, stateSize);
        message.codeLen = stateSize;

        // Ptr to the least significant byte of the resulting state for this
        // protocol.
        uint8_t *statePtr = &message.code8[stateSize - 1];

        // Convert the string into a state array of the correct length.
        for (uint16_t i = 0; i < hexstrlength; i++)
        {
            // Grab the next least sigificant hexadecimal digit from the string.
            uint8_t c = tolower(hexstr[hexstrlength + strOffset - i - 1]);
            if (isxdigit(c))
            {
                if (isdigit(c))
                    c -= '0';
                else
                    c = c - 'a' + 10;
            }
            else
            {
                Serial.printf("Code %s contains non-hexidecimal characters.", parts[1]);
                return;
            }
            if (i % 2 == 1)
            { // Odd: Upper half of the byte.
                *statePtr += (c << 4);
                statePtr--; // Advance up to the next least significant byte of state.
            }
            else
            { // Even: Lower half of the byte.
                *statePtr = c;
            }
        }
    }

    message.format = hex;
    message.action = send;
}

void queueIRMessage(ir_message_t &message)
{
    if (irQueueHandle != NULL)
    {
        int ret = xQueueSend(irQueueHandle, (void *)&message, 0);
        if (ret == pdTRUE)
        {
            // The message was successfully sent.
            Serial.println("Action successfully send to the IR Queue");
        }
        else if (ret == errQUEUE_FULL)
        {
            Serial.println("The `TaskWeb` was unable to send message to IR Queue");
        }
    }
    else
    {
        Serial.println("The `TaskWeb` was unable to send message to IR Queue; no queue defined");
    }
}

void queueIR(JsonDocument &input, JsonDocument &output)
{
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

    if (uxQueueMessagesWaiting(irQueueHandle) != 0) 
    {
        // Message being processed
        if(irCode[0] && (strcmp(newCode, irCode) == 0) && (strcmp(newFormat, irFormat) == 0))
        {
            // Same message. send repeat command
            setDefaultResponseFields(input, output, 202);
            message.action = repeat;
            queueIRMessage(message);
            return;
        }
        else
        {
            // Different message - reject
            setDefaultResponseFields(input, output, 429);
            return;
        }
    }

    if (strlen(newCode) > sizeof(irCode))
    {
        Serial.printf("Length of sent code is longer than allocated buffer. Length = %d; Max = %s\n", strlen(newCode), sizeof(irCode));
        setDefaultResponseFields(input, output, 400);
        return;
    }

    if (strlen(newFormat) > sizeof(irFormat))
    {
        Serial.printf("Length of sent format is longer than allocated buffer. Length = %d; Max = %s\n", strlen(newFormat), sizeof(irFormat));
        setDefaultResponseFields(input, output, 400);
        return;
    }

    strcpy(irCode, newCode);
    strcpy(irFormat, newFormat);

    if (strcmp("hex", newFormat) == 0)
    {
        buildHexMessage(message);
        queueIRMessage(message);
        setDefaultResponseFields(input, output);
    }
    else if (strcmp("pronto", newFormat) == 0)
    {
        buildProntoMessage(message);
        queueIRMessage(message);
        setDefaultResponseFields(input, output);
    }
    else
    {
        Serial.printf("Unknown ir format %s\n", newFormat);
        setDefaultResponseFields(input, output, 400);
        irCode[0] = 0;
        irFormat[0] = 0;
    }
}

void stopIR(JsonDocument &input, JsonDocument &output)
{
    ir_message_t message;
    message.action = stop;

    queueIRMessage(message);
    setDefaultResponseFields(input, output);
}

void onDockMessage(JsonDocument &input, JsonDocument &output)
{
    const char *type = input["type"];
    // Order most common -> least common
    if (!strcmp("dock", type))
    {
        const char *command = input["command"];
        if (!strcmp("ir_send", command))
        {
            queueIR(input, output);
        }
        else if (!strcmp("ir_stop", command))
        {
            stopIR(input, output);
        }
        else if (!strcmp("get_sysinfo", command))
        {
            setDefaultResponseFields(input, output);
            output["name"] = DEVICE_NAME;
            output["hostname"] = HOSTNAME;
            output["model"] = deviceModel;
            output["revision"] = deviceRevision;
            output["version"] = firmwareVersion;
            output["serial"] = deviceSerialNo;
            output["ir_learning"] = false;
        }
        else if (!strcmp("set_config", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("identify", command))
        {
            identifying = true;
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("ir_receive_on", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("ir_receive_off", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("remote_charged", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("remote_lowbattery", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("remote_normal", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("reboot", command))
        {
            setDefaultResponseFields(input, output, 200, true);
            ESP.restart();
        }
        else if (!strcmp("reset", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("set_brightness", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else if (!strcmp("set_logging", command))
        {
            // DO NOTHING BUT REPLY (for now)
            setDefaultResponseFields(input, output);
        }
        else
        {
            Serial.printf("Unknown command %s\n", command);
            setDefaultResponseFields(input, output, 400);
        }
    }
    else if (!strcmp("auth", type))
    {
        Serial.printf("Received auth message type\n");
        output["type"] = input["type"];
        output["msg"] = "authentication";
        output["code"] = 200;
    }
    else
    {
        Serial.printf("Unknown type %s\n", type);
        setDefaultResponseFields(input, output, 400);
    }
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
        onConnection(input, output);
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        Serial.printf("Raw json Message %.*s\n", len, data);
        deserializeJson(input, data, len);
        if (!input.isNull())
        {
            onDockMessage(input, output);
        }
        else
        {
            Serial.print("WebSocket no JSON document sent\n");
        }
    case WS_EVT_PONG:
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
    }
}

// Websocket server to communicate with the remote two
void TaskWeb(void *pvParameters)
{
    Serial.printf("TaskWeb running on core %d\n", xPortGetCoreID());
    Serial.printf("Serial number %s\n", pvParameters);
    strcpy(deviceSerialNo, (const char *) pvParameters);

    AsyncWebServer server(946);
    AsyncWebSocket ws("/");

    // server.
    ws.onEvent(onWSEvent);
    server.addHandler(&ws);
    server.onNotFound(notFound);

    server.begin();

    for (;;)
    {
        if(identifying) {
            for (int i = 0; i < 20; i++) // 10 seconds (20*0.5)
            {
                digitalWrite(BLASTER_PIN_INDICATOR, 1);
                vTaskDelay(250 / portTICK_PERIOD_MS);
                digitalWrite(BLASTER_PIN_INDICATOR, 0);
                vTaskDelay(250 / portTICK_PERIOD_MS);
            }
            identifying = 0;
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
