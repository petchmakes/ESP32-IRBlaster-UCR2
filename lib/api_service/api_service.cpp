

#include "api_service.h"



/* this needs to go to the config service in future */
//const char *deviceModel2 = "KoeBlaster"; //original was UCD2
const char *deviceModel2 = "UCD2"; //original was UCD2
const char *deviceRevision2 = "1.0";
const char *firmwareVersion2 = "0.9.0";

//char deviceSerialNo[50];
const char *deviceSerialNo2 = "0042"; //actually this is generated in main from mac address

/*
void setDefaultResponseFields(JsonDocument &input, JsonDocument &output){
    setDefaultResponseFields(input, output, 200, false);
}

void setDefaultResponseFields(JsonDocument &input, JsonDocument &output, int code){
    setDefaultResponseFields(input, output, code, false);
}*/

void fillDefaultResponseFields(JsonDocument &input, JsonDocument &output, int code=200, boolean reboot=false)
{
    output["type"] = input["type"];
    output["req_id"] = input["id"];
    output["msg"] = input["command"];
    output["code"] = code;
    output["reboot"] = reboot;
}


void buildSysinfoResponse(JsonDocument &input, JsonDocument &output){
    output["name"] = "UC-ESP32-IRBlaster";
    output["hostname"] = "blah.local";
    output["model"] = deviceModel2;
    output["revision"] = deviceRevision2;
    output["version"] = firmwareVersion2;
    output["serial"] = deviceSerialNo2;
    output["ir_learning"] = false;
}

void processSetConfig(JsonDocument &input, JsonDocument &output){
    //bool reboot = false;
    if (input.containsKey("ssid") && input.containsKey("wifi_password"))
    {
        String ssid = input["ssid"].as<String>();
        String pass = input["wifi_password"].as<String>();

        Config::getInstance()->setWifiSsid(ssid);
        Config::getInstance()->setWifiPassword(pass);

        Serial.println("Saving new WIFI config. SSID:" + ssid + " PASS:" + pass);
        output["reboot"] = true;
    }
    if (input.containsKey("friendly_name")){
        String friendlyname = input["friendly_name"].as<String>();

        Config::getInstance()->setFriendlyName(friendlyname);
    }
    if (input.containsKey("token")){
        //ignore for now.
        //String token = input["token"].as<String>();
        //Config::getInstance()->setToken(token);
    }
    // if(reboot){
    //     //use a central state manager in future
    //     Serial.println(F("Rebooting..."));
    //     ESP.restart();
    // }
}


void processData(JsonDocument &request, int id, String source, void(*sendCallback)(JsonDocument)){
    String type;
    String command;
    //prepare response Json object
    JsonDocument response;


    if (request.containsKey("type")) {
        type = request["type"].as<String>();
    }

    if (request.containsKey("command")) {
        command = request["command"].as<String>();
    }

    if(type == "dock"){
      if (command == "get_sysinfo"){
        fillDefaultResponseFields(request, response);
        buildSysinfoResponse(request, response);
      } else if (command == "identify"){
        fillDefaultResponseFields(request, response);
        //blink some leds in future
      } else if (command == "set_config"){
        fillDefaultResponseFields(request, response);
        processSetConfig(request, response);
      } else {
        Serial.printf("Unknown command %s\n", command);
        fillDefaultResponseFields(request, response, 400);
      }
    } else {
        Serial.printf("Unknown type %s\n", type);
        fillDefaultResponseFields(request, response, 400);
    }

    if (!response.isNull()){
        // send document back
        if(source == "bluetooth"){
            sendCallback(response);
            //m_btserial->sendCallback(response);
        }
    }

}



