#ifndef COMPILER_H
#define COMPILER_H

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "commands.h"

const size_t       RUNS_CNT       = 2;
const size_t       RUN_LBL_UPD    = 1;

const size_t       MAX_LINES      = 100;
const size_t       CMDS_PER_LINE  = 2;
const char * const DFLT_CMDS_FILE = "user_commands.txt";

enum ASM_OUT {
    ASM_OUT_NO_ERR = 0,
    ASM_OUT_ERR    = 1
};

enum REG_ID_OUT {
    REG_ID_NOT_ALLOWED = -2,
    REG_ID_NOT_A_REG   = -1
};

static enum ASM_OUT AssembleMath      (const char * fin_name, const char * fout_name);
static int          PreprocessProgram (char ** text, size_t n_lines);
static int          TranslateProgram  (char * text, char * prog_code);
static int          WriteCodeTxt      (const char * fout_name, char * prog_code, size_t n_cmds);
static int          WriteCodeBin      (const char * fout_name, char * prog_code, size_t n_cmds);

#endif // COMPILER_H
