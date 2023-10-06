#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdio.h>
#include "stack.h"

struct CPU {

    int   rax;
    int   rbx;
    int   rcx;
    int   rdx;

    stack stk;

};

#endif // PROCESSOR_H
