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

    String      getHostName();
    
    String      getSerial();

    const boolean getIRLearning() { return irlearning; }

    const String getHWRevision() { return hwrevision; }

    const String getFWVersion() { return fwversion; }

    const String getDeviceModel() { return devicemodel; }

    // reset config to defaults
    void        reset();

    static Config*           getInstance() { return s_instance; }

    //OTA Port currently not announced by original dock
    //int             OTA_port = 80;
    const int       API_port = 946;

private:
    Preferences  m_preferences;

    const int    m_defaultLedBrightness = 75;
    
    const String hwrevision = "0.1";
    const String fwversion = "0.1.0";
    const String devicemodel = "KoeBlaster";

    const boolean irlearning = false;

    static Config* s_instance;
};

#endif