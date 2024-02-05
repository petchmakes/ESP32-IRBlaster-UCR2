#ifndef BT_SERVICE_H
#define BT_SERVICE_H

/*

This file was created using information and code snippets from:

https://github.com/YIO-Remote/dock-software

*/

#include <Arduino.h>

#include <ArduinoJson.h>
#include <api_service.h>

#include <libconfig.h>

#include "BluetoothSerial.h"

void sCallback(JsonDocument responseJson);

class BluetoothService
{
public:
    explicit BluetoothService();
    virtual ~BluetoothService() {}

    static BluetoothService* getInstance() { return s_instance; } 

    void init();
    void handle();
    void sendCallback(JsonDocument responseJson);


private:
    static BluetoothService* s_instance;

    BluetoothSerial* btSerial = new BluetoothSerial();
//    State*                        m_state = State::getInstance();
    Config*                       m_config = Config::getInstance();
   // API*                          m_api; 

    String m_receivedData = "";
    bool m_interestingData = false;
};




#endif