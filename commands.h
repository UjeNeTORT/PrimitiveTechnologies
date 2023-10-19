#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>

const size_t MAX_CMD = 100;
const size_t MAX_LBLS = 5; // random number

const char * BIN_FILENAME = "translated.bin";

const int ARG_IMMED_VAL = 0b0001'0000;
const int ARG_REGTR_VAL = 0b0010'0000;

struct Label {

    int    cmd_ptr;
    char * name;

};

enum CMDS {
    CMD_HLT  = -1,
    CMD_PUSH =  1,
    CMD_POP  = 11,
    CMD_IN   =  3,
    CMD_OUT  =  4,
    CMD_ADD  =  5,
    CMD_SUB  =  6,
    CMD_MUL  =  7,
    CMD_DIV  =  8,
    CMD_JMP  =  9,
};

#endif // COMMANDS_H
