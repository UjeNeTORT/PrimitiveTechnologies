#ifndef ASM_H
#define ASM_H

#include <assert.h>
#include <ctype.h>
#include <cstdio>
#include <stdlib.h>

#include "../enums.h"

const size_t       RUNS_CNT       = 2;
const size_t       RUN_LBL_UPD    = 1;

const size_t       MAX_LINES      = 100;
const size_t       CMDS_PER_LINE  = 2;

const char * const DFLT_CMDS_FILE = "user_commands.txt";

struct Label
{
    int    cmd_ptr;
    size_t hash;
    char * name;
};

typedef enum
{
    ASM_OUT_NO_ERR = 0,
    ASM_OUT_ERR    = 1,
} AsmResType;

typedef enum
{
    DECOM_NO_ERR = 0,
    DECOM_ERR    = 1,
} DecommentRes;

typedef enum
{
    WRITE_NO_ERR    = 0,
    WRITE_ERR       = 1,
} WriteBinRes;

enum REG_ID_OUT
{
    REG_ID_NOT_ALLOWED = -2,
    REG_ID_NOT_A_REG   = -1
};

static AsmResType   Assemble          (const char * fin_name, const char * fout_name);
static DecommentRes DecommentProgram  (char ** text, size_t n_lines);
static int          TranslateProgram  (char * text, char * prog_code);
static WriteBinRes  WriteCodeBin      (const char * fout_name, char * prog_code, size_t n_bytes);

#endif // ASM_H
