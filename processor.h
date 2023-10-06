#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>
#include "stack.h"

struct CPU {

    stack stk;

    int   rax;
    int   rbx;
    int   rcx;
    int   rdx;

};

#endif // PROCESSOR_H
