// Copyright 2024 Craig Petchell

#include <freertos/FreeRTOS.h>
#include <IRsend.h>

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
    pinMode(BLASTER_PIN_INDICATOR, OUTPUT);
    pinMode(BLASTER_PIN_IR_INTERNAL, OUTPUT);
    pinMode(BLASTER_PIN_IR_OUT_1, OUTPUT);
    pinMode(BLASTER_PIN_IR_OUT_2, OUTPUT);

    irsend.setRepeatCallback(repeatCallback);
    irsend.begin();
}

void sendProntoCode(ir_message_t &message)
{
    irsend.sendPronto(message.code, message.codeLen, message.repeat);
    irRepeat = 1;
}

void sendRawCode(ir_message_t &message)
{
    // Copy so not overritten on next message
    memcpy(&repeatMessage, &message, sizeof(ir_message_t));
    // irsend.sendRaw(message.code, message.codeLen, message.hz);
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
                if (message.action == stop)
                {
                    irRepeat = 0;
                }
                else
                {
                    // // TODO validate same type, if still repeating
                    uint32_t ir_pin_mask = 0 | 1 << BLASTER_PIN_INDICATOR | message.ir_internal << BLASTER_PIN_IR_INTERNAL | message.ir_ext1 << BLASTER_PIN_IR_OUT_1 | message.ir_ext2 << BLASTER_PIN_IR_OUT_2;
                    irsend.setPinMask(ir_pin_mask);
                    switch (message.format)
                    {
                    case pronto:
                        sendProntoCode(message);
                        break;
                    case hex:
                        sendRawCode(message);
                        break;
                    }
                }
            }
        }
    }
}