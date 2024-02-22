// Copyright by Alex Koessler


// Provides WiFi Service that connects the Dock to a Wifi network and handles reconnections if needed.


#include "wifi_service.h"
#include <mdns_service.h>
#include <log_service.h>
#include <time.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

const String logModule="wifi";

const unsigned long thirtySecs = 30 * 1000UL;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    logOutput(LOG_INFO, logModule, F("Wifi connected successfully."));
    WifiService::getInstance().updateMillis();
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
    //we got an ip, therfore we can check for the current time.
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    //print ip
    logOutput(LOG_INFO, logModule, "IP Address: " + WiFi.localIP().toString());
    //update mDNS
    MDNSService::getInstance().restartService();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    logOutput(LOG_WARNING, logModule, "Disconnected from WiFi access point. Reason: " + String(info.wifi_sta_disconnected.reason));
    WifiService::getInstance().updateMillis();
}

void WifiService::loop(){
    unsigned long currentMillis = millis();
    //check if wifi is active
    if(isActive()){
        //if wifi is down, reconnect after 30secs
        if((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= thirtySecs)){
            logOutput(LOG_INFO, logModule, F("Wifi reconnection attempt ..."));
            WiFi.disconnect();
            WiFi.reconnect();
            previousMillis = currentMillis;
            if(WiFi.status() != WL_CONNECTED){
                logOutput(LOG_DEBUG, logModule, F("Reconnection failed. Trying again in 30 seconds."));
            }else{
                logOutput(LOG_INFO, logModule, F("Reconnection successful."));
            }
        }
    }
}


void WifiService::connect(){

   Config &config = Config::getInstance();

    if (config.getWifiSsid() != "")
    {
        logOutput(LOG_DEBUG, logModule, F("SSID present in config."));
        
        WiFi.enableSTA(true);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);

        strcpy(wifihostname, config.getHostName().c_str());
        logOutput(LOG_DEBUG, logModule, "Setting Wifi Hostname: " + String(wifihostname));
        WiFi.setHostname(wifihostname);

        WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
        WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
        WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

        logOutput(LOG_INFO, logModule, F("Connecting to Wifi"));
        for(int i = 0; i < MAX_WIFI_CONNECTION_TRIES; ++i){
            String out = "Connection attempt " + String(i+1) + "/" + MAX_WIFI_CONNECTION_TRIES;
            logOutput(LOG_DEBUG, logModule, out);
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
            logOutput(LOG_ERROR, logModule, F("Starting WiFi Failed! Falling back to Bluetooth discovery."));
            // turn off wifi! nasty aborts will happen when accessing bluetooth if you don't.
            WiFi.mode(WIFI_OFF);
            delay(500);
        }
    }
    else
    {
        logOutput(LOG_INFO, logModule, F("SSID not found in config. Booting without Wifi."));
    }
}

