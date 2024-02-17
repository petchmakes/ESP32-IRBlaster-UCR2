// Copyright by Alex Koessler

// Provides WiFi Service that connects the Dock to a Wifi network and handles reconnections if needed.

#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

//#include <Arduino.h>
#include <WiFi.h>

#include <libconfig.h>

#define MAX_WIFI_CONNECTION_TRIES 3

class WifiService
{
public:
    static WifiService &getInstance()
    {
        static WifiService instance;
        return instance;
    }

    void connect();
    void loop();
    boolean isActive() {return (WiFi.getMode() == WIFI_STA);}
    void updateMillis() {previousMillis = millis();}

private:
    explicit WifiService() {}
    virtual ~WifiService() {}
    char wifihostname[50] = {0};

    boolean reconnect = false;
    unsigned long previousMillis;
};

#endif