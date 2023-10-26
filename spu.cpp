#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "spu.h"
#include "./stacklib/stack.h"
#include "./text_processing_lib/text_buf.h"

static int RunBin     (const char * in_fname);
static int SPUCtor    (SPU * spu, int stack_capacity, int regs_num, int ram_size);
static int SPUDtor    (SPU * spu);
static int GetArg     (const char * prog_code, size_t * ip, int regs[], int RAM[]);

static Elem_t PopCmpTopStack (stack * stk_ptr);
static int    DivideInts     (int numerator, int denominator);

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

    SPUCtor(&my_spu, SPU_STK_CAPA, SPU_REGS_NUM, SPU_RAM_SIZE);

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

    for (size_t ip = 0; ip < n_bytes; ip += sizeof(char)) {

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
                ip += sizeof(char);

                val = 0;
                memcpy(&val, (prog_code + ip), sizeof(int));

                ip += 3;

                PushStack(&my_spu.stk, val);

                fprintf(stderr, "# Push imm val (%d)\n", val);

                val = 0;

                break;
            }

            case ARG_REGTR_VAL | CMD_PUSH:
            {
                ip += sizeof(char);

                val = 0;
                memcpy(&val, (prog_code + ip), sizeof(char));

                PushStack(&my_spu.stk, my_spu.regs[val]);

                fprintf(stderr, "# Push from register (reg_id = %d)\n", val);

                val = 0;

                break;
            }

            case ARG_MEMRY_VAL | CMD_PUSH:
            {
                //todo
            }
            // case CMD_PUSH: // fresh fucntion using GetArg
            // {
            //     int arg = GetArg(prog_code, &ip, my_spu.regs, my_spu.RAM);
            //     PushStack(&my_spu.stk, arg);
            //     fprintf(stderr, "# Push GetArg -> %d\n", arg);
            // }

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

                ip += sizeof(char);
                val = 0;
                memcpy(&val, (prog_code + ip), sizeof(char));

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

                fprintf(stderr, "# add: %d\n", val);

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
                val = PopStack(&my_spu.stk, &pop_err) * PopStack(&my_spu.stk, &pop_err);

                fprintf(stderr, "# mul: %d\n", val);

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
                memcpy(&ip, (prog_code + ip + sizeof(char)), sizeof(int));

                fprintf(stderr, "# Jmp to %lu\n", ip);

                ip -= sizeof(char); // because ip is to be increased in for-statement

                break;
            }

            case ARG_IMMED_VAL | CMD_JA:
            {
                int cmp_res = PopCmpTopStack(&my_spu.stk);

                if (cmp_res > 0)
                {
                    ip += sizeof(char);

                    memcpy(&ip, (prog_code + ip), sizeof(int));

                    fprintf(stderr, "# Jmp to %lu\n", ip);

                    ip -= sizeof(char); // because ip is to be increased in for-statement
                }
                else
                {
                    ip += sizeof(int); // skip integer pointer to a position in code
                }

                break;
            }

            case ARG_IMMED_VAL | CMD_JAE:
            {
                int cmp_res = PopCmpTopStack(&my_spu.stk);

                if (cmp_res >= 0)
                {
                    ip += sizeof(char);

                    memcpy(&ip, (prog_code + ip), sizeof(int));

                    fprintf(stderr, "# Jmp to %lu\n", ip);

                    ip -= sizeof(char); // because ip is to be increased in for-statement
                }
                else
                {
                    ip += sizeof(int); // skip integer pointer to a position in code
                }

                break;
            }

            case ARG_IMMED_VAL | CMD_JB:
            {
                int cmp_res = PopCmpTopStack(&my_spu.stk);

                if (cmp_res < 0)
                {
                    ip += sizeof(char);

                    memcpy(&ip, (prog_code + ip), sizeof(int));

                    fprintf(stderr, "# Jmp to %lu\n", ip);

                    ip -= sizeof(char); // because ip is to be increased in for-statement
                }
                else
                {
                    ip += sizeof(int); // skip integer pointer to a position in code
                }

                break;
            }

            case ARG_IMMED_VAL | CMD_JBE:
            {
                int cmp_res = PopCmpTopStack(&my_spu.stk);

                if (cmp_res <= 0)
                {
                    ip += sizeof(char);

                    memcpy(&ip, (prog_code + ip), sizeof(int));

                    fprintf(stderr, "# Jmp to %lu\n", ip);

                    ip -= sizeof(char); // because ip is to be increased in for-statement
                }
                else
                {
                    ip += sizeof(int); // skip integer pointer to a position in code
                }

                break;
            }

            case ARG_IMMED_VAL | CMD_JE:
            {
                int cmp_res = PopCmpTopStack(&my_spu.stk);

                if (cmp_res == 0)
                {
                    ip += sizeof(char);

                    memcpy(&ip, (prog_code + ip), sizeof(int));

                    fprintf(stderr, "# Jmp to %lu\n", ip);

                    ip -= sizeof(char); // because ip is to be increased in for-statement
                }
                else
                {
                    ip += sizeof(int); // skip integer pointer to a position in code
                }

                break;
            }

            case ARG_IMMED_VAL | CMD_JNE:
            {
                int cmp_res = PopCmpTopStack(&my_spu.stk);

                if (cmp_res != 0)
                {
                    ip += sizeof(char);

                    memcpy(&ip, (prog_code + ip), sizeof(int));

                    fprintf(stderr, "# Jmp to %lu\n", ip);

                    ip -= sizeof(char); // because ip is to be increased in for-statement
                }
                else
                {
                    ip += sizeof(int); // skip integer pointer to a position in code
                }

                break;
            }

            case ARG_IMMED_VAL | CMD_JF:
            {
                // todo jump on friday
                if (0)
                {
                    ip += sizeof(char);

                    memcpy(&ip, (prog_code + ip), sizeof(int));

                    fprintf(stderr, "# Jmp to %lu\n", ip);

                    ip -= sizeof(char); // because ip is to be increased in for-statement
                }
                else
                {
                    ip += sizeof(int); // skip integer pointer to a position in code
                }

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
    SPUDtor(&my_spu);

    return 0;
}

