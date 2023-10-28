#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>

#include "./stacklib/stack.h"

#define PUSH_SPU_STK()                                              \
    int arg = GetArg(prog_code, &ip, my_spu.gp_regs, my_spu.RAM);   \
    PushStack(&my_spu.stk, arg);                                    \

#define INCREASE_IP(cmd)         \
    ip += sizeof(char);          \
    if (cmd & ARG_IMMED_VAL)     \
        ip += sizeof(int);       \
    if (cmd & ARG_REGTR_VAL)     \
        ip += sizeof(char);      \

const int SPU_STK_CAPA      = 1000;
const int SPU_CALL_STK_CAPA = 100;
const int SPU_REGS_NUM      = 26;   // because i want 4
const int SPU_RAM_WIDTH     = 10;  // for graphics
const int SPU_RAM_HIGHT     = 10;  // for graphics

const int STK_PRECISION = 100;

typedef enum {
    REACH_HLT = 0, //< reached hlt
    REACH_END = 1, //< reached end
    OCC_ERROR = 2, //< occured error
} RunBinRes;

typedef enum {
    ALL_OK = 0,     //< free done its job
    FREE_TRBLS = 1, //< free couldnt free memory
} FinishWorkRes;

typedef enum {
    MULT = 1,
    DIV = 2,
} MultDblFrom;

struct SPU {

    // general purpose
    // also need instruction pointer register
    int gp_regs[SPU_REGS_NUM];


    stack stk;

    stack call_stk;

    int * RAM;
};

#endif // PROCESSOR_H
