#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    size_t n_bytes = 0;
    fread(&n_bytes, sizeof(size_t), 1, asm_file);

    // read byte code array: form and fill prog_code array
    char * prog_code = (char *) calloc(n_bytes, sizeof(char));
    assert(prog_code);
    char * const prog_code_init = prog_code;

    size_t readen = 0;
    readen = fread(prog_code, sizeof(char), n_bytes, asm_file);
    assert(readen == n_bytes);

    fclose(asm_file);

    FILE * fout = fopen(out_fname, "wb");

    int val = 0;

    for (size_t ip = 0; ip < n_bytes; ip += sizeof(char)) {

        switch (prog_code[ip]) {

            case CMD_HLT:
            {
                fprintf(fout, "hlt\n");

                return 0;
            }

            case ARG_IMMED_VAL | CMD_PUSH:
            {
                ip += 1;

                memcpy(&val, (prog_code + ip), sizeof(int));

                ip+=3;

                fprintf(fout, "push %d\n", val);

                break;
            }

            case ARG_REGTR_VAL | CMD_PUSH:
            {
                ip++;

                memcpy(&val, (prog_code + ip), sizeof(char));

                fprintf(fout, "push r%cx\n", 'a' + val);

                break;
            }

            case CMD_POP:
            {
                fprintf(fout, "pop\n");

                break;
            }

            case ARG_REGTR_VAL | CMD_POP:
            {
                ip++;

                fprintf(fout, "pop r%cx\n", 'a' + val);

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

            case ARG_IMMED_VAL | CMD_JMP:
            {
                ip += 1;

                memcpy(&val, (prog_code + ip), sizeof(int));

                ip+=3;

                fprintf(fout, "jmp %d\n", val);

                break;
            }

            case ARG_IMMED_VAL | CMD_JA:
            {

                ip += 1;

                memcpy(&val, (prog_code + ip), sizeof(int));

                ip+=3;

                fprintf(fout, "ja %d\n", val);

                break;
            }
            default:
            {
                fprintf(stderr, "# DISASM: Syntax Error! No command \"%d\" (%lu) found! Bye bye looser!\n", prog_code[ip], ip);

                return 1;
            }
        }

        val = 0;
    }

    fclose(fout);
    free(prog_code_init);

    return 0;
}
