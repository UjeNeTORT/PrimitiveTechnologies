#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>

#include "../stacklib/stack.h"

const int SHOW_INTERMED_INFO = 1;

const int SPU_STK_CAPA      = 10000;
const int SPU_CALL_STK_CAPA = 100;
const int SPU_REGS_NUM      = 26;  // 26 letters in english alphabet

const int SPU_VRAM_WIDTH     = 11; // for graphics
const int SPU_VRAM_HIGHT     = 11; // for graphics
const int VRAM_MAPPING       = 100;

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
