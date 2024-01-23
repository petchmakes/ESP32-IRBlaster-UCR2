// Copyright 2024 Craig Petchell

#ifndef IR_MESSAGE_H_
#define IR_MESSAGE_H_

#include <Arduino.h>

#define MAX_IR_CODE_LENGTH 2048

enum ir_action {
    stop,
    send,
};

enum ir_format {
    pronto,
    hex
};

typedef struct {
    ir_action action;
    ir_format format;
    uint16_t code[MAX_IR_CODE_LENGTH];
    uint16_t codeLen;
    uint16_t repeat = 0;
    bool ir_internal;
    bool ir_ext1;
    bool ir_ext2;
} ir_message_t;

#endif
