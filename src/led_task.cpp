// Copyright 2024 Alex Koessler

#include <freertos/FreeRTOS.h>
#include <Arduino.h>

#include "led_task.h"
#include "blaster_config.h"
// #include <bt_service.h>

// normal: breath (white)

// remote_lowbattery: blink fast

// remote_charged: breath (white)

// charging --> on

// identify --> blink fast

// setup --> blink slow

enum ledState
{
    off,
    identify,
    normal,
    none,
};

ledState l_state = off;
ledState triggerState = none;
uint MAXPRC = 80;

uint current_pwm = 0;

uint goal_pwm = 0;
uint stepsize = 0;

#define UPDATERATE_MS 20
#define MINSTEP 10

#define RESOLUTION 10

uint prc2pwm(uint prc)
{
    return (prc * ((1 << RESOLUTION) - 1) / 100);
}

uint calcNextPwmVal(uint current, uint target, uint duration_ms)
{
    int diff = target - current;
    if (diff == 0)
    {
        // target reached. do nothing
        return target;
    }

    int absdiff = abs(diff);
    int sign = diff / abs(diff);

    float numstep = (float)duration_ms / (float)UPDATERATE_MS;
    float stepsize = (float)absdiff / numstep;
    int stepsize_i = (int)round(stepsize);

    if (absdiff <= MINSTEP)
    {
        // we are close enough to directly jump to the target
        return target;
    }
    if (stepsize_i <= MINSTEP)
    {
        // ramp is too slow, make at least minstep
        return (current + sign * MINSTEP);
    }
    // make normal step
    return (current + sign * stepsize_i);
}

void transition(uint *cur_pwm, uint target_pwm, uint duration_ms)
{
    uint remaining = duration_ms;

    while (*cur_pwm != target_pwm)
    {
        *cur_pwm = calcNextPwmVal(*cur_pwm, target_pwm, remaining);
        // output current pwm
        ledcWrite(0, *cur_pwm);

        remaining = remaining - UPDATERATE_MS;
        vTaskDelay(UPDATERATE_MS / portTICK_PERIOD_MS);
    }
}

void set_led_off(uint duration_ms = 50)
{
    transition(&current_pwm, 0, duration_ms);
}
void set_led_on(uint duration_ms = 50)
{
    uint onpwm = prc2pwm(MAXPRC);
    transition(&current_pwm, onpwm, duration_ms);
}
void breath_once(uint duration_ms = 6000)
{
    set_led_on(duration_ms / 2);
    set_led_off(duration_ms / 2);
}

void setPWMState(ledState nextState)
{
    if (l_state == nextState)
    {
        return;
    }
    if (l_state != off)
    {
        // transition to intermediate off state
        set_led_off(100);
    }
    l_state = nextState;
}

void setLedStateIdentify()
{
    triggerState = identify;
}

void TaskLed(void *pvParameters)
{
    Serial.printf("TaskLed running on core %d\n", xPortGetCoreID());

    pinMode(BLASTER_PIN_INDICATOR, OUTPUT);
    const int ledChannel = 0;

    // configure LED PWM functionalitites
    ledcSetup(ledChannel, 5000, RESOLUTION);

    // attach the channel to the GPIO2 to be controlled
    ledcAttachPin(BLASTER_PIN_INDICATOR, ledChannel);
    current_pwm = 0;
    ledcWrite(0, current_pwm);
    l_state = off;

    int i = 0;
    for (;;)
    {
        if (triggerState != none)
        {
            setPWMState(triggerState);
            triggerState = none;
        }
        switch (l_state)
        {
        case normal:
            // breath_once();
            break;
        case identify:
            for (i = 0; i < 2; ++i)
            {
                breath_once();
            }
            l_state = off;
            break;

        default:
            break;
        }

        // TODO: blink the led in the discovery pattern!

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
