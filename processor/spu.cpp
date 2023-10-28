#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "../commands.h"
#include "spu.h"
#include "../stacklib/stack.h"
#include "../text_processing_lib/text_buf.h"

const int SHOW_INTERMED_INFO = 0;
#define PRINTF_INTERMED_INFO(format, ...)     \
    if (SHOW_INTERMED_INFO)                   \
        fprintf(stderr, format, __VA_ARGS__); \

static size_t        ReadByteCode (const char * in_fname, char ** prog_code);
static RunBinRes     RunBin       (const char * prog_code, size_t n_bytes, SPU * spu);
static FinishWorkRes FinishWork   (char * prog_code, SPU * spu);

static int SPUCtor (SPU * spu, int stack_capacity, int call_stack_capacity, int ram_size);
static int SPUDtor (SPU * spu);

static int   GetArg   (const char * prog_code, size_t ip, int gp_regs[], int RAM[]);
static int * SetArg   (const char * prog_code, size_t ip, int gp_regs[], int RAM[]);
static int   CalcIpOffset   (char cmd);
static int   CmdCodeIsValid (char code);

static Elem_t PopCmpTopStack (stack * stk_ptr);
static int    DivideInts (int numerator, int denominator);
static int    MultInts   (int frst, int scnd);

int main(int argc, char * argv[]) {

    fprintf(stdout, "\n"
                    "# Processor by NeTort, 2023\n"
                    "# Does stuff... What else can i say?\n\n");

    SPU my_spu = {};
    SPUCtor(&my_spu, SPU_STK_CAPA, SPU_STK_CAPA, SPU_RAM_WIDTH * SPU_RAM_HIGHT);

    char * prog_code = NULL;
    size_t n_bytes = ReadByteCode(BIN_FILENAME, &prog_code);

    RunBinRes run_res = RunBin((const char *) prog_code, n_bytes, &my_spu);

    FinishWork(prog_code, &my_spu);

    if ((run_res != REACH_END) &&
        (run_res != REACH_HLT))
    {
        fprintf(stderr, "RunBin cant finish its work due to an unexpected error (%d)!\n", run_res);
        return 1;
    }

    return 0;
}

size_t ReadByteCode (const char * in_fname, char ** prog_code)
{
    assert(in_fname);
    assert(prog_code);

    FILE * in_file = fopen(in_fname, "rb");

    // read size of the long long byte code array
    size_t n_bytes = 0;
    fread(&n_bytes, sizeof(size_t), 1, in_file);

    // read byte code array: form and fill prog_code array
    *prog_code = (char *) calloc(n_bytes, sizeof(char));
    assert(*prog_code);

    size_t readen = 0;
    readen = fread(*prog_code, sizeof(char), n_bytes, in_file);
    assert(readen == n_bytes);

    fclose(in_file);

    return n_bytes;
}

FinishWorkRes FinishWork (char * prog_code, SPU * spu)
{
    assert(prog_code);
    assert(spu);

    free(prog_code);
    SPUDtor(spu);

    return FinishWorkRes::ALL_OK;
}

