#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>

#include "../stacklib/stack.h"

const int SPU_STK_CAPA      = 1000;
const int SPU_CALL_STK_CAPA = 100;
const int SPU_REGS_NUM      = 26;   // because i want 4
const int SPU_RAM_WIDTH     = 10;  // for graphics
const int SPU_RAM_HIGHT     = 10;  // for graphics

const int STK_PRECISION = 100;

typedef enum {
    REACH_HLT   = 0, //< reached hlt
    REACH_END   = 1, //< reached end
    ILL_CDMCODE = 2, //< occured error
} RunBinRes;

struct SPU {

    // general purpose
    // also need instruction pointer register
    Elem_t gp_regs[SPU_REGS_NUM];

    stack stk;

    stack call_stk;

    Elem_t * RAM;
};

#endif // PROCESSOR_H
