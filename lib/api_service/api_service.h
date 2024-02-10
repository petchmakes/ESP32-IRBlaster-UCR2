// Copyright by Alex Koessler

// Provides API decoding service for communication with dock via bluetooth and wifi.

#ifndef API_SERVICE_H
#define API_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>




void api_processData(JsonDocument &request, JsonDocument &response);

void api_fillDefaultResponseFields(JsonDocument &input, JsonDocument &output, int code = 200, boolean reboot = false);

void api_buildSysinfoResponse(JsonDocument &input, JsonDocument &output);

void api_buildConnectionResponse(JsonDocument &input, JsonDocument &output);

#endif