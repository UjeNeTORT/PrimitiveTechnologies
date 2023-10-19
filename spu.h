#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>

#include "./stacklib/stack.h"

const int SPU_STK_CAPA = 100;
const int SPU_REGS_NUM = 4;

struct SPU {

    int regs[SPU_REGS_NUM];

    stack stk;

};

#endif // PROCESSOR_H
