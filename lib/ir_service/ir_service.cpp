// Copyright 2024 Craig Petchell


#include <string.h>
#include <IRutils.h>

#include <api_service.h>


#include "ir_queue.h"
#include "ir_message.h"
#include "ir_service.h"

//#include <libconfig.h>


#define MAX_IR_TEXT_CODE_LENGTH 2048
#define MAX_IR_FORMAT_TYPE 50

// Current text ir code
char irCode[MAX_IR_TEXT_CODE_LENGTH] = "";
char irFormat[MAX_IR_FORMAT_TYPE] = "";



void buildProntoMessage(ir_message_t &message)
{
    const int strCodeLen = strlen(irCode);

    char workingCode[strCodeLen + 1] = "";
    char *workingPtr;
    message.codeLen = (strCodeLen + 1) / 5;
    uint16_t offset = 0;

    strcpy(workingCode, irCode);
    
    char delimiter[2] = {0,0};
    delimiter[0] = workingCode[4];
    if((delimiter[0] != ' ') && (delimiter[0] != ','))
    {
        Serial.printf("Pronto delimiter not recognized. Prontocode: %s", workingCode);
    }
    
    char *hexCode = strtok_r(workingCode, delimiter, &workingPtr);
    while (hexCode)
    {
        message.code16[offset++] = strtoul(hexCode, NULL, 16);
        hexCode = strtok_r(NULL, delimiter, &workingPtr);
    }

    message.format = pronto;
    message.action = send;
    message.decodeType = PRONTO;
}

void buildHexMessage(ir_message_t &message)
{
    // Split the UC_CODE parameter into protocol / code / bits / repeats
    char workingCode[MAX_IR_TEXT_CODE_LENGTH];
    char *parts[4];
    int partcount = 0;

    strcpy(workingCode, irCode);
    parts[partcount++] = workingCode;

    char *ptr = workingCode;
    while (*ptr && partcount < 4)
    {
        if (*ptr == ';')
        {
            *ptr = 0;
            parts[partcount++] = ptr + 1;
        }
        ptr++;
    }

    if (partcount != 4)
    {
        Serial.print("Unvalid UC code");
        return;
    }

    message.decodeType = strToDecodeType(parts[0]);

    switch (message.decodeType)
    {
    // Unsupported types
    case decode_type_t::UNUSED:
    case decode_type_t::UNKNOWN:
    case decode_type_t::GLOBALCACHE:
    case decode_type_t::PRONTO:
    case decode_type_t::RAW:
        Serial.print("The protocol specified is not supported by this program.");
        return;
    default:
        break;
    }

    uint16_t nbits = static_cast<uint16_t>(std::stoul(parts[2]));
    if (nbits == 0 && (nbits <= kStateSizeMax * 8))
    {
        Serial.printf("No of bits %s is invalid\n", parts[2]);
        return;
    }

    uint16_t stateSize = nbits / 8;

    uint16_t repeats = static_cast<uint16_t>(std::stoul(parts[3]));
    if (repeats > 20)
    {
        Serial.printf("Repeat count is too large: %d. Maximum is 20.\n", repeats);
        return;
    }

    if (!hasACState(message.decodeType))
    {
        message.code64 = std::stoull(parts[1], nullptr, 16);
        message.codeLen = nbits;
    }
    else
    {

        String hexstr = String(parts[1]);
        uint64_t hexstrlength = hexstr.length();
        uint64_t strOffset = 0;
        if (hexstrlength > 1 && hexstr[0] == '0' &&
            (hexstr[1] == 'x' || hexstr[1] == 'X'))
        {
            // Skip 0x/0X
            strOffset = 2;
        }

        hexstrlength -= strOffset;

        memset(&message.code8, 0, stateSize);
        message.codeLen = stateSize;

        // Ptr to the least significant byte of the resulting state for this
        // protocol.
        uint8_t *statePtr = &message.code8[stateSize - 1];

        // Convert the string into a state array of the correct length.
        for (uint16_t i = 0; i < hexstrlength; i++)
        {
            // Grab the next least sigificant hexadecimal digit from the string.
            uint8_t c = tolower(hexstr[hexstrlength + strOffset - i - 1]);
            if (isxdigit(c))
            {
                if (isdigit(c))
                    c -= '0';
                else
                    c = c - 'a' + 10;
            }
            else
            {
                Serial.printf("Code %s contains non-hexidecimal characters.", parts[1]);
                return;
            }
            if (i % 2 == 1)
            { // Odd: Upper half of the byte.
                *statePtr += (c << 4);
                statePtr--; // Advance up to the next least significant byte of state.
            }
            else
            { // Even: Lower half of the byte.
                *statePtr = c;
            }
        }
    }

    message.format = hex;
    message.action = send;
}

