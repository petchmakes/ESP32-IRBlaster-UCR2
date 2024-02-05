
#include "bt_service.h"
#include "api_service.h"


/*
This file was created using information and code snippets from:

https://github.com/YIO-Remote/dock-software
https://github.com/espressif/arduino-esp32/tree/master/libraries/BluetoothSerial


*/





BluetoothService* BluetoothService::s_instance = nullptr;

BluetoothService::BluetoothService(){
    s_instance = this;
}

void btConnectCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT){
    Serial.println(F("BT Client connected"));
  }
  if(event == ESP_SPP_CLOSE_EVT ){
    Serial.println(F("BT Client disconnected"));
  }
}


void BluetoothService::sendCallback(JsonDocument responseJson){
    String outString;
    serializeJson(responseJson, outString);
    btSerial->println(outString);
    Serial.println("Raw Json response: " + outString);
    if(responseJson["reboot"].as<boolean>()){
      Serial.println(F("Rebooting..."));
      delay(1000);
      ESP.restart();
    }
}

void sCallback(JsonDocument responseJson){
    BluetoothService::getInstance()->sendCallback(responseJson);
}

/*
String buildHostName(void)
{
    char hostName[] = "UC-Dock-X-xx-xx-xx-xx-xx-xx";
    uint8_t wifiMac[6];
    esp_read_mac(wifiMac, ESP_MAC_WIFI_STA);
    sprintf(hostName, "UC-Dock-X-%02X-%02X-%02X-%02X-%02X-%02X", wifiMac[0], wifiMac[1], wifiMac[2], wifiMac[3], wifiMac[4], wifiMac[5]);
    return hostName;    
}
*/

void BluetoothService::init()
{
  btSerial->register_callback(btConnectCallback);

  if (btSerial->begin(m_config->getHostName())) {
    Serial.print(F("BT initialized with Hostname "));
    Serial.println(m_config->getHostName().c_str());
  } else {
    Serial.println(F("BT initialization failed!"));
  }
}

void BluetoothService::handle()
{
    char incomingChar = btSerial->read();
    //int charAsciiNumber = incomingChar + 0;

    if (String(incomingChar) == "{")
    {
      m_interestingData = true;
      m_receivedData = "";
    }
    else if (String(incomingChar) == "}")
    {
      m_interestingData = false;
      m_receivedData += "}";

      JsonDocument requestJson;
      DeserializationError error = deserializeJson(requestJson, m_receivedData);

      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
      } else {
        Serial.print("Received Json: ");
        Serial.println(m_receivedData.c_str());
        processData(requestJson, 0, "bluetooth", sCallback);
      }
      m_receivedData = "";
    }
    if (m_interestingData)
    {
      m_receivedData += String(incomingChar);
    }
    delay(10);
}


