#include <stdio.h>

#include "asm.h"

int DisAssemble(const char * asm_fname, const char * out_fname) {

    FILE * asm_file = fopen(asm_fname, "rb");

    // read size of the long long byte code array
    long long prog_code_size = 0;
    fread(&prog_code_size, sizeof(prog_code_size), 1, asm_file);

    // read byte code array: form and fill asm_nums array
    long long *asm_nums = (long long *) calloc(prog_code_size, sizeof(long long));
    assert(asm_nums);
    long long * const asm_nums_init = asm_nums;

    size_t readen = 0;
    readen = fread(asm_nums, sizeof(long long), prog_code_size, asm_file);
    assert(readen == prog_code_size);

    fclose(asm_file);

    FILE * fout = fopen(out_fname, "wb");

    int val     = 0;
    int in_var  = 0;
    int reg_id  = 0;
    int *regptr = 0;

    for (int ip = 0; ip < prog_code_size; ip++) {

        switch (*asm_nums >> 32) {

            case CMD_HLT:

                fprintf(fout, "hlt\n");
                break;

            case ARG_IMMED_VAL | CMD_PUSH:

                fprintf(fout, "push %d\n", *asm_nums & VAL_MASK);
                break;

            case ARG_REGTR_VAL | CMD_PUSH:

                fprintf(fout, "push r%cx\n", 'a' + *asm_nums & VAL_MASK - 1);
                break;

            case CMD_POP:

                fprintf(fout, "pop\n");
                break;

            case ARG_REGTR_VAL | CMD_POP:

                fprintf(fout, "pop r%cx\n", 'a' + *asm_nums & VAL_MASK - 1);
                break;

            case CMD_IN:

                fprintf(fout, "in\n");
                break;

            case CMD_OUT:

                fprintf(fout, "out\n");
                break;

            case CMD_ADD:

                fprintf(fout, "add\n");
                break;

            case CMD_SUB:

                fprintf(fout, "sub\n");
                break;

            case CMD_MUL:

                fprintf(fout, "mul\n");
                break;

            case CMD_DIV:

                fprintf(fout, "div\n");
                break;
        }

        asm_nums++;
    }

    fclose(fout);
    free(asm_nums_init);

    return 0;
}
