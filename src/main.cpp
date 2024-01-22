// Copyright 2024 Craig Petchell

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <string.h>
#include <IRsend.h>

#include <secrets.h>
#include <blaster_config.h>

AsyncWebServer server(946);
AsyncWebSocket ws("/");

IRsend irsend(true, 0);
char irCode[2048] = "";
char irFormat[50] = "";
int irRepeat = 0;
bool irToSend = false;

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

void queueIR(JsonDocument& input, JsonDocument& output) {
    const char *newCode = input["code"];
    const char *newFormat = input["format"];
    const int newRepeat = input["repeat"];
    const bool ir_internal = input["int_side"] || input["int_top"];
    const bool ir_ext1 = input["ext1"]; 
    const bool ir_ext2 = input["ext2"];
    // Serial.printf("compare code %d '%s' '%s'\n", strcmp(newCode, irCode), newCode, irCode);
    // Serial.printf("compare format %d '%s' '%s'\n", strcmp(newFormat, irFormat), newFormat, irFormat);

    if(irCode[0] && (strcmp(newCode, irCode) != 0) && (strcmp(newFormat, irFormat) != 0)) {
        // Still sending and different code - reject
        setDefaultResponseFields(input, output, 429);
        return;        
    }


    uint32_t ir_pin_mask = 0 
        | 1 << BLASTER_PIN_INDICATOR
        | ir_internal << BLASTER_PIN_IR_INTERNAL
        | ir_ext1 << BLASTER_PIN_IR_OUT_1
        | ir_ext2 << BLASTER_PIN_IR_OUT_2
    ;

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

    irsend.setPinMask(ir_pin_mask);
    strcpy(irCode, newCode);
    strcpy(irFormat, newFormat);
    irRepeat = newRepeat;
    irToSend = true;

    // Serial.printf("[queueIR] irFormat,irCode %s\n%s\n",irFormat,irCode);

    if(strcmp("hex", newFormat) && strcmp("pronto", newFormat)) {
        Serial.printf("Unknown ir format %s\n", newFormat);
        setDefaultResponseFields(input, output, 400);
        irCode[0] = 0;
        irFormat[0] = 0;
        irRepeat = 0;
        irToSend = false;
        return;
    }

    setDefaultResponseFields(input, output);
}

void stopIR(JsonDocument& input, JsonDocument& output) {
    // irCode[0] = 0;
    // irFormat[0] = 0;
    // irRepeat = 0;
    // irToSend = false;
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
        AsyncWebSocketMessageBuffer* buf = ws.makeBuffer(len);
        serializeJson(output, buf->get(), len);
        Serial.printf("Raw json response %.*s\n", len, buf->get());
        client->text(buf);
    }
}

bool repeatCallback() {
    if(irRepeat > 0) {
        irRepeat--;
        return true;
    }
    return false;
}


void setup() {
    // Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    pinMode(BLASTER_PIN_INDICATOR, OUTPUT);
    pinMode(BLASTER_PIN_IR_INTERNAL, OUTPUT);
    pinMode(BLASTER_PIN_IR_OUT_1, OUTPUT);
    pinMode(BLASTER_PIN_IR_OUT_2, OUTPUT);

    irsend.setRepeatCallback(repeatCallback);
    irsend.begin();

    ws.onEvent(onWSEvent);
    server.addHandler(&ws);

    server.begin();
}

void sendProntoCode() {
    const char* token;
    const char* code;
    const int codeLen = strlen(irCode);

    // Serial.printf("[sendProntoCode] irFormat,irCode %s\n%s\n",irFormat,irCode);

    char workingCode[codeLen] = "";
    char *workingPtr;
    uint16_t rawProntoLen = (codeLen + 1) / 5;
    uint16_t rawPronto[rawProntoLen] = {};
    uint16_t offset = 0;
    
    strcpy(workingCode, irCode);
    char* hexCode = strtok_r(workingCode, ",", &workingPtr);
    // Serial.printf("workingCode = %s\n", workingCode);
    while(hexCode) {
        // Serial.printf("workingPtr = %s\n", workingCode);
        // Serial.printf("hexCode = %s; %4x\n", hexCode, strtoul(hexCode, NULL, 16));
        rawPronto[offset++] = strtoul(hexCode, NULL, 16);
        hexCode = strtok_r(NULL, ",", &workingPtr);
    }

    // Serial.printf("Parsed pronto: ");
    // for(int i = 0; i < rawProntoLen; i++) {
    //     Serial.printf("%4x ", rawPronto[i]);
    // }
    // Serial.printf("\n");
    // Serial.printf("len = %d\n", rawProntoLen);

    irsend.sendPronto(rawPronto, rawProntoLen, irRepeat);
    irToSend = false;
    Serial.printf("Sent pronto %d repeats\n", irRepeat);
}

void loop() {
    // Do actual sending in main loop. See:
    // https://github.com/crankyoldgit/IRremoteESP8266/wiki/Frequently-Asked-Questions#im-using-the-async-webserver-library-and-some-ir-protocols-dont-seem-to-send-correctly-why
    if(irToSend) {
        if(!strcmp("hex", irFormat)) {
            Serial.printf("Sending UC hex\n");
            decode_type_t type = UNKNOWN;
            const uint8_t *state = {};
            const uint16_t nbytes = 0;
            // irsend.send(type, state, nbytes);
        } else if(!strcmp("pronto", irFormat)) {
            Serial.printf("Sending UC pronto\n");
            sendProntoCode();
        }
        irToSend = false;
    }
}