#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>

#include "stack.h"

const int    SPU_STK_CAPA = 100;

struct SPU {

    int rax;
    int rbx;
    int rcx;
    int rdx;

    stack stk;

};

#endif // PROCESSOR_H
