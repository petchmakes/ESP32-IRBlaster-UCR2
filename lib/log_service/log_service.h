
#include <Arduino.h>


enum LogLevel{
    LOG_ERROR=0,
    LOG_WARNING=1,
    LOG_INFO=2,
    LOG_DEBUG=3,
};


void logOutput(LogLevel lvl, String module, String logMsg);
