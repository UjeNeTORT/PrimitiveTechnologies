#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "../enums.h"
#include "spu.h"
#include "../stacklib/stack.h"
#include "../text_processing_lib/text_buf.h"

/**
 * fill byte code array prog_code from "in_fname" file
*/
static size_t    ReadByteCode   (const char * in_fname, cmd_code_t ** prog_code);

/**
 * interpret instructions from prog_code array
*/
static RunBinRes RunBin         (const cmd_code_t * prog_code, size_t n_bytes, SPU * spu);

static int       SPUCtor        (SPU * spu, int stack_capacity, int call_stack_capacity, int ram_size);
static int       SPUDtor        (SPU * spu);

/**
 * given position of cmd push in byte code array get argument for this cmd
*/
static Elem_t    GetPushArg     (const cmd_code_t * prog_code, size_t ip, Elem_t gp_regs[], Elem_t RAM[]);

/**
 * given position of cmd pop in byte code array get an address with which pop is to interact
*/
static Elem_t *  GetPopArgAddr  (const cmd_code_t * prog_code, size_t ip, Elem_t gp_regs[], Elem_t RAM[]);

/**
 * print VRAM to show "picture"
*/
static int       ShowFrame      (SPU * spu);

/**
 * validate cmd index
*/
static int       CmdCodeIsValid (cmd_code_t cmd);

/**
 * pop val_top val_below from stack and return val_top - val_below
*/
static Elem_t    PopCmpTopStack (stack * stk_ptr);

/**
 * given position in bytecode array return offset for ip
*/
static int       CalcIpOffset   (cmd_code_t cmd);

/**
 * calculate numerator % denominator (precise)
*/
static Elem_t    CalcMod        (Elem_t numerator, Elem_t denominator);

/**
 * calculate (numerator - numerator % denominator) / denominator (precise)
*/
static Elem_t    CalcIdiv       (Elem_t numerator, Elem_t denominator);

/**
 * calculate numerator / denominator (precise)
*/
static Elem_t    DivideInts     (Elem_t numerator, Elem_t denominator);

/**
 * multiply (precise)
*/
static Elem_t    MultInts       (Elem_t frst, Elem_t scnd);

/**
 * former macros for console logging
*/
static int      printf_intermed_info (const char * format, ...) __attribute__(( format (printf, 1, 2) ));

int main(int argc, char * argv[])
{
    fprintf(stdout, "\n"
                    "# Processor by NeTort\n"
                    "# (c) TIKHONOV YAROSLAV 2023\n\n");

    char * bin_fname = NULL;

    for (int argn = 0; argn < argc; argn++)
    {
        if (strcmp(argv[argn], "--finname") == 0)
        {
            bin_fname = argv[argn + 1];
            argn++;
        }
    }

    if (!bin_fname)
    {
        fprintf(stderr, "No translated file given\n");
        abort();
    }

    SPU my_spu = {};
    SPUCtor(&my_spu, SPU_STK_CAPA, SPU_CALL_STK_CAPA, SPU_VRAM_WIDTH * SPU_VRAM_HIGHT + VRAM_MAPPING);

    cmd_code_t * prog_code = NULL;
    size_t n_bytes = ReadByteCode(bin_fname, &prog_code);

    fprintf(stdout, "Running...\n");

    RunBinRes run_res = RunBin((const cmd_code_t *) prog_code, n_bytes, &my_spu);

    free(prog_code);
    SPUDtor(&my_spu);

    if (run_res != REACH_END &&
        run_res != REACH_HLT)
    {
        fprintf(stderr, "RunBin: unexpected error (%d)!\n", run_res);

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
    if (fread(&n_bytes, sizeof(size_t), 1, in_file) != 1)
    {
        fprintf(stderr, "Presentation error: could not read size from byte code\n");
        abort();
    }

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
            #define DEF_CMD(name, num, text, have_arg, spu_code, ...)  \
                case CMD_##name:                                       \
                    {                                                  \
                        spu_code;                                      \
                        break;                                         \
                    }                                                  \

            #include "../commands.h"

            #undef DEF_CMD

            default:
            {
                fprintf(stderr, "Illegal instruction \"%d\" (%lu)\n", cmd, ip_init);

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
        res += tmp_res * STK_PRECISION;

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
        res = RAM[res / STK_PRECISION];         // no precision operations as in ram all values are already multiplied by precision
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
        imm_storage = tmp_res * STK_PRECISION;

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
        ram_ptr = &RAM[imm_storage / STK_PRECISION];

        return ram_ptr;
    }

    return reg_ptr;
}

int CalcIpOffset (cmd_code_t cmd)
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
        abort();
    }

    spu->RAM = (Elem_t *) calloc(ram_size, sizeof(Elem_t));
    if (spu->RAM == NULL)
    {
        fprintf(stderr, "Unable to allocate memory for RAM\n");
        abort();
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

Elem_t PopCmpTopStack(stack * stk_ptr)
{
    assert(stk_ptr);

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

static int ShowFrame(SPU * spu)
{
    assert(spu);

    for (int i = 0; i < SPU_VRAM_HIGHT; i++, printf("\n"))
    {
        for (int j = 0; j < SPU_VRAM_WIDTH; j++)
        {
            printf("%c%c", spu->RAM[VRAM_MAPPING + i * SPU_VRAM_HIGHT + j] / STK_PRECISION,
                           spu->RAM[VRAM_MAPPING + i * SPU_VRAM_HIGHT + j] / STK_PRECISION);
        }
    }

    return 0;
}

static Elem_t CalcMod (Elem_t numerator, Elem_t denominator)
{
    return (Elem_t) numerator % denominator;
}

static Elem_t CalcIdiv (Elem_t numerator, Elem_t denominator)
{
    Elem_t mod = CalcMod(numerator, denominator);

    return (Elem_t) ( (float) numerator - mod) / denominator * STK_PRECISION;
}

static Elem_t MultInts (Elem_t frst, Elem_t scnd)
{
    return frst * scnd / STK_PRECISION;
}

Elem_t DivideInts(Elem_t numerator, int denominator)
{
   return (Elem_t) ( (float) numerator / denominator * STK_PRECISION);
}

int printf_intermed_info (const char * format, ...)
{
    assert(format);

    if (SHOW_INTERMED_INFO)
    {
        va_list ptr;

        va_start(ptr, format);

        int res = vfprintf(stderr, format, ptr);

        va_end(ptr);

        return res;
    }

    return 0;
}
