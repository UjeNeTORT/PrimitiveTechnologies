#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>

const size_t MAX_CMD = 100;
const size_t MAX_LBLS = 50; // random number

const char * BIN_FILENAME = "translated.bin";

const char OPCODE_MSK    = (char) 0b0001'1111;
const char ARG_TYPE_MSK  = (char) 0b1110'0000;
const char ARG_IMMED_VAL = (char) 0b0010'0000;
const char ARG_REGTR_VAL = (char) 0b0100'0000;
const char ARG_MEMRY_VAL = (char) 0b1000'0000;

struct Label {

    int    cmd_ptr;
    char * name;
};

enum CMDS {
    CMD_HLT  = 0b0001'1111,
    CMD_PUSH =  1,
    CMD_POP  =  2,
    CMD_IN   =  3,
    CMD_OUT  =  4,
    CMD_ADD  =  5,
    CMD_SUB  =  6,
    CMD_MUL  =  7,
    CMD_DIV  =  8,
    CMD_JMP  =  9,
    CMD_JA   = 10,
    CMD_JAE  = 11,
    CMD_JB   = 12,
    CMD_JBE  = 13,
    CMD_JE   = 14,
    CMD_JNE  = 15,
    CMD_JF   = 16,
};

// // ??
// static int check_cmds()
// {
//     switch(0)
//     {
//         // #define DEF_CMD(code) case code:
//         case 0:
//         case 0:
//     }
// }

#endif // COMMANDS_H
