

#include "api_service.h"
#include <mdns_service.h>







void fillDefaultResponseFields(JsonDocument &input, JsonDocument &output, int code, boolean reboot){
    output["type"] = input["type"];
    output["req_id"] = input["id"];
    output["msg"] = input["command"];
    output["code"] = code;
    output["reboot"] = reboot;
}

void buildConnectionResponse(JsonDocument &input, JsonDocument &output){
    output["type"] = "auth_required";
    output["model"] = Config::getInstance()->getDeviceModel();
    output["revision"] = Config::getInstance()->getHWRevision();
    output["version"] = Config::getInstance()->getFWVersion();
}

void buildSysinfoResponse(JsonDocument &input, JsonDocument &output){
    output["name"] = Config::getInstance()->getFriendlyName();
    output["hostname"] = Config::getInstance()->getHostName();
    output["model"] = Config::getInstance()->getDeviceModel();
    output["revision"] = Config::getInstance()->getHWRevision();
    output["version"] = Config::getInstance()->getFWVersion();
    output["serial"] = Config::getInstance()->getSerial();
    output["ir_learning"] = Config::getInstance()->getIRLearning();
    
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
        if(friendlyname != Config::getInstance()->getFriendlyName() ){
            //friendlyname has changed. update conf
            Serial.printf("Updating FriendlyName to %s\n", friendlyname.c_str());
            Config::getInstance()->setFriendlyName(friendlyname);
            //inform other subsystems about the change (e.g., mdns, ...)
            MDNSService::getInstance()->restartService();
        }

    }
    if (input.containsKey("token")){
        //not sure about the effects of setting a token
        String token = input["token"].as<String>();
        if(strlen(token.c_str())>=4 && strlen(token.c_str())<=40){
            Config::getInstance()->setToken(token);
        } else {
            //TODO: what happens if token is wrong everything els is ok?

            output["code"] = 400;
        }
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



