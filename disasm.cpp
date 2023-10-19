#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "commands.h"

const char * DISASM_FILENAME = "disasmed.txt";

static int DisAssemble(const char * asm_fname, const char * out_fname);

int main() {

    fprintf(stdout, "\n"
                    "# Disassembler by NeTort, 2023\n"
                    "# Working...\n"
                    "# Like a surgeon carefully slicing flesh of byte code array and showing all its organs\n\n");

    DisAssemble(BIN_FILENAME, DISASM_FILENAME);

    return 0;
}

int DisAssemble(const char * asm_fname, const char * out_fname) {

    assert (asm_fname);
    assert (out_fname);

    FILE * asm_file = fopen(asm_fname, "rb");

    // read size of the long long byte code array
    size_t n_cmds = 0;
    fread(&n_cmds, sizeof(n_cmds), 1, asm_file);

    // read byte code array: form and fill prog_code array
    int * prog_code = (int *) calloc(n_cmds, sizeof(int));
    assert(prog_code);
    int * const prog_code_init = prog_code;

    size_t readen = 0;
    readen = fread(prog_code, sizeof(int), n_cmds, asm_file);
    assert(readen == n_cmds);

    fclose(asm_file);

    FILE * fout = fopen(out_fname, "wb");

    int val     = 0;

    for (size_t ip = 0; ip < n_cmds; ip++) {

        switch (*prog_code) {

            case CMD_HLT:
                {

                fprintf(fout, "hlt\n");

                break;

                }

            case ARG_IMMED_VAL | CMD_PUSH:
                {

                prog_code++;
                ip++;
                val = *prog_code;

                fprintf(fout, "push %d\n", val);

                break;
                }

            case ARG_REGTR_VAL | CMD_PUSH:
                {

                prog_code++;
                ip++;
                val = *prog_code;

                fprintf(fout, "push %d\n", val);
                break;

                }

            case CMD_POP:
                {

                fprintf(fout, "pop\n");
                break;

                }

            case ARG_REGTR_VAL | CMD_POP:
                {

                prog_code++;
                ip++;
                val = *prog_code;
                fprintf(fout, "pop %d\n", val);
                break;

                }

            case CMD_IN:
                {

                fprintf(fout, "in\n");
                break;

                }

            case CMD_OUT:
                {

                fprintf(fout, "out\n");
                break;

                }

            case CMD_ADD:
                {

                fprintf(fout, "add\n");
                break;

                }

            case CMD_SUB:
                {

                fprintf(fout, "sub\n");
                break;

                }

            case CMD_MUL:
                {

                fprintf(fout, "mul\n");
                break;

                }

            case CMD_DIV:
                {

                fprintf(fout, "div\n");
                break;

                }

            default:
                {
                fprintf(fout, "DISASM CMD NOT FOUND (CODE = %d)\n", *prog_code);
                break;
                }
        }
    }

    fclose(fout);
    free(prog_code_init);

    return 0;
}