RunBinRes RunBin (const char * prog_code, size_t n_bytes, SPU * spu)
{
    assert(prog_code);
    assert(spu);

    POP_OUT pop_err = POP_NO_ERR;
    int val   = 0;
    size_t ip = 0;

    while (ip < n_bytes)
    {
        switch (prog_code[ip] & OPCODE_MSK)
        {
            case CMD_HLT:
            {
                PRINTF_INTERMED_INFO("# (%s) Hlt encountered, goodbye!\n", "proc");

                return RunBinRes::REACH_HLT;
            }

            case CMD_PUSH:
            {
                int arg = GetArg(prog_code, ip, spu->gp_regs, spu->RAM);

                PushStack(&spu->stk, arg);

                ip += CalcIpOffset(prog_code[ip]);

                PRINTF_INTERMED_INFO("# (%s) Push GetArg -> %d\n", "proc", arg);

                break;
            }

            case CMD_POP:
            {
                if (prog_code[ip] & ARG_TYPE_MSK)
                {
                    int * arg_ptr = SetArg(prog_code, ip, spu->gp_regs, spu->RAM);

                    if (arg_ptr == NULL)
                    {
                        fprintf(stderr, "Processor Error! SetArg couldnt return stuff\n");
                        abort();
                    }

                    pop_err = POP_NO_ERR;
                    *arg_ptr = PopStack(&spu->stk, &pop_err);

                    ip += CalcIpOffset(prog_code[ip]);

                    PRINTF_INTERMED_INFO("# (%s) Pop number to %p\n", "proc", arg_ptr);
                }
                else
                {
                    pop_err = POP_NO_ERR;
                    PopStack(&spu->stk, &pop_err);

                    PRINTF_INTERMED_INFO("# (%s) Pop number\n", "proc");

                    ip += CalcIpOffset(prog_code[ip]);
                }

                break;
            }

            case CMD_IN:
            {
                fprintf(stdout, "\n>> ");

                fscanf(stdin, "%d", &val);

                val *= STK_PRECISION;

                PushStack(&spu->stk, val);

                ip += CalcIpOffset(prog_code[ip]);

                break;
            }

            case CMD_OUT:
            {
                pop_err = POP_NO_ERR;

                val = PopStack(&spu->stk, &pop_err);

                fprintf(stdout, "\n<< %.2f\n", (float) val / STK_PRECISION);

                ip += CalcIpOffset(prog_code[ip]);

                break;
            }

            case CMD_ADD:
            {
                // PUSH(POP() + POP())
                pop_err = POP_NO_ERR;
                val = 0;
                val = PopStack(&spu->stk, &pop_err) + PopStack(&spu->stk, &pop_err);
                PushStack(&spu->stk, val);

                PRINTF_INTERMED_INFO("# (%s) Add: %d\n", "proc", val);

                ip += CalcIpOffset(prog_code[ip]);

                break;
            }

            case CMD_SUB:
            {
                pop_err = POP_NO_ERR;
                val = 0;
                val -= PopStack(&spu->stk, &pop_err);
                val += PopStack(&spu->stk, &pop_err);
                PushStack(&spu->stk, val);

                PRINTF_INTERMED_INFO("# (%s) Sub: %d\n", "proc", val);

                ip += CalcIpOffset(prog_code[ip]);

                break;
            }

            case CMD_MUL:
            {
                pop_err = POP_NO_ERR;
                val = 0;
                val = MultInts(PopStack(&spu->stk, &pop_err), PopStack(&spu->stk, &pop_err));

                PRINTF_INTERMED_INFO("# (%s) Mul: %d\n", "proc", val);

                PushStack(&spu->stk, val);

                ip += CalcIpOffset(prog_code[ip]);

                break;
            }

            case CMD_SQRT:
            {
                pop_err = POP_NO_ERR;
                val = 0;
                val = PopStack(&spu->stk, &pop_err);

                val = (int) sqrt(val * STK_PRECISION);

                PRINTF_INTERMED_INFO("# (%s) Sqrt: %d\n", "proc", val);

                PushStack(&spu->stk, val);

                ip += CalcIpOffset(prog_code[ip]);

                break;
            }

            case CMD_DIV:
            {
                pop_err = POP_NO_ERR;

                int denominator = PopStack(&spu->stk, &pop_err);
                int numerator   = PopStack(&spu->stk, &pop_err);

                val = 0;
                val = DivideInts(numerator, denominator);

                PushStack(&spu->stk, val);

                PRINTF_INTERMED_INFO("# (%s) Div: %d\n", "proc", val);

                ip += CalcIpOffset(prog_code[ip]);

                break;
            }

            case CMD_CALL:
            {
                PushStack(&spu->call_stk, (Elem_t)(ip + sizeof(char) + sizeof(int)));

                memcpy(&ip, (prog_code + ip + sizeof(char)), sizeof(int));

                PRINTF_INTERMED_INFO("# (%s) Call to %lu\n", "proc", ip);

                break;
            }

            case CMD_RET:
            {
                pop_err = POP_NO_ERR;
                ip = PopStack(&spu->call_stk, &pop_err);

                PRINTF_INTERMED_INFO("# (%s) Ret to %lu\n", "proc", ip);

                break;
            }

            case CMD_JMP:
            {
                memcpy(&ip, (prog_code + ip + 1), sizeof(int));

                PRINTF_INTERMED_INFO("# (%s) Jmp to %lu\n", "proc", ip);

                break;
            }
            case CMD_JA:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res > 0)
                {
                    memcpy(&ip, (prog_code + ip + 1), sizeof(int));

                    PRINTF_INTERMED_INFO("# (%s) Jmp to %lu\n", "proc", ip);
                }
                else
                {
                    ip += CalcIpOffset(prog_code[ip]); // skip integer pointer to a position in code
                }

                break;
            }
            case CMD_JAE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res >= 0)
                {
                    memcpy(&ip, (prog_code + ip + 1), sizeof(int));

                    PRINTF_INTERMED_INFO("# (%s) Jmp to %lu\n", "proc", ip);
                }
                else
                {
                    ip += CalcIpOffset(prog_code[ip]); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JB:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res < 0)
                {
                    memcpy(&ip, (prog_code + ip + 1), sizeof(int));

                    PRINTF_INTERMED_INFO("# (%s) Jmp to %lu\n", "proc", ip);
                }
                else
                {
                    ip += CalcIpOffset(prog_code[ip]); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JBE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res <= 0)
                {
                    memcpy(&ip, (prog_code + ip + 1), sizeof(int));

                    PRINTF_INTERMED_INFO("# (%s) Jmp to %lu\n", "proc", ip);
                }
                else
                {
                    ip += CalcIpOffset(prog_code[ip]); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res == 0)
                {
                    memcpy(&ip, (prog_code + ip + 1), sizeof(int));

                    PRINTF_INTERMED_INFO("# (%s) Jmp to %lu\n", "proc", ip);
                }
                else
                {
                    ip += CalcIpOffset(prog_code[ip]); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JNE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res != 0)
                {
                    memcpy(&ip, (prog_code + ip + 1), sizeof(int));

                    PRINTF_INTERMED_INFO("# (%s) Jmp to %lu\n", "proc", ip);
                }
                else
                {
                    ip += CalcIpOffset(prog_code[ip]); // skip integer pointer to a position in code
                }

                break;
            }

            default:
            {
                fprintf(stderr, "Syntax Error! No command \"%d\" (%lu) found! Bye bye looser!\n", prog_code[ip], ip);

                return RunBinRes::OCC_ERROR;
            }
        }
        val = 0;
    }

    return RunBinRes::REACH_END;
}

