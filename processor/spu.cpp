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

const int SHOW_INTERMED_INFO = 1;
#define PRINTF_INTERMED_INFO(format, ...)     \
    if (SHOW_INTERMED_INFO)                   \
        fprintf(stderr, format, __VA_ARGS__); \

static size_t    ReadByteCode   (const char * in_fname, cmd_code_t ** prog_code);
static RunBinRes RunBin         (const cmd_code_t * prog_code, size_t n_bytes, SPU * spu);

static int       SPUCtor        (SPU * spu, int stack_capacity, int call_stack_capacity, int ram_size);
static int       SPUDtor        (SPU * spu);

static Elem_t    GetPushArg     (const cmd_code_t * prog_code, size_t ip, Elem_t gp_regs[], Elem_t RAM[]);
static Elem_t *  GetPopArgAddr  (const cmd_code_t * prog_code, size_t ip, Elem_t gp_regs[], Elem_t RAM[]);

static int       CalcIpOffset   (cmd_code_t cmd);
static int       CmdCodeIsValid (cmd_code_t cmd);

static Elem_t    PopCmpTopStack (stack * stk_ptr);
static Elem_t    DivideInts     (Elem_t numerator, Elem_t denominator);
static Elem_t    MultInts       (Elem_t frst, Elem_t scnd);

int main(int argc, char * argv[]) {

    fprintf(stdout, "\n"
                    "# Processor by NeTort, 2023\n"
                    "# Does stuff... What else can i say?\n\n");

    SPU my_spu = {};
    SPUCtor(&my_spu, SPU_STK_CAPA, SPU_CALL_STK_CAPA, SPU_RAM_WIDTH * SPU_RAM_HIGHT);

    cmd_code_t * prog_code = NULL;
    size_t n_bytes = ReadByteCode(BIN_FILENAME, &prog_code);

    RunBinRes run_res = RunBin((const cmd_code_t *) prog_code, n_bytes, &my_spu);

    free(prog_code);
    SPUDtor(&my_spu);

    if (run_res != REACH_END &&
        run_res != REACH_HLT)
    {
        fprintf(stderr, "RunBin cant finish its work due to an unexpected error (%d)!\n", run_res);

        return 1;
    }

    return 0;
}

size_t ReadByteCode (const char * in_fname, cmd_code_t ** prog_code)
{
    assert(in_fname);
    assert(prog_code);

    FILE * in_file = fopen(in_fname, "rb");

    // read size of the long long byte code array
    size_t n_bytes = 0;
    fread(&n_bytes, sizeof(size_t), 1, in_file);

    // read byte code array: form and fill prog_code array
    *prog_code = (cmd_code_t *) calloc(n_bytes, sizeof(cmd_code_t));
    assert(*prog_code);

    size_t readen = 0;
    readen = fread(*prog_code, sizeof(cmd_code_t), n_bytes, in_file);
    assert(readen == n_bytes);

    fclose(in_file);

    return n_bytes;
}

