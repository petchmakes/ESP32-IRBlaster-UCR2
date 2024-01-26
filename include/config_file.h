#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <Arduino.h>
#include <ArduinoJson.h>
#define CONFIG_FILE_WIFI_SSID_ENTRY "wifi_ssid"
#define CONFIG_FILE_WIFI_PASSWORD_ENTRY "wifi_password"

JsonDocument loadConfig();
bool saveConfig(JsonDocument doc);

#ifdef __cplusplus
}
#endif

#endif
