//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Dictionary.h>
#include <secrets.h>

AsyncWebServer server(946);
AsyncWebSocket ws("/");

enum MessageType {
    UNKNOWN_MESSAGE_TYPE = 0,
    AUTH,
    DOCK,
};

Dictionary* MessageTypeDict = new Dictionary(3);

void initMessageTypeDict() {
    MessageTypeDict->insert("auth", AUTH);
    MessageTypeDict->insert("dock", DOCK);
}

MessageType getMessageType(const char *messageType) {
    String value = MessageTypeDict->search(messageType);
    return (MessageType) value.toInt();
}

enum Command {
    UNKNOWN_COMMAND = 0,
    GET_SYSINFO,
    SET_CONFIG,
    IR_SEND,
    IR_STOP,
    IR_RECEIVE_ON,
    IR_RECEIVE_OFF,
    REMOTE_CHARGED,
    REMOTE_LOWBATTERY,
    REMOTE_NORMAL,
    IDENTIFY,
    REBOOT,
    RESET,
    SET_BRIGHTNESS,
    SET_LOGGING,
};

Dictionary* CommandsDict = new Dictionary(15);

void initCommandsDict() {
    CommandsDict->insert("get_sysinfo", GET_SYSINFO);
    CommandsDict->insert("set_config", SET_CONFIG);
    CommandsDict->insert("ir_send", IR_SEND);
    CommandsDict->insert("ir_stop", IR_STOP);
    CommandsDict->insert("ir_receive_on", IR_RECEIVE_ON);
    CommandsDict->insert("ir_receive_off", IR_RECEIVE_OFF);
    CommandsDict->insert("remote_charged", REMOTE_CHARGED);
    CommandsDict->insert("remote_lowbattery", REMOTE_LOWBATTERY);
    CommandsDict->insert("remote_normal", REMOTE_NORMAL);
    CommandsDict->insert("identify", IDENTIFY);
    CommandsDict->insert("reboot", REBOOT);
    CommandsDict->insert("reset", RESET);
    CommandsDict->insert("set_brightness", SET_BRIGHTNESS);
    CommandsDict->insert("set_logging", SET_LOGGING);
}

Command getCommand(const char *command) {
    String value = CommandsDict->search(command);
    return (Command) value.toInt();
}

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
    MessageType typeEnum = getMessageType(type);
    switch(typeEnum) {
        case AUTH:
        {
            Serial.printf("Received auth message type\n");
            output["type"] = input["type"];
            output["msg"] = "authentication";
            output["code"] = 200;
            break;
        }

        case DOCK:
        {
            const char* command = input["command"];
            Command commandEnum = getCommand(command);
            switch(commandEnum) {
                case GET_SYSINFO:
                {
                    setDefaultResponseFields(input, output);
                    output["name"] = "ESP32-S2";
                    output["hostname"] = "blah.local";
                    output["model"] = "ESP32-S2";
                    output["revision"] = "5.4";
                    output["version"] = "0.4.0";
                    output["serial"] = "666";
                    output["ir_learning"] = false; 
                    break;
                }
                case IR_SEND:
                {
                    setDefaultResponseFields(input, output);
                    // TODO send IR
                    break;
                }
                case IR_STOP:
                {
                    setDefaultResponseFields(input, output);
                    // TODO stop any active IR
                    break;
                }
                case REBOOT:
                {
                    setDefaultResponseFields(input, output, 200, true);
                    ESP.restart();
                    break;
                }
                case SET_CONFIG:
                case IR_RECEIVE_ON:
                case IR_RECEIVE_OFF:
                case REMOTE_CHARGED:
                case REMOTE_LOWBATTERY:
                case REMOTE_NORMAL:
                case IDENTIFY:
                case RESET:
                case SET_BRIGHTNESS:
                case SET_LOGGING:
                {
                    setDefaultResponseFields(input, output);
                    // DO NOTHING BUT REPLY (for now)
                    break;
                }

                default:
                {
                    Serial.printf("Unknown command %s\n", command);
                    break;
                }      
            }  
            break;
        }

        default:
        {
            Serial.printf("Unknown type %s\n", type);
            break;
        }        
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
    initMessageTypeDict();
    initCommandsDict();

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