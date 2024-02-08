// Copyright by Alex Koessler

// file based on archived yio repo
// https://github.com/YIO-Remote/dock-software/tree/master/lib/config

// Provides configuration service that persisting dock settings.

#include "libconfig.h"
#include <Arduino.h>
#include <WiFi.h>

Config *Config::s_instance = nullptr;

// initializing config
Config::Config()
{
    s_instance = this;

    // if no LED brightness setting, set default
    if (getLedBrightness() == 0)
    {
        Serial.println("Setting default brightness");
        setLedBrightness(m_defaultLedBrightness);
    }

    // if no friendly name is set, set mac address
    if (getFriendlyName() == "")
    {
        // get the default friendly name
        Serial.println("Setting default friendly name");
        setFriendlyName(getHostName());
    }
}

// getter and setter for brightness value
int Config::getLedBrightness()
{
    m_preferences.begin("general", false);
    int led_brightness = m_preferences.getInt("brightness", 0);
    m_preferences.end();

    return led_brightness;
}

void Config::setLedBrightness(int value)
{
    m_preferences.begin("general", false);
    m_preferences.putInt("brightness", value);
    m_preferences.end();
}

// getter and setter for dock friendly name
String Config::getFriendlyName()
{
    m_preferences.begin("general", false);
    String friendlyName = m_preferences.getString("friendly_name", "");
    m_preferences.end();

    return friendlyName;
}

void Config::setFriendlyName(String value)
{
    m_preferences.begin("general", false);
    m_preferences.putString("friendly_name", value);
    m_preferences.end();
}

// getter and setter for dock token name
String Config::getToken()
{
    m_preferences.begin("general", false);
    // default password is 0000
    String token = m_preferences.getString("token", "0000");
    m_preferences.end();

    return token;
}

void Config::setToken(String value)
{
    m_preferences.begin("general", false);
    m_preferences.putString("token", value);
    m_preferences.end();
}

// getter and setter for wifi credentials
String Config::getWifiSsid()
{
    m_preferences.begin("wifi", false);
    String ssid = m_preferences.getString("ssid", "");
    m_preferences.end();

    return ssid;
}

void Config::setWifiSsid(String value)
{
    m_preferences.begin("wifi", false);
    m_preferences.putString("ssid", value);
    m_preferences.end();
}

String Config::getWifiPassword()
{
    m_preferences.begin("wifi", false);
    String password = m_preferences.getString("password", "");
    m_preferences.end();

    return password;
}

void Config::setWifiPassword(String value)
{
    m_preferences.begin("wifi", false);
    m_preferences.putString("password", value);
    m_preferences.end();
}

// get hostname
String Config::getHostName()
{
    static char hostName[] = "UC-Dock-X-xxxxxxxxxxxx";
    uint8_t wifiMac[6];
    esp_read_mac(wifiMac, ESP_MAC_WIFI_STA);
    sprintf(hostName, "UC-Dock-X-%02X%02X%02X%02X%02X%02X", wifiMac[0], wifiMac[1], wifiMac[2], wifiMac[3], wifiMac[4], wifiMac[5]);
    return hostName;
}

// get serial
String Config::getSerial()
{
    static char serial[] = "xxxxxxxxxxxx";
    uint8_t wifiMac[6];
    esp_read_mac(wifiMac, ESP_MAC_WIFI_STA);
    sprintf(serial, "%02X%02X%02X%02X%02X%02X", wifiMac[0], wifiMac[1], wifiMac[2], wifiMac[3], wifiMac[4], wifiMac[5]);
    return serial;
}

// reset config to defaults
void Config::reset()
{
    Serial.println("Resetting stored configuration.");

    Serial.println("Resetting general config.");
    m_preferences.begin("general", false);
    m_preferences.clear();
    m_preferences.end();

    Serial.println("Resetting general config done.");

    delay(500);

    Serial.println("Resetting wifi settings.");
    m_preferences.begin("wifi", false);
    m_preferences.clear();
    m_preferences.end();

    Serial.println("Resetting wifi settings done.");

    delay(500);

    Serial.println("Erasing flash.");
    int err;
    err = nvs_flash_init();
    Serial.println("nvs_flash_init: " + err);
    err = nvs_flash_erase();
    Serial.println("nvs_flash_erase: " + err);

    delay(500);

    // Take care of ESP reset after sending response!
}
