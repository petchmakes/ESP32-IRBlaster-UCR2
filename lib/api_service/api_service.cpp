// Copyright by Alex Koessler

// Provides API decoding service for communication with dock via bluetooth and wifi.

#include <api_service.h>
#include <ir_service.h>
#include <mdns_service.h>
#include <libconfig.h>


void api_fillTypeIDCommand(JsonDocument &input, JsonDocument &output){
    output["type"] = input["type"];
    if(input.containsKey("id")){
        output["req_id"] = input["id"];
    }
    if(input.containsKey("command")){
        output["msg"] = input["command"];
    }

}

void api_fillDefaultResponseFields(JsonDocument &input, JsonDocument &output, int code, boolean reboot)
{
    api_fillTypeIDCommand(input, output);
    output["code"] = code;
    output["reboot"] = reboot;
}

void api_buildConnectionResponse(JsonDocument &input, JsonDocument &output)
{
    output["type"] = "auth_required";
    output["model"] = Config::getInstance()->getDeviceModel();
    output["revision"] = Config::getInstance()->getHWRevision();
    output["version"] = Config::getInstance()->getFWVersion();
}

void api_buildSysinfoResponse(JsonDocument &input, JsonDocument &output)
{
    output["name"] = Config::getInstance()->getFriendlyName();
    output["hostname"] = Config::getInstance()->getHostName();
    output["model"] = Config::getInstance()->getDeviceModel();
    output["revision"] = Config::getInstance()->getHWRevision();
    output["version"] = Config::getInstance()->getFWVersion();
    output["serial"] = Config::getInstance()->getSerial();
    output["ir_learning"] = Config::getInstance()->getIRLearning();
}

void processSetConfig(JsonDocument &input, JsonDocument &output)
{
    if (input.containsKey("token"))
    {
        // not sure about the effects of setting a token
        String token = input["token"].as<String>();
        if (strlen(token.c_str()) >= 4 && strlen(token.c_str()) <= 40)
        {
            Config::getInstance()->setToken(token);
        }
        else
        {
            // in case of token error we do not want to continue
            output["code"] = 400;
            return;
        }
    }

    if (input.containsKey("ssid") && input.containsKey("wifi_password"))
    {
        String ssid = input["ssid"].as<String>();
        String pass = input["wifi_password"].as<String>();

        Config::getInstance()->setWifiSsid(ssid);
        Config::getInstance()->setWifiPassword(pass);
        Serial.println("Saved new WIFI config. SSID:" + ssid + " PASS:" + pass);
        output["reboot"] = true;
    }

    if (input.containsKey("friendly_name"))
    {
        String friendlyname = input["friendly_name"].as<String>();
        if (friendlyname != Config::getInstance()->getFriendlyName())
        {
            // friendlyname has changed. update conf
            Serial.printf("Updating FriendlyName to %s\n", friendlyname.c_str());
            Config::getInstance()->setFriendlyName(friendlyname);
            // inform other subsystems about the change (e.g., mdns, ...)
            MDNSService::getInstance()->restartService();
        }
    }
}

void replyWithError(JsonDocument &request, JsonDocument &response, int errorCode, String errorMsg=""){
    api_fillTypeIDCommand(request, response);

    response["code"] = errorCode;
    if(errorMsg != ""){
        response["error"] = errorMsg;
    }
}

void processPingMessage(JsonDocument &request, JsonDocument &response){
    Serial.printf("Received Ping message type\n");

    response["type"] = request["type"];
    response["msg"] = "pong";
}



void processAuthMessage(JsonDocument &request, JsonDocument &response){
    Serial.printf("Received auth message type\n");

    response["type"] = request["type"];
    response["msg"] = "authentication";

    //check if the auth token matches our expected one
    String token = request["token"].as<String>();
    if(token == Config::getInstance()->getToken()){
        //auth successful
        Serial.printf("Authentification successful\n");
        response["code"] = 200;
    } else {
        //auth failed - problem when trying to bind a previously configured dock with non-standard password!
        Serial.printf("Authentification failed\n");
        //response["code"] = 401;
        response["code"] = 200;
    }
}

//TODO: find a nicer solution later than crossreferencing a flag
extern boolean identifying;

void processDockMessage(JsonDocument &request, JsonDocument &response){
    String command;

    if (request.containsKey("msg")){
        command = request["msg"].as<String>();
        if (command == "ping"){
            //we got a ping message
            processPingMessage(request, response);
            return;
        }
    }

    if (!request.containsKey("command")){
        replyWithError(request, response, 400, "Missing command field");
        return;
    }
    command = request["command"].as<String>();

    if (command == "get_sysinfo"){
        api_fillDefaultResponseFields(request, response);
        api_buildSysinfoResponse(request, response);
    }
    else if (command == "identify")
    {
        // blink some leds
        api_fillDefaultResponseFields(request, response);
        identifying = true;
    }
    else if (command == "set_config")
    {
        api_fillDefaultResponseFields(request, response);
        processSetConfig(request, response);
    }
    else if (command == "ir_send")
    {
        api_fillDefaultResponseFields(request, response);
        queueIR(request, response);

    }
    else if (command == "ir_stop")
    {
        api_fillDefaultResponseFields(request, response);
        stopIR(request, response);

    }
    else if (command == "ir_receive_on")
    {
        // DO NOTHING BUT REPLY (for now)
        api_fillDefaultResponseFields(request, response);
    }
    else if (command == "ir_receive_off")
    {
        // DO NOTHING BUT REPLY (for now)
        api_fillDefaultResponseFields(request, response);
    }
    else if (command == "remote_charged")
    {
        // DO NOTHING BUT REPLY (for now)
        api_fillDefaultResponseFields(request, response);
    }
    else if (command == "remote_lowbattery")
    {
        // DO NOTHING BUT REPLY (for now)
        api_fillDefaultResponseFields(request, response);
    }
    else if (command == "remote_normal")
    {
        // DO NOTHING BUT REPLY (for now)
        api_fillDefaultResponseFields(request, response);
    }
    else if (command == "set_brightness")
    {
        // DO NOTHING BUT REPLY (for now)
        api_fillDefaultResponseFields(request, response);
    }
    else if (command == "set_logging")
    {
        // DO NOTHING BUT REPLY (for now)
        api_fillDefaultResponseFields(request, response);
    }
    else if (command == "reboot")
    {
        api_fillDefaultResponseFields(request, response, 200, true);
        //reboot is done after sending response
    }
    else if (command == "reset")
    {
        api_fillDefaultResponseFields(request, response, 200, true);
        Config::getInstance()->reset();
        //reboot is done after sending response
    }
    else
    {
        Serial.printf("Unsupported command %s\n", command);
        replyWithError(request, response, 400, "Unsupported command");
    }
}

void api_processData(JsonDocument &request, JsonDocument &response){

    String type;

    if (request.containsKey("type"))
    {
        type = request["type"].as<String>();
    }

    if (type == "dock")
    {
        processDockMessage(request, response);
    } 
    else if (type == "auth")
    {
        processAuthMessage(request, response);
    }
    else
    {
        Serial.printf("Unknown type %s\n", type);
        api_fillDefaultResponseFields(request, response, 400);
    }
}