int GetArg (const char * prog_code, size_t ip, int gp_regs[], int RAM[])
{
    assert(prog_code);
    assert(ip);
    assert(gp_regs);
    assert(RAM);

    char cmd = 0;
    memcpy(&cmd, (prog_code + ip), sizeof(char));

    ip += sizeof(char);

    int res     = 0;
    int tmp_res = 0;

    if (cmd & ARG_IMMED_VAL)
    {
        memcpy(&tmp_res, (prog_code + ip), sizeof(int));
        res += tmp_res * STK_PRECISION; // we multiply only here because in other cases values in ram and in regs are allready multiplied

        tmp_res = 0;

        ip += sizeof(int);
    }

    if (cmd & ARG_REGTR_VAL)
    {
        memcpy(&tmp_res, (prog_code + ip), sizeof(char));
        res += gp_regs[tmp_res]; // no precision operations as in registers all values are already multiplied by precision

        tmp_res = 0;

        ip += sizeof(char);
    }

    if (cmd & ARG_MEMRY_VAL)
    {
        res = RAM[res];         // no precision operations as in ram all values are already multiplied by precision
        sleep(2);
    }

    return res;
}

int * SetArg (const char * prog_code, size_t ip, int gp_regs[], int RAM[])
{

    assert(prog_code);
    assert(ip);
    assert(gp_regs);
    assert(RAM);

    char cmd = 0;
    memcpy(&cmd, (prog_code + ip), sizeof(char));

    if (!CmdCodeIsValid(cmd))
    {
        fprintf(stderr, "Forbidden command code %d\n", cmd);
        abort();
    }

    ip += sizeof(char);

    int tmp_res = 0;
    int imm_storage = 0;

    int * ram_ptr = NULL;
    int * reg_ptr = NULL;

    if (cmd & ARG_IMMED_VAL)
    {
        memcpy(&tmp_res, (prog_code + ip), sizeof(int));
        imm_storage = tmp_res;

        ip += sizeof(int);
    }
    if (cmd & ARG_REGTR_VAL)
    {
        memcpy(&tmp_res, (prog_code + ip), sizeof(char));
        imm_storage += gp_regs[tmp_res];
        reg_ptr = &gp_regs[tmp_res];

        ip += sizeof(char);
    }
    if (cmd & ARG_MEMRY_VAL)
    {
        ram_ptr = &RAM[imm_storage];

        return ram_ptr;
    }

    return reg_ptr;
}

static int CalcIpOffset (char cmd)
{
    int offset = sizeof(char);

    if (cmd & ARG_IMMED_VAL)
        offset += sizeof(int);

    if (cmd & ARG_REGTR_VAL)
        offset += sizeof(char);

    return offset;
}

static int CmdCodeIsValid (char code)
{
    if ((code & OPCODE_MSK) == CMD_POP)
    {
        if ((code & ARG_IMMED_VAL) && (code & ARG_REGTR_VAL))
        {
            return 0;
        }

    }

    return 1;
}

int SPUCtor (SPU * spu, int stack_capacity, int call_stack_capacity, int ram_size)
{
    assert(spu);

    if (CtorStack(&(spu->stk), stack_capacity) != CTOR_NO_ERR)
    {
        fprintf(stderr, "Stack Constructor returned error\n");
        abort();                                                    // aborting is justified
    }

    if (CtorStack(&(spu->call_stk), call_stack_capacity) != CTOR_NO_ERR)
    {
        fprintf(stderr, "Call-Stack Constructor returned error\n");
        abort();                                                    // aborting is justified
    }

    spu->RAM = (int *) calloc(ram_size, sizeof(int));
    if (spu->RAM == NULL) // todo pointer validator
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
    DtorStack(&spu->call_stk);

    free(spu->RAM);

    return 0;
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

    return val_top - val_below;
}

static int MultInts (int frst, int scnd)
{
    return frst * scnd / STK_PRECISION;
}
 int DivideInts(int numerator, int denominator)
 {
    return (int) ((float) numerator / denominator * STK_PRECISION);
 }
