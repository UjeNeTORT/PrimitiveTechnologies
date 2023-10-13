#ifndef COMMANDS_H
#define COMMANDS_H

const size_t MAX_CMD = 100;

const long long ARG_IMMED_VAL = 16;
const long long ARG_REGTR_VAL = 32;
const long long VAL_MASK      = 0b0000'1111;
const long long CMD_MASK      = 0b1111'0000;

struct usr_cmd {

    const char * name;
    int    code;

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
};

const usr_cmd CMDS_ARR[] = {
    {"hlt",  CMD_HLT },
    {"push", CMD_PUSH},
    {"pop",  CMD_POP },
    {"in",   CMD_IN  },
    {"out",  CMD_OUT },
    {"add",  CMD_ADD },
    {"sub",  CMD_SUB },
    {"mul",  CMD_MUL },
    {"div",  CMD_DIV }
};

#endif // COMMANDS_H
