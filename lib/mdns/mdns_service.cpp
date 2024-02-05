
/*
based on file from 
https://github.com/YIO-Remote/dock-software/blob/master/lib/service_mdns/service_mdns.cpp

*/

#include "mdns_service.h"

MDNSService* MDNSService::s_instance = nullptr;

char wifihostname2[50] = { 0 };


MDNSService::MDNSService()
{
    s_instance = this;
}

void MDNSService::loop()
{
    const unsigned long fiveMinutes = 1 * 60 * 1000UL;
    static unsigned long lastSampleTime = 0 - fiveMinutes;

    unsigned long now = millis();
    if (now - lastSampleTime >= fiveMinutes)
    {
        lastSampleTime += fiveMinutes;
        MDNS.end();
        delay(100);
        strcpy(wifihostname2, m_config->getHostName().c_str());
        Serial.printf("mdnsHostname: %s\n", wifihostname2);

        if (!MDNS.begin(wifihostname2))
        {
            Serial.println(F("[MDNS] Error setting up MDNS responder!"));
            while (1)
            {
            delay(1000);
            }
        }
        Serial.print(F("[MDNS] mDNS started with Hostname: "));
        Serial.println(wifihostname2);


        // Add mDNS service
        //MDNS.addService("_uc-dock-ota", "_tcp", m_config->OTA_port);
        MDNS.addService("_uc-dock", "_tcp", m_config->API_port);
        MDNS.addServiceTxt("_uc-dock", "_tcp", "rev", "1.0");
        MDNS.addServiceTxt("_uc-dock", "_tcp", "model", "KoeBlaster");
        MDNS.addServiceTxt("_uc-dock", "_tcp", "ver", "0.9.0");
        MDNS.addServiceTxt("_uc-dock", "_tcp", "name", String(wifihostname2));
        //addFriendlyName(String(wifihostname2));
        //addFriendlyName(m_config->getFriendlyName());
        Serial.println(F("[MDNS] Services updated"));
        
    }
}

void MDNSService::addFriendlyName(String name)
{
    MDNS.addServiceTxt("_uc-dock", "_tcp", "name", name);
    
}