void queueIRMessage(ir_message_t &message)
{

    if (irQueueHandle != NULL)
    {
        int ret = xQueueSend(irQueueHandle, (void *)&message, 0);
        if (ret == pdTRUE)
        {
            // The message was successfully sent.
            Serial.println("Action successfully send to the IR Queue");
        }
        else if (ret == errQUEUE_FULL)
        {
            Serial.println("The `TaskWeb` was unable to send message to IR Queue");
        }
    }
    else
    {
        Serial.println("The `TaskWeb` was unable to send message to IR Queue; no queue defined");
    }

}

void queueIR(JsonDocument &input, JsonDocument &output)
{
    const char *newCode = input["code"];
    const char *newFormat = input["format"];
    const uint16_t newRepeat = input["repeat"];
    const bool ir_internal = input["int_side"] || input["int_top"];
    const bool ir_ext1 = input["ext1"];
    const bool ir_ext2 = input["ext2"];
    ir_message_t message;

    message.action = send;
    message.ir_internal = ir_internal;
    message.ir_ext1 = ir_ext1;
    message.ir_ext2 = ir_ext2;
    message.repeat = newRepeat;

    if (uxQueueMessagesWaiting(irQueueHandle) != 0) 
    {
        // Message being processed
        if(irCode[0] && (strcmp(newCode, irCode) == 0) && (strcmp(newFormat, irFormat) == 0))
        {
            // Same message. send repeat command
            api_fillDefaultResponseFields(input, output, 202);
            message.action = repeat;
            queueIRMessage(message);
            return;
        }
        else
        {
            // Different message - reject
            api_fillDefaultResponseFields(input, output, 429);
            return;
        }
    }

    if (strlen(newCode) > sizeof(irCode))
    {
        Serial.printf("Length of sent code is longer than allocated buffer. Length = %d; Max = %s\n", strlen(newCode), sizeof(irCode));
        api_fillDefaultResponseFields(input, output, 400);
        return;
    }

    if (strlen(newFormat) > sizeof(irFormat))
    {
        Serial.printf("Length of sent format is longer than allocated buffer. Length = %d; Max = %s\n", strlen(newFormat), sizeof(irFormat));
        api_fillDefaultResponseFields(input, output, 400);
        return;
    }

    strcpy(irCode, newCode);
    strcpy(irFormat, newFormat);

    if (strcmp("hex", newFormat) == 0)
    {
        buildHexMessage(message);
        queueIRMessage(message);
        api_fillDefaultResponseFields(input, output);
    }
    else if (strcmp("pronto", newFormat) == 0)
    {
        buildProntoMessage(message);
        queueIRMessage(message);
        api_fillDefaultResponseFields(input, output);
    }
    else
    {
        Serial.printf("Unknown ir format %s\n", newFormat);
        api_fillDefaultResponseFields(input, output, 400);
        irCode[0] = 0;
        irFormat[0] = 0;
    }
}

void stopIR(JsonDocument &input, JsonDocument &output)
{
    ir_message_t message;
    message.action = stop;

    queueIRMessage(message);
    api_fillDefaultResponseFields(input, output);
}
