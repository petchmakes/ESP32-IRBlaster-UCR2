
#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

/* 
file based on yio repo
https://github.com/YIO-Remote/dock-software/blob/master/lib/config/config.cpp

 */

#include <ESPmDNS.h>
#include <libconfig.h>

class MDNSService
{
public:
    explicit MDNSService();
    virtual ~MDNSService() {}

    static MDNSService*           getInstance() { return s_instance; }

    void restartService() {m_forceRestart = true;}

    void loop();
    void addFriendlyName(String name);

private:
    static MDNSService*           s_instance;
    void startService();
    void stopService();
    boolean m_forceRestart = false;

    String m_mdnsHostname;
    Config*                       m_config = Config::getInstance();
};

#endif


