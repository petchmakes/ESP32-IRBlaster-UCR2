#ifndef LIBCONFIG_H
#define LIBCONFIG_H

/* 
file based on yio repo
https://github.com/YIO-Remote/dock-software/blob/master/lib/config/config.cpp

 */

#include <Preferences.h>
#include <nvs.h>
#include <nvs_flash.h>

class Config
{
public:
    explicit Config();
    virtual ~Config(){}

    // getter and setter for brightness value
    // int between 5 and 255
    int         getLedBrightness();
    void        setLedBrightness(int value);

    // getter and setter for dock friendly name
    String      getFriendlyName();
    void        setFriendlyName(String value);

    // getter and setter for wifi credentials
    String      getWifiSsid();
    void        setWifiSsid(String value);

    String      getWifiPassword();
    void        setWifiPassword(String value);

    String      getToken();
    void        setToken(String value);

    // get hostname
    String      getHostName();

    // reset config to defaults
    void        reset();

    static Config*           getInstance()
    { return s_instance; }

    int             OTA_port = 80;
    int             API_port = 946;
    const String    token = "0";

private:
    Preferences     m_preferences;
    int             m_defaultLedBrightness = 50;
    String m_hostname;

    static Config*  s_instance;
};

#endif