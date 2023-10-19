#include <stdio.h>
#include <iostream>
#include <string.h>

#include "asm.h"
#include "spu.h"
#include "stack.h"
#include "text_buf.h"

const char * BIN_FILENAME    = "translated.bin";
const char * DISASM_FILENAME = "disasmed.txt";
const int    MAX_CMD_CODE    = 8;

int RunBin     (const char * in_fname);

int DivideInts (int numerator, int denominator);

int main() {

    AssembleMath("ex1.txt", "ex1_translated.txt", "user_commands.txt"); // TODO temporary decision

    RunBin(BIN_FILENAME);
    DisAssemble(BIN_FILENAME, DISASM_FILENAME);

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
    int prog_code[n_cmds] = {};

    size_t readen = 0;
    readen = fread(prog_code, sizeof(int), n_cmds, in_file);
    assert(readen == n_cmds);

    fclose(in_file);

    POP_OUT pop_err = POP_NO_ERR;
    int val     = 0;
    int in_var  = 0;

    for (int ip = 0; ip < n_cmds; ip++) {

        switch (prog_code[ip]) {

            case CMD_HLT:
                {

                fprintf(stderr, "hlt encountered, goodbye!\n");

                DtorStack(&my_spu.stk);

                return 0;

                }

            case ARG_IMMED_VAL | CMD_PUSH:
                {

                fprintf(stderr, "Push imm val\n");

                ip++;
                val = prog_code[ip];

                PushStack(&my_spu.stk, val);

                break;
                }

            case ARG_REGTR_VAL | CMD_PUSH:
                {

                fprintf(stderr, "Push from register\n");

                ip++;
                val = prog_code[ip];

                PushStack(&my_spu.stk, my_spu.regs[val]);

                val = 0;

                break;

                }

            case CMD_POP:
                {

                fprintf(stderr, "Pop immediate number\n");

                pop_err = POP_NO_ERR;
                PopStack(&my_spu.stk, &pop_err);

                break;

                }

            case ARG_REGTR_VAL | CMD_POP:
                {

                fprintf(stderr, "Pop to register\n");
                pop_err = POP_NO_ERR;

                ip++;
                val = prog_code[ip];

                my_spu.regs[val] = PopStack(&my_spu.stk, &pop_err);

                break;

                }

            case CMD_IN:
                {

                fprintf(stderr, "In: please enter your variable...\n");

                in_var = 0;
                fscanf(stdin, "%d", &in_var);

                PushStack(&my_spu.stk, in_var);

                break;

                }

            case CMD_OUT:
                {

                pop_err = POP_NO_ERR;
                fprintf(stderr, "Out:   %d\n", PopStack(&my_spu.stk, &pop_err));

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

                fprintf(stderr, "Syntax Error! No command \"%s\" found! Bye bye looser!\n", *prog_code);

                return 1;

                }
        }

        val     = 0;
        in_var  = 0;

    }

    DtorStack(&my_spu.stk);

    return 0;
}


int *TokenizeCmdIntArr (char **asm_lines_raw, size_t n_instructs) {

    stack asm_codes = {};
    CtorStack(&asm_codes, n_instructs * 3);

    int n_words = 0; // i mean integer numbers (cmd codes or cmd args)

    char * asm_line_cpy = (char *) calloc(MAX_CMD_CODE, 1);

    for (int i = 0; i < n_instructs; i++) {

        strcpy(asm_line_cpy, asm_lines_raw[i]);

        char * token = strtok(asm_line_cpy, " ");

        while(token != NULL) {
            n_words++;
            PushStack(&asm_codes, atoi(token));

            token = strtok(NULL, " ");
        }
    }
    free(asm_line_cpy);

    int *asm_lines = (int *) calloc(n_words, sizeof(int));

    for (int i = 0; i < n_words; i++) {
        asm_lines[i] = asm_codes.data.buf[i];
    }

    DtorStack(&asm_codes);

    return asm_lines;
 }

 int DivideInts(int numerator, int denominator) {
    return (int) numerator / denominator;
 }
