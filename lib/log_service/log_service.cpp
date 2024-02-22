
#include "log_service.h"
#include <time.h>


#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 2
#endif

String enum2txt(LogLevel lvl){
    switch (lvl)
    {
    case LOG_DEBUG:
        return "D";
        break;
    case LOG_INFO:
        return "I";
        break;
    case LOG_WARNING:
        return "W";
        break;
    case LOG_ERROR:
        return "E";
        break;
    
    default:
        return "-";
        break;
    }

}



void logOutput(LogLevel lvl, String module, String logMsg){
    if(lvl <= DEBUG_LEVEL){
        struct tm timeinfo;
        char dateTime[20];
        getLocalTime(&timeinfo);
        strftime(dateTime,sizeof(dateTime), "%F %T", &timeinfo);
        Serial.printf("%s [%s] [%s] %s\n", dateTime, enum2txt(lvl), module, logMsg.c_str());
    }
}
