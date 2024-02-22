// Copyright by Alex Koessler


// Provides WiFi Service that connects the Dock to a Wifi network and handles reconnections if needed.

#include "wifi_service.h"
#include <mdns_service.h>

const unsigned long thirtySecs = 30 * 1000UL;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("Wifi connected successfully.");
    WifiService::getInstance().updateMillis();
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString());
    MDNSService::getInstance().restartService();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    WifiService::getInstance().updateMillis();
}

void WifiService::loop(){
    unsigned long currentMillis = millis();
    //check if wifi is active
    if(isActive()){
        //if wifi is down, reconnect after 30secs
        if((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= thirtySecs)){
            Serial.println("Wifi reconnection attempt ...");
            WiFi.disconnect();
            WiFi.reconnect();
            previousMillis = currentMillis;
            if(WiFi.status() != WL_CONNECTED){
                Serial.println("Reconnection failed. Trying again in 30 seconds.");
            }else{
                Serial.println("Reconnection successful.");
            }
        }
    }
}


void WifiService::connect(){

   Config &config = Config::getInstance();

    if (config.getWifiSsid() != "")
    {
        Serial.println(F("SSID present in config."));
        WiFi.enableSTA(true);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        Serial.print("Setting Wifi Hostname: ");
        strcpy(wifihostname, config.getHostName().c_str());
        Serial.println(wifihostname);
        WiFi.setHostname(wifihostname);

        WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
        WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
        WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

        Serial.println(F("Connecting to Wifi"));
        for(int i = 0; i < MAX_WIFI_CONNECTION_TRIES; ++i){
            Serial.printf("Connection attempt %d/%d ...", i+1, MAX_WIFI_CONNECTION_TRIES);
            WiFi.begin(config.getWifiSsid().c_str(), config.getWifiPassword().c_str());
            // try to connect to wifi for 10 secs
            if (WiFi.waitForConnectResult(10000) == WL_CONNECTED)
            {
                //we are connected.
                break;
            }
        }
        if(!WiFi.isConnected())
        {
            Serial.printf("Starting WiFi Failed! Falling back to Bluetooth discovery.\n");
            // turn off wifi! nasty aborts will happen when accessing bluetooth if you don't.
            WiFi.mode(WIFI_OFF);
            delay(500);
        }
    }
    else
    {
        Serial.println(F("Booting without Wifi. SSID not found in config."));
    }
}