//! didnot debug
int GetArg (const char * prog_code, size_t * ip, int regs[], int RAM[])
{
    assert(prog_code);
    assert(ip);
    assert(regs);
    assert(RAM);

    char cmd = 0;
    memcpy(&cmd, (prog_code + *ip), sizeof(char));

    (*ip) += sizeof(char);

    int res     = 0;
    int tmp_res = 0;

    if (cmd & ARG_IMMED_VAL)
    {
        memcpy(&tmp_res, (prog_code + *ip), sizeof(int));
        res += tmp_res;

        tmp_res = 0;

        (*ip) += sizeof(int);
    }

    if (cmd & ARG_REGTR_VAL)
    {
        memcpy(&tmp_res, (prog_code + *ip), sizeof(char));
        res += regs[tmp_res];

        tmp_res = 0;

        (*ip) += sizeof(char);
    }

    if (cmd & ARG_MEMRY_VAL)
    {
        res = RAM[res];
    }

    return res;
}

int SPUCtor (SPU * spu, int stack_capacity, int regs_num, int ram_size)
{
    assert(spu);

    if (CtorStack(&(spu->stk), SPU_STK_CAPA) != CTOR_NO_ERR)
    {
        fprintf(stderr, "Stack Constructor returned error (TODO TODO TODO)\n"); // todo
        abort();                                                                // aborting is justified because stack is the "kernel" of spu
    }

    spu->RAM = (int *) calloc(SPU_RAM_SIZE, sizeof(int)); //? why does memory leak
    if (/*validateptr*/spu->RAM == NULL) // todo pointer validator
    {
        fprintf(stderr, "Unable to allocate memory for RAM\n");
        abort();                        // todo return enum. What if we dont use RAM in asm code? program shouldnt fall
    }

    return 0;
}

int SPUDtor (SPU * spu)
{
    assert(spu);

    DtorStack(&spu->stk);

    memset(spu->RAM, 0xcc, SPU_RAM_SIZE * sizeof(int));
    free(spu->RAM);
}

Elem_t PopCmpTopStack(stack * stk_ptr) {

    Elem_t val_top = 0;
    Elem_t val_below = 0;

    enum POP_OUT pop_err = POP_NO_ERR;

    val_top = PopStack(stk_ptr, &pop_err);

    if (pop_err != POP_NO_ERR) {
        fprintf(stderr, "Stack Error!\n");
        abort();
    }

    val_below = PopStack(stk_ptr, &pop_err);

    if (pop_err != POP_NO_ERR) {
        fprintf(stderr, "Stack Error!\n");
        abort();
    }

    return val_below - val_top;
}

 int DivideInts(int numerator, int denominator) {
    return (int) numerator / denominator;
 }
