// Copyright by Alex Koessler

// file based on archived yio repo
// https://github.com/YIO-Remote/dock-software/tree/master/lib/service_mdns

// Provides mDNS Service that announces the dock to remotes in the same network.


#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include <ESPmDNS.h>
#include <libconfig.h>

class MDNSService
{
public:

    //static MDNSService *getInstance() { return s_instance; }

    static MDNSService& getInstance()
    {
        static MDNSService instance;
        return instance;
    }

    // Triggers a restart of the mDNS service with current config
    void restartService() { m_forceRestart = true; }
    void startService();

    // Worker loop
    void loop();

private:
    explicit MDNSService() {}
    virtual ~MDNSService() {}

//    static MDNSService *s_instance;
    boolean m_forceRestart = false;
    String m_mdnsHostname;
    Config &m_config = Config::getInstance();

    //void startService();
    void stopService();
};

#endif
