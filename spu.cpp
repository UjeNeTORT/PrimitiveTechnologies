#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "spu.h"
#include "./stacklib/stack.h"
#include "./text_processing_lib/text_buf.h"

static int RunBin     (const char * in_fname);

static int FixEndianess(int num);
static int DivideInts (int numerator, int denominator);

int main() {

    fprintf(stdout, "\n"
                    "# Processor by NeTort, 2023\n"
                    "# Does stuff... What else can i say?\n"
                    "# Today i ve accidentially skipped descrete analisys seminar. Im such a morron.\n"
                    "# On the other hand i have coded this app faster...\n\n");

    printf("%d\n", FixEndianess(1));

    RunBin(BIN_FILENAME);

    return 0;
}

/** assume that binary file contains int byte code array where each number except 1st one contains command / argument
 *
*/
int RunBin (const char * in_fname) {

    assert(in_fname);

    //================================================================== CREATE AND INITIALIZE CPU STRUCTURE

    SPU my_spu = {};

    CtorStack(&my_spu.stk, SPU_STK_CAPA);

    //=======================================================================================================

    FILE * in_file = fopen(in_fname, "rb");

    // read size of the long long byte code array
    size_t n_bytes = 0;
    fread(&n_bytes, sizeof(size_t), 1, in_file);

    // read byte code array: form and fill prog_code array
    char * prog_code = (char *) calloc(n_bytes, sizeof(char));
    assert(prog_code);
    char * const prog_code_init = prog_code;

    size_t readen = 0;
    readen = fread(prog_code, sizeof(char), n_bytes, in_file);
    assert(readen == n_bytes);

    fclose(in_file);

    POP_OUT pop_err = POP_NO_ERR;
    int val     = 0;
    int in_var  = 0;

    for (size_t ip = 0; ip < n_bytes; ip++) {

        switch (prog_code[ip]) {

            case CMD_HLT:
            {

                fprintf(stderr, "# Hlt encountered, goodbye!\n");

                free(prog_code_init);
                DtorStack(&my_spu.stk);

                return 0;
            }

            case ARG_IMMED_VAL | CMD_PUSH:
            {

                ip += 1;

                // val = ((int *)prog_code)[ip];
                val = *(int *)(prog_code + ip);

                ip += 3;

                PushStack(&my_spu.stk, val);

                fprintf(stderr, "# Push imm val (%d)\n", val);

                break;
            }

            case ARG_REGTR_VAL | CMD_PUSH:
            {

                fprintf(stderr, "# Push from register\n");

                ip++;
                val = prog_code[ip];

                PushStack(&my_spu.stk, my_spu.regs[val]);

                val = 0;

                break;
            }

            case CMD_POP:
            {

                fprintf(stderr, "# Pop immediate number\n");

                pop_err = POP_NO_ERR;
                PopStack(&my_spu.stk, &pop_err);

                break;
            }

            case ARG_REGTR_VAL | CMD_POP:
            {

                fprintf(stderr, "# Pop to register\n");
                pop_err = POP_NO_ERR;

                ip++;
                val = prog_code[ip];

                my_spu.regs[val] = PopStack(&my_spu.stk, &pop_err);

                break;
            }

            case CMD_IN:
            {

                fprintf(stdout, "In: please enter your variable...\n>> ");

                in_var = 0;
                fscanf(stdin, "%d", &in_var);

                PushStack(&my_spu.stk, in_var);

                break;
            }

            case CMD_OUT:
            {

                pop_err = POP_NO_ERR;
                fprintf(stdout, "Out:   %d\n", PopStack(&my_spu.stk, &pop_err));

                break;
            }

            case CMD_ADD:
            {

                pop_err = POP_NO_ERR;
                val = 0;
                val = PopStack(&my_spu.stk, &pop_err) + PopStack(&my_spu.stk, &pop_err);
                PushStack(&my_spu.stk, val);

                break;
            }

            case CMD_SUB:
            {

                pop_err = POP_NO_ERR;
                val = 0;
                val -= PopStack(&my_spu.stk, &pop_err);
                val += PopStack(&my_spu.stk, &pop_err);
                PushStack(&my_spu.stk, val);

                break;
            }

            case CMD_MUL:
            {

                pop_err = POP_NO_ERR;
                val = 0;
                val =  PopStack(&my_spu.stk, &pop_err) * PopStack(&my_spu.stk, &pop_err);
                PushStack(&my_spu.stk, val);

                break;
            }

            case CMD_DIV:
            {

                pop_err = POP_NO_ERR;
                int denominator = PopStack(&my_spu.stk, &pop_err);
                int numerator   = PopStack(&my_spu.stk, &pop_err);

                val = 0;
                val = DivideInts(numerator, denominator);

                PushStack(&my_spu.stk, val);

                break;
            }

            case ARG_IMMED_VAL | CMD_JMP:
            {

                ip = prog_code[ip + 1];

                fprintf(stderr, "# Jump to %lu\n", ip);

                ip--; // because ip is to be increased in for-statement
                break;
            }

            default:
            {

                fprintf(stderr, "# Syntax Error! No command \"%d\" (%lu) found! Bye bye looser!\n", prog_code[ip], ip);

                return 1;
            }
        }

        val     = 0;
        in_var  = 0;

    }

    free(prog_code_init);
    DtorStack(&my_spu.stk);

    return 0;
}

int FixEndianess(int num) {

    char byte1 = (num & 0xff'00'00'00) >> 3*8;
    char byte2 = (num & 0x00'ff'00'00) >> 2*8;
    char byte3 = (num & 0x00'00'ff'00) >> 1*8;
    char byte4 = (num & 0x00'00'00'ff) >> 0*8;

    int res = 0;

    res |= byte4 << 3*8;
    res |= byte3 << 2*8;
    res |= byte2 << 1*8;
    res |= byte1 << 0*8;

    return res;
}

 int DivideInts(int numerator, int denominator) {
    return (int) numerator / denominator;
 }
