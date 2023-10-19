#include <assert.h>
#include <stdio.h>

#include "my_hash.h"

const size_t DENOMINATOR = 1000000007; // prostoye

Hash_t HashMod(const void * val, int size) {

    assert (val);

    Hash_t hash = 0;

    int char_num = size % sizeof(int);

    for (size_t i = 0; i < size / sizeof(int); i++) {
        hash += *(const int * ) val % DENOMINATOR;
        hash %= DENOMINATOR;
        val = (const int *) val + 1;
    }

    for (int i = 0; i < char_num; i++) {
        hash += *(const char *) val % DENOMINATOR;
        hash %= DENOMINATOR;
        val = (const char *) val + 1;
    }

    return hash;
}
