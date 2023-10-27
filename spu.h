#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>

#include "./stacklib/stack.h"

const int SPU_STK_CAPA      = 1000;
const int SPU_CALL_STK_CAPA = 100;
const int SPU_REGS_NUM      = 4;   // because i want 4
const int SPU_RAM_SIZE      = 100; // for graphics

struct SPU {

    // general purpose
    // also need instruction pointer register
    int gp_regs[SPU_REGS_NUM];


    stack stk;

    stack call_stk;

    int * RAM;

};

#endif // PROCESSOR_H
