#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>

const size_t MAX_CMD = 100;
const size_t MAX_LBLS = 5; // random number

const char * BIN_FILENAME = "translated.bin";

const int ARG_IMMED_VAL = 0b0010'0000;
const int ARG_REGTR_VAL = 0b0100'0000;

struct Label {

    int    cmd_ptr;
    char * name;

};

enum CMDS {
    CMD_HLT  = -1,
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
    CMD_JF  = 16,
};

#endif // COMMANDS_H
