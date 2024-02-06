#ifndef API_SERVICE_H
#define API_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include <libconfig.h>


void processData(JsonDocument &request, int id, String source, void (*sendCallback)(JsonDocument));
void fillDefaultResponseFields(JsonDocument &input, JsonDocument &output, int code=200, boolean reboot=false);
void processSetConfig(JsonDocument &input, JsonDocument &output);
void buildSysinfoResponse(JsonDocument &input, JsonDocument &output);
void buildConnectionResponse(JsonDocument &input, JsonDocument &output);


#endif