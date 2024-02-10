// Copyright 2024 Alex Koessler
// Based on file Copyrighted by 2024 Craig Petchell


#ifndef IR_SERVICE_H_
#define IR_SERVICE_H_


#include <Arduino.h>
#include <ArduinoJson.h>




void queueIR(JsonDocument &input, JsonDocument &output);

void stopIR(JsonDocument &input, JsonDocument &output);


#endif