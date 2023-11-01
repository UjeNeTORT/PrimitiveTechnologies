#ifndef ENUMS_H
#define ENUMS_H

#include <stdio.h>

const char * BIN_FILENAME = "translated.bin";

const size_t MAX_CMD = 100;
const size_t MAX_LBLS = 50; // random number

typedef char cmd_code_t;

const cmd_code_t OPCODE_MSK    = (cmd_code_t) 0b0001'1111;
const cmd_code_t ARG_TYPE_MSK  = (cmd_code_t) 0b1110'0000;
const cmd_code_t ARG_IMMED_VAL = (cmd_code_t) 0b0010'0000;
const cmd_code_t ARG_REGTR_VAL = (cmd_code_t) 0b0100'0000;
const cmd_code_t ARG_MEMRY_VAL = (cmd_code_t) 0b1000'0000;

enum CMDS
{
    #define DEF_CMD(name, num, ...) \
        CMD_##name = num,

    #include "commands.h"

    #undef DEF_CMD
};

#endif // ENUMS_H
