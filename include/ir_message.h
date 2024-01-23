// Copyright 2024 Craig Petchell

#ifndef IR_MESSAGE_H_
#define IR_MESSAGE_H_

#include <Arduino.h>
#include <IRsend.h>

#define MAX_IR_CODE_LENGTH 2048

enum ir_action {
    stop,
    send,
    repeat,
};

enum ir_format {
    pronto,
    hex
};

typedef struct {
    ir_action action;
    ir_format format;
    union {
        uint16_t code16[MAX_IR_CODE_LENGTH/2];
        uint8_t code8[MAX_IR_CODE_LENGTH];
        uint64_t code64;
    };
    uint16_t codeLen;
    decode_type_t decodeType;
    uint16_t repeat = 0;
    bool ir_internal;
    bool ir_ext1;
    bool ir_ext2;
} ir_message_t;

#endif
