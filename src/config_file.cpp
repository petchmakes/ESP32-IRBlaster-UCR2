#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include "global_config.h"
#include <config_file.h>

#define FORMAT_SPIFFS_IF_FAILED true

JsonDocument loadConfig()
{
    Serial.println("loadConfig");
    // Default configuration
    JsonDocument doc;
    doc[CONFIG_FILE_WIFI_SSID_ENTRY] = "";
    doc[CONFIG_FILE_WIFI_PASSWORD_ENTRY] = "";

    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println("SPIFFS Mount Failed");
        return doc;
    }
    /*while (!Serial)
        continue;*/

    Serial.printf("Reading  config file JSON from SPIFFS: %s\n", CONFIG_FILE_REMOTE);
    File file = SPIFFS.open(CONFIG_FILE_REMOTE);
    if (!file)
    {
        Serial.println("No config file, returns default configuration");
        return doc;
    }
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.println(F("Failed to read file, using default configuration"));
        if (file)
            file.close();
        return doc;
    }
    Serial.println("Config loaded :");
    serializeJsonPretty(doc, Serial);
    Serial.print("\n");
    return doc;
}

bool saveConfig(JsonDocument doc)
{
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    while (!Serial)
        continue;
    File file = SPIFFS.open(CONFIG_FILE_REMOTE, "w", true);
    if (!file)
    {
        Serial.println("Unable to save configuration");
        return false;
    }
    serializeJson(doc, file);
    if (file)
        file.close();
    Serial.println("Configuration file saved");
    return true;
}