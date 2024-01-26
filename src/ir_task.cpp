// Copyright 2024 Craig Petchell

#include <freertos/FreeRTOS.h>
#include <IRsend.h>
#include <IRutils.h>

#include <ir_message.h>
#include <ir_task.h>
#include <blaster_config.h>
#include <ir_queue.h>

uint16_t irRepeat = 0;
ir_message_t repeatMessage;
IRsend irsend(true, 0);

bool repeatCallback()
{
    if (irRepeat > 0)
    {
        irRepeat--;
        return true;
    }
    return false;
}

void irSetup()
{
    //pinMode(BLASTER_PIN_INDICATOR, OUTPUT);
    pinMode(BLASTER_PIN_IR_INTERNAL, OUTPUT);
    //pinMode(BLASTER_PIN_IR_OUT_1, OUTPUT);
    //pinMode(BLASTER_PIN_IR_OUT_2, OUTPUT);

    irsend.setRepeatCallback(repeatCallback);
    irsend.begin();
}

void sendProntoCode(ir_message_t &message)
{
    irsend.sendPronto(message.code16, message.codeLen, message.repeat);
    irRepeat = 0;
}

void sendHexCode(ir_message_t &message)
{
    // Copy so not overritten on next message
    memcpy(&repeatMessage, &message, sizeof(ir_message_t));
    irRepeat = message.repeat;
    if (hasACState(message.decodeType))
    {
        irsend.send(message.decodeType, message.code8, message.codeLen);
    }
    else
    {
        irsend.send(message.decodeType, message.code64, message.codeLen, irRepeat);
    }
}

void TaskSendIR(void *pvParameters)
{
    Serial.printf("TaskSendIR running on core %d\n", xPortGetCoreID());

    irSetup();
    ir_message_t message;
    for (;;)
    {
        if (irQueueHandle != NULL)
        {
            int ret = xQueueReceive(irQueueHandle, &message, portMAX_DELAY);
            if (ret == pdPASS)
            {
                switch (message.action)
                {
                case send:
                {
                    //uint32_t ir_pin_mask = 0 | 1 << BLASTER_PIN_INDICATOR | message.ir_internal << BLASTER_PIN_IR_INTERNAL | message.ir_ext1 << BLASTER_PIN_IR_OUT_1 | message.ir_ext2 << BLASTER_PIN_IR_OUT_2;
                    uint32_t ir_pin_mask = 0;
                    if (BLASTER_PIN_INDICATOR) ir_pin_mask |= 1 << BLASTER_PIN_INDICATOR;
                    if (BLASTER_PIN_IR_INTERNAL) ir_pin_mask |= message.ir_internal << BLASTER_PIN_IR_INTERNAL;
                    if (BLASTER_PIN_IR_OUT_1) ir_pin_mask |= message.ir_ext1  << BLASTER_PIN_IR_OUT_1;
                    if (BLASTER_PIN_IR_OUT_2) ir_pin_mask |= message.ir_ext2  << BLASTER_PIN_IR_OUT_2;

                    irsend.setPinMask(ir_pin_mask);
                    switch (message.format)
                    {
                    case pronto:
                        sendProntoCode(message);
                        break;
                    case hex:
                        sendHexCode(message);
                        break;
                    }
                    break;
                }
                case repeat:
                {
                    irRepeat += message.repeat;
                    break;
                }
                case stop:
                default:
                {
                    irRepeat = 0;
                    break;
                }
                }
            }
        }
    }
}