RunBinRes RunBin (const cmd_code_t * prog_code, size_t n_bytes, SPU * spu)
{
    assert(prog_code);
    assert(spu);

    POP_OUT pop_err = POP_NO_ERR;

    cmd_code_t cmd  = 0;
    Elem_t     val  = 0;

    size_t ip = 0;
    size_t ip_init = 0;

    while (ip < n_bytes)
    {
        ip_init = ip;

        cmd = prog_code[ip];

        switch (cmd & OPCODE_MSK)
        {
            case CMD_HLT:
            {
                PRINTF_INTERMED_INFO("# (%s - %3d) Hlt encountered, goodbye!\n", "proc");

                return REACH_HLT;
            }

            case CMD_PUSH:
            {
                Elem_t arg = GetPushArg(prog_code, ip, spu->gp_regs, spu->RAM);

                PushStack(&spu->stk, arg);

                ip += CalcIpOffset(cmd);

                PRINTF_INTERMED_INFO("# (%s - %3d) Push GetArg -> %d\n", "proc", ip_init, arg);

                break;
            }

            case CMD_POP:
            {
                if (cmd & ARG_TYPE_MSK)
                {
                    Elem_t * arg_ptr = GetPopArgAddr(prog_code, ip, spu->gp_regs, spu->RAM);

                    if (arg_ptr == NULL)
                    {
                        fprintf(stderr, "Processor Error! SetArg couldnt return stuff\n");
                        abort();
                    }

                    pop_err = POP_NO_ERR;
                    *arg_ptr = PopStack(&spu->stk, &pop_err);

                    ip += CalcIpOffset(cmd);

                    PRINTF_INTERMED_INFO("# (%s - %3d) Pop number to %p\n", "proc", ip_init, arg_ptr);
                }
                else
                {
                    pop_err = POP_NO_ERR;
                    PopStack(&spu->stk, &pop_err);

                    PRINTF_INTERMED_INFO("# (%s - %3d) Pop number\n", "proc", ip_init);

                    ip += CalcIpOffset(cmd);
                }

                break;
            }

            case CMD_IN:
            {
                fprintf(stdout, "\n>> ");

                fscanf(stdin, "%d", &val);

                val *= STK_PRECISION;

                PushStack(&spu->stk, val);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_OUT:
            {
                pop_err = POP_NO_ERR;

                val = PopStack(&spu->stk, &pop_err);

                fprintf(stdout, "\n<< %.2f\n", (float) val / STK_PRECISION);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_ADD:
            {
                // PUSH(POP() + POP())
                pop_err = POP_NO_ERR;
                val = 0;
                val = PopStack(&spu->stk, &pop_err) + PopStack(&spu->stk, &pop_err);
                PushStack(&spu->stk, val);

                PRINTF_INTERMED_INFO("# (%s - %3d) Add: %d\n", "proc", ip_init, val);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_SUB:
            {
                pop_err = POP_NO_ERR;
                val = 0;
                val -= PopStack(&spu->stk, &pop_err);
                val += PopStack(&spu->stk, &pop_err);
                PushStack(&spu->stk, val);

                PRINTF_INTERMED_INFO("# (%s - %3d) Sub: %d\n", "proc", ip_init, val);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_MUL:
            {
                pop_err = POP_NO_ERR;
                val = 0;
                val = MultInts(PopStack(&spu->stk, &pop_err), PopStack(&spu->stk, &pop_err));

                PRINTF_INTERMED_INFO("# (%s - %3d) Mul: %d\n", "proc", ip_init, val);

                PushStack(&spu->stk, val);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_DIV:
            {
                pop_err = POP_NO_ERR;

                Elem_t denominator = PopStack(&spu->stk, &pop_err);
                Elem_t numerator   = PopStack(&spu->stk, &pop_err);

                val = 0;
                val = DivideInts(numerator, denominator);

                PushStack(&spu->stk, val);

                PRINTF_INTERMED_INFO("# (%s - %3d) Div: %d\n", "proc", ip_init, val);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_SQRT:
            {
                pop_err = POP_NO_ERR;
                val = 0;
                val = PopStack(&spu->stk, &pop_err);

                val = (int) sqrt(val * STK_PRECISION);

                PRINTF_INTERMED_INFO("# (%s - %3d) Sqrt: %d\n", "proc", ip_init, val);

                PushStack(&spu->stk, val);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_SQR:
            {
                pop_err = POP_NO_ERR;
                val = 0;
                val = PopStack(&spu->stk, &pop_err);

                val = val * val / STK_PRECISION;

                PRINTF_INTERMED_INFO("# (%s - %3d) Sqr: %d\n", "proc", ip_init, val);

                PushStack(&spu->stk, val);

                ip += CalcIpOffset(cmd);

                break;
            }

            case CMD_CALL:
            {
                PushStack(&spu->call_stk, (Elem_t)(ip + sizeof(cmd_code_t) + sizeof(Elem_t)));

                ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                PRINTF_INTERMED_INFO("# (%s - %3d) Call to %lu\n", "proc", ip_init, ip);

                break;
            }

            case CMD_RET:
            {
                pop_err = POP_NO_ERR;
                ip = PopStack(&spu->call_stk, &pop_err);

                PRINTF_INTERMED_INFO("# (%s - %3d) Ret to %lu\n", "proc", ip_init, ip);

                break;
            }

            case CMD_JMP:
            {
                ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                PRINTF_INTERMED_INFO("# (%s - %3d) Jmp to %lu\n", "proc", ip_init, ip);

                break;
            }
            case CMD_JA:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res > 0)
                {
                    ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                    PRINTF_INTERMED_INFO("# (%s - %3d) Jmp to %lu\n", "proc", ip_init, ip);
                }
                else
                {
                    ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                }

                break;
            }
            case CMD_JAE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res >= 0)
                {
                    ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                    PRINTF_INTERMED_INFO("# (%s - %3d) Jmp to %lu\n", "proc", ip_init, ip);
                }
                else
                {
                    ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JB:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res < 0)
                {
                    ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                    PRINTF_INTERMED_INFO("# (%s - %3d) Jmp to %lu\n", "proc", ip_init, ip);
                }
                else
                {
                    ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JBE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res <= 0)
                {
                    ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                    PRINTF_INTERMED_INFO("# (%s - %3d) Jmp to %lu\n", "proc", ip_init, ip);
                }
                else
                {
                    ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res == 0)
                {
                    ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                    PRINTF_INTERMED_INFO("# (%s - %3d) Jmp to %lu\n", "proc", ip_init, ip);
                }
                else
                {
                    ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                }

                break;
            }

            case CMD_JNE:
            {
                int cmp_res = PopCmpTopStack(&spu->stk);

                if (cmp_res != 0)
                {
                    ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                    PRINTF_INTERMED_INFO("# (%s - %3d) Jmp to %lu\n", "proc", ip_init, ip);
                }
                else
                {
                    ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                }

                break;
            }

            default:
            {
                fprintf(stderr, "SIGILL! Illegal instruction \"%d\" (%lu)\n", cmd, ip);

                return ILL_CDMCODE;
            }
        }
        val = 0;
    }

    return REACH_END;
}

Elem_t GetPushArg (const cmd_code_t * prog_code, size_t ip, Elem_t gp_regs[], Elem_t RAM[])
{
    assert(prog_code);
    assert(gp_regs);
    assert(RAM);

    cmd_code_t cmd = 0;
    memcpy(&cmd, (prog_code + ip), sizeof(cmd_code_t));

    if (!CmdCodeIsValid(cmd))
    {
        fprintf(stderr, "Forbidden command code %d\n", cmd);
        abort();
    }

    ip += sizeof(cmd_code_t);

    Elem_t res     = 0;
    Elem_t tmp_res = 0;

    if (cmd & ARG_IMMED_VAL)
    {
        memcpy(&tmp_res, prog_code + ip, sizeof(Elem_t));
        res += tmp_res * STK_PRECISION; // we multiply only here because in other cases values in ram and in regs are allready multiplied

        tmp_res = 0;

        ip += sizeof(Elem_t);
    }

    if (cmd & ARG_REGTR_VAL)
    {
        memcpy(&tmp_res, prog_code + ip, sizeof(cmd_code_t));
        res += gp_regs[tmp_res]; // no precision operations as in registers all values are already multiplied by precision

        tmp_res = 0;

        ip += sizeof(cmd_code_t);
    }

    if (cmd & ARG_MEMRY_VAL)
    {
        res = RAM[res];         // no precision operations as in ram all values are already multiplied by precision
        sleep(2);
    }

    return res;
}

Elem_t * GetPopArgAddr (const cmd_code_t * prog_code, size_t ip, Elem_t gp_regs[], Elem_t RAM[])
{
    assert(prog_code);
    assert(gp_regs);
    assert(RAM);

    cmd_code_t cmd = 0;
    memcpy(&cmd, prog_code + ip, sizeof(cmd_code_t));

    if (!CmdCodeIsValid(cmd))
    {
        fprintf(stderr, "Forbidden command code %d\n", cmd);
        abort();
    }

    ip += sizeof(cmd_code_t);

    Elem_t tmp_res = 0;
    Elem_t imm_storage = 0;

    Elem_t * ram_ptr = NULL;
    Elem_t * reg_ptr = NULL;

    if (cmd & ARG_IMMED_VAL)
    {
        memcpy(&tmp_res, prog_code + ip, sizeof(Elem_t));
        imm_storage = tmp_res;

        ip += sizeof(Elem_t);
    }
    if (cmd & ARG_REGTR_VAL)
    {
        memcpy(&tmp_res, prog_code + ip, sizeof(cmd_code_t));
        imm_storage += gp_regs[tmp_res];
        reg_ptr = &gp_regs[tmp_res];

        ip += sizeof(cmd_code_t);
    }
    if (cmd & ARG_MEMRY_VAL)
    {
        ram_ptr = &RAM[imm_storage];

        return ram_ptr;
    }

    return reg_ptr;
}

static int CalcIpOffset (cmd_code_t cmd)
{
    int offset = sizeof(cmd_code_t);

    if (cmd & ARG_IMMED_VAL)
        offset += sizeof(Elem_t);

    if (cmd & ARG_REGTR_VAL)
        offset += sizeof(cmd_code_t);

    return offset;
}

static int CmdCodeIsValid (cmd_code_t cmd)
{
    if ((cmd & OPCODE_MSK) == CMD_POP)
    {
        if ((cmd & ARG_IMMED_VAL) && (cmd & ARG_REGTR_VAL))
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

static Elem_t MultInts (Elem_t frst, Elem_t scnd)
{
    return frst * scnd / STK_PRECISION;
}
 Elem_t DivideInts(Elem_t numerator, int denominator)
 {
    return (Elem_t) ( (float) numerator / (float) denominator * STK_PRECISION);
 }
