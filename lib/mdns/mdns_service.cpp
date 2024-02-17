// Copyright by Alex Koessler

// file based on archived yio repo
// https://github.com/YIO-Remote/dock-software/tree/master/lib/service_mdns

// Provides mDNS Service that announces the dock to remotes in the same network.

#include "mdns_service.h"


void MDNSService::startService()
{
    // hostname is in the form: "UC-Dock-X-xx-xx-xx-xx-xx-xx"
    static String hostname = "";

    // getting hostname from config and storing it statically
    hostname = m_config.getHostName();

    Serial.println(F("Starting mDNS ..."));
    // TODO: Add max retries for MDNS starting
    if (!MDNS.begin(hostname))
    {
        Serial.println(F("Error setting up mDNS responder!"));
        // try again to start mDNS immediately in next loop iteration
        m_forceRestart = true;
    }
    else
    {

        // Adding required mDNS services
        MDNS.addService("_uc-dock", "_tcp", m_config.API_port);
        MDNS.addServiceTxt("_uc-dock", "_tcp", "rev", m_config.getHWRevision());
        MDNS.addServiceTxt("_uc-dock", "_tcp", "model", m_config.getDeviceModel());
        MDNS.addServiceTxt("_uc-dock", "_tcp", "ver", m_config.getFWVersion());
        MDNS.addServiceTxt("_uc-dock", "_tcp", "name", m_config.getFriendlyName());
        // available in archived repo but not used in v2 any more?
        // MDNS.addService("_uc-dock-ota", "_tcp", m_config.OTA_port);
        Serial.printf("Started mDNS with hostname: %s\n", hostname.c_str());
        Serial.printf("Announcing friendly name: %s\n", m_config.getFriendlyName().c_str());
        
        Serial.println(F("mDNS services updated"));
        m_forceRestart = false;
    }
}
void MDNSService::stopService()
{
    MDNS.end();
}

void MDNSService::loop()
{
    if (m_forceRestart)
    {
        stopService();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        startService();
    }
}
