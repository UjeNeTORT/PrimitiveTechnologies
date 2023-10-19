#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "spu.h"
#include "./stacklib/stack.h"
#include "./text_processing_lib/text_buf.h"

static int RunBin     (const char * in_fname);

static int DivideInts (int numerator, int denominator);

int main() {

    fprintf(stdout, "\n"
                    "# Processor by NeTort, 2023\n"
                    "# Does stuff... What else can i say?\n"
                    "# Today i ve accidentially skipped descrete analisys seminar. Im such a morron.\n"
                    "# On the other hand i have coded this app faster...\n\n");

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
    size_t n_cmds = 0;
    fread(&n_cmds, sizeof(n_cmds), 1, in_file);

    // read byte code array: form and fill prog_code array
    int * prog_code = (int *) calloc(n_cmds, sizeof(int));
    assert(prog_code);
    int * const prog_code_init = prog_code;

    size_t readen = 0;
    readen = fread(prog_code, sizeof(int), n_cmds, in_file);
    assert(readen == n_cmds);

    fclose(in_file);

    POP_OUT pop_err = POP_NO_ERR;
    int val     = 0;
    int in_var  = 0;

    for (size_t ip = 0; ip < n_cmds; ip++) {

        switch (*prog_code) {

            case CMD_HLT:
                {

                fprintf(stderr, "# Hlt encountered, goodbye!\n");

                free(prog_code_init);
                DtorStack(&my_spu.stk);

                return 0;

                }

            case ARG_IMMED_VAL | CMD_PUSH:
                {

                fprintf(stderr, "# Push imm val\n");

                prog_code++;
                ip++;
                val = *prog_code;

                PushStack(&my_spu.stk, val);

                break;
                }

            case ARG_REGTR_VAL | CMD_PUSH:
                {

                fprintf(stderr, "# Push from register\n");

                prog_code++;
                ip++;
                val = *prog_code;

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

                prog_code++;
                ip++;
                val = *prog_code;

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

            default:
                {

                fprintf(stderr, "# Syntax Error! No command \"%d\" found! Bye bye looser!\n", *prog_code);

                return 1;

                }
        }

        val     = 0;
        in_var  = 0;

        prog_code++;
    }

    free(prog_code_init);
    DtorStack(&my_spu.stk);

    return 0;
}

 int DivideInts(int numerator, int denominator) {
    return (int) numerator / denominator;
 }
