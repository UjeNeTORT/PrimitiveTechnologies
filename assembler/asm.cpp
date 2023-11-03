#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"
#include "../enums.h"
#include "../text_processing_lib/text_buf.h"

/**
 * @brief create new label in labels array (n_lbls has no be increased manually outside the function)
*/
static int  LabelCtor     (Label labels[], int n_lbls, int byte_pos, const char * name);

/**
 * @brief delete all the labels from the labels array, free the memory
*/
static int  LabelDtor     (Label labels[], int n_lbls);

/**
 * @brief return position of label with name token in labels array
*/
static int  LabelFind     (Label labels[], int n_lbls, size_t hash);

/**
 * @brief put cmd-code with argument val (int) to prog-code array
*/
static int  EmitCodeArg   (char * prog_code, int * n_bytes, char code, int val);

/**
 * @brief put cmd-code with argument reg_id (char) to prog-code array
*/
static int  EmitCodeReg   (char * prog_code, int * n_bytes, char code, char reg_id);

/**
 * @brief put cmd-code with argument val (int) + reg_id (char) to prog-code array
*/
static int  EmitCodeSum   (char * prog_code, int * n_bytes, char code, int val, char reg_id);

/**
 * @brief put cmd-code without argument to prog-code array
*/
static int  EmitCodeNoArg (char * prog_code, int * n_bytes, char code);

/**
 * @brief put all the words to an array separated by blanks
*/
static int  TokenizeText  (char ** text_ready, size_t n_lines, char * text_tokenized);

/**
 * @brief check if register name is allowed
*/
static int  CorrectRegId  (int reg_id);

/**
 * @brief check if token has : in the end = is label
*/
static int IsLabel (const char * token);

static int ProcessPushArguments (cmd_code_t * prog_code, int * n_bytes, char ** text);

static int ProcessPopArguments (cmd_code_t * prog_code, int * n_bytes, char ** text);

static int ProcessJmpArguments (char ** text, size_t n_run, Label labels[], int n_lbls);

static size_t HashMod(const char * word, size_t size);

const int MAX_REGS = 26; //* duplicate, register is to be checked in processor

int main(int argc, char * argv[])
{
    fprintf(stdout, "\n"
                    "# Assembler by NeTort\n"
                    "# (c) TIKHONOV YAROSLAV 2023\n\n");


    for (int argn = 0; argn < argc; argn++)
    {
        if (strcmp(argv[argn], "--finname") == 0)
        {
            Assemble(argv[argn + 1], BIN_FILENAME);
            argn++;
        }
    }

    return 0;
}

/**
 * @brief change commands from fin_name to their codes (from usr_cmd) fout_name. cmd_arr of usr_cmd structs is formed using ParseCmdNames func
*/
AsmResType Assemble (const char * fin_name, const char * fbinout_name)
{
    assert(fin_name);
    assert(fbinout_name);

    //============= READ TEXT FROM PROGRAM FILE AND SPLIT IT INTO LINES ==============

    char *  in_buf  = NULL;
    char ** in_text = NULL; // program text split into lines

    size_t n_lines = ReadText(fin_name, &in_text, &in_buf);

    //==================== PREPROCESS EACH LINE OF THE TEXT ==========================

    if (DecommentProgram(in_text, n_lines) != DECOM_NO_ERR)
    {
        fprintf(stderr, "DecommentProgram: error\n");
        abort();
    }

    //============= PUT TEXT IN ARRAY OF WORDS SEPEARATED BY BLANKS ==================

    char * text_tokenized = (char *) calloc(n_lines * CMDS_PER_LINE * MAX_CMD, sizeof(char));

    int n_tokens = TokenizeText(in_text, n_lines, text_tokenized);

    //========================== CREATE BYTECODE ARRAY ===============================

    cmd_code_t * prog_code = (cmd_code_t *) calloc(n_tokens, sizeof(int));

    //====================== TRANSLATE TEXT INTO BYTE-CODES ==========================

    int n_bytes = TranslateProgram(text_tokenized, prog_code);

    //========================== OUTPUT TO BINARY FILE ===============================

    if (WriteCodeBin(fbinout_name, prog_code, n_bytes) != WRITE_NO_ERR)
    {
        abort();
    }

    //====================== FREE ALL THE ALLOCATED MEMORY ===========================

    free(prog_code);
    free(in_buf);
    free(in_text);
    free(text_tokenized);

    return ASM_OUT_NO_ERR;
}

DecommentRes DecommentProgram (char ** text, size_t n_lines)
{
    assert(text);

    //====================== DEL COMMENTS =======================
    char * comm_pos = 0;

    for (size_t i = 0; i < n_lines; i++)
    {
        comm_pos = strchr(text[i], ';');
        if (comm_pos)
            *comm_pos = '\0';
    }

    return DECOM_NO_ERR;
}

//* works only with preprocessed program
//* any error inside translator leads to abort of assembly program with error message (no return codes due to no need)
int TranslateProgram (char * text, cmd_code_t * prog_code) {

    assert(text);
    assert(*text);
    assert(prog_code);

    Label labels[MAX_LBLS] = {};
    int n_lbls = 0;

    char token[MAX_CMD] = "";
    char temp_token[MAX_CMD] = "";
    int n_bytes = 0;

    int symbs = 0;

    char * text_init = text;
    cmd_code_t * prog_code_init = prog_code;

    #define DEF_CMD(name, num, text, have_arg, spu_code, code_have_arg, ...) \
        else if (strcmp(token, text) == 0)                              \
        {                                                               \
            if (have_arg)                                               \
            {                                                           \
                code_have_arg;                                          \
            }                                                           \
            else                                                        \
            {                                                           \
                EmitCodeNoArg(prog_code, &n_bytes, CMD_##name);         \
            }                                                           \
        }

    for (size_t n_run = 0; n_run < RUNS_CNT; n_run++)
    {
        prog_code = prog_code_init;
        text = text_init;
        n_bytes = 0;

        while (*text)
        {
            if (sscanf(text, "%s %n", token, &symbs) <= 0)
                break;

            text += symbs;

            if (0)
            {
                ;
            }

            #include "../commands.h"

            else if (IsLabel(token))
            {
                if (n_run == RUN_LBL_UPD)
                {
                    ;
                }
                else
                {
                    LabelCtor(labels, n_lbls, n_bytes, (const char *) token);
                    n_lbls++;
                }
            }
            else
            {
                fprintf(stderr, "# Syntax error! No command \"%s\" (%d) found. Bye bye looser!\n", token, n_bytes);
                abort();
            }

            memset(token, 0, MAX_CMD); // clean memory in token
            memset(temp_token, 0, MAX_CMD);

        }
    }

    #undef DEF_CMD

    LabelDtor(labels, n_lbls);

    return n_bytes;
}

static int ProcessJmpArguments (char ** text, size_t n_run, Label labels[], int n_lbls)
{
    char lbl_name[MAX_CMD] = "";

    int symbs = 0;

    if (sscanf(*text, "%s %n", lbl_name, &symbs) != 1)
    {
        fprintf(stderr, "Syntax Error! No label to jmp given! Bye bye looser!\n");
        abort();
    }

    *text += symbs;

    int cmd_ptr = -1;

    if (n_run == RUN_LBL_UPD)
    {
        cmd_ptr = LabelFind(labels, n_lbls, HashMod(lbl_name, strlen(lbl_name)));
        if (cmd_ptr == -1)
        {
            fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
            abort();
        }
    }

    return cmd_ptr;
}

static int ProcessPushArguments (cmd_code_t * prog_code, int * n_bytes, char ** text)
{
    int  symbs  = 0;
    char reg_id = 0;
    int  val    = 0;

    if (sscanf(*text, "[ r%cx + %d ] %n", &reg_id, &val, &symbs) == 2 ||
        sscanf(*text, "[ %d + r%cx ] %n", &val, &reg_id, &symbs) == 2)
    {
        reg_id -= 'a';
        if (!CorrectRegId(reg_id))
        {
            fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed!\n", reg_id + 'a');
            abort();
        }

        EmitCodeSum(prog_code, n_bytes, ARG_MEMRY_VAL | ARG_IMMED_VAL | ARG_REGTR_VAL | CMD_PUSH, val, reg_id);
        *text += symbs;
    }
    else if (sscanf(*text, " [ r%cx ] %n", &reg_id, &symbs) == 1)
    {
        reg_id -= 'a';
        if (!CorrectRegId(reg_id))
        {
            fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed!\n", reg_id + 'a');
            abort();
        }

        EmitCodeReg(prog_code, n_bytes, ARG_MEMRY_VAL | ARG_REGTR_VAL | CMD_PUSH, reg_id);
        *text += symbs;
    }
    else if (sscanf(*text, " [ %d ] %n", &val, &symbs) == 1)
    {
        EmitCodeArg(prog_code, n_bytes, ARG_MEMRY_VAL | ARG_IMMED_VAL | CMD_PUSH, val);
        *text += symbs;
    }
    else if (sscanf(*text, "r%cx + %d %n", &reg_id, &val, &symbs) == 2 ||
             sscanf(*text, "%d + r%cx %n", &val, &reg_id, &symbs) == 2)
    {
        reg_id -= 'a';
        if (!CorrectRegId(reg_id))
        {
            fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed!\n", reg_id + 'a');
            abort();
        }

        EmitCodeSum(prog_code, n_bytes, ARG_IMMED_VAL | ARG_REGTR_VAL | CMD_PUSH, val, reg_id);
        *text += symbs;
    }
    else if (sscanf(*text, "r%cx %n", &reg_id, &symbs) == 1)
    {
        reg_id -= 'a';
        if (!CorrectRegId(reg_id))
        {
            fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed!\n", reg_id + 'a');
            abort();
        }

        EmitCodeReg(prog_code, n_bytes, ARG_REGTR_VAL | CMD_PUSH, reg_id);
        *text += symbs;
    }
    else if (sscanf(*text, "%d %n", &val, &symbs) == 1)
    {
        EmitCodeArg(prog_code, n_bytes, ARG_IMMED_VAL | CMD_PUSH, val);
        *text += symbs;
    }
    else
    {
       fprintf(stderr, "Syntax Error! No command after \"push\" matches its argument type\n");
       abort();
    }

    return 0;
}

static int ProcessPopArguments (cmd_code_t * prog_code, int * n_bytes, char ** text)
{
    int  symbs  = 0;
    char reg_id = 0;
    int  val    = 0;

    if (sscanf(*text, "[ r%cx + %d ] %n", &reg_id, &val, &symbs) == 2 ||
        sscanf(*text, "[ %d + r%cx ] %n", &val, &reg_id, &symbs) == 2)
    {
        reg_id -= 'a';
        if (!CorrectRegId(reg_id))
        {
            fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed!\n", reg_id + 'a', reg_id + 'a');
            abort();
        }

        EmitCodeSum(prog_code, n_bytes, ARG_MEMRY_VAL | ARG_REGTR_VAL | ARG_IMMED_VAL | CMD_POP, val, reg_id);
        *text += symbs;
    }
    else if (sscanf(*text, "[ r%cx ] %n", &reg_id, &symbs) == 1)
    {
        reg_id -= 'a';
        if (!CorrectRegId(reg_id))
        {
            fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed!\n", reg_id + 'a', reg_id + 'a');
            abort();
        }
        EmitCodeReg(prog_code, n_bytes, ARG_MEMRY_VAL | ARG_REGTR_VAL | CMD_POP, reg_id);
        *text += symbs;
    }
    else if (sscanf(*text, "[ %d ] %n", &val, &symbs) == 1)
    {
        EmitCodeArg(prog_code, n_bytes, ARG_MEMRY_VAL | ARG_IMMED_VAL | CMD_POP, val);
        *text += symbs;
    }
    else if (sscanf(*text, "r%cx %n", &reg_id, &symbs) == 1)
    {
        reg_id -= 'a';
        if (!CorrectRegId(reg_id))
        {
            fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed!\n", reg_id + 'a', reg_id + 'a');
            abort();
        }
        EmitCodeReg(prog_code, n_bytes, ARG_REGTR_VAL | CMD_POP, reg_id);
        *text += symbs;
    }

    return 0;
}

WriteBinRes WriteCodeBin (const char * fout_name, cmd_code_t * prog_code, size_t n_bytes)
{
    assert(fout_name);
    assert(prog_code);

    FILE * fout = fopen(fout_name, "wb");
    assert(fout);

    // put size in the beginning

    size_t write_res = fwrite(&n_bytes, sizeof(size_t), 1, fout);
    if (write_res == 0)
    {
        fprintf(stderr, "WriteCodeBin: fwrite error\n");

        return WRITE_ERR;
    }

    write_res = fwrite(prog_code, sizeof(cmd_code_t), n_bytes, fout);
    if (write_res == 0)
    {
        fprintf(stderr, "WriteCodeBin: fwrite error\n");

        return WRITE_ERR;
    }

    fclose(fout);

    return WRITE_NO_ERR;
}

int TokenizeText (char ** text, size_t n_lines, char * text_tokenized)
{
    assert(text);

    char * tt_init = text_tokenized;

    size_t line_size = 0;

    for (size_t line = 0; line < n_lines; line++) {

        strcpy(text_tokenized, text[line]);

        line_size = strlen(text[line]);
        text_tokenized += line_size;
        *text_tokenized = ' ';
        text_tokenized++;
    }

    *text_tokenized++ = 0;

    char token[MAX_CMD] = "";
    int symbs = 0;
    int n_tokens = 0;

    text_tokenized = tt_init;

    // calculate number of tokens
    int scan_res = sscanf(text_tokenized, "%s %n", token, &symbs);
    while (scan_res != 0 && scan_res != EOF)
    {
        text_tokenized += symbs;
        n_tokens++;

        scan_res = sscanf(text_tokenized, "%s %n", token, &symbs);
    }

    return n_tokens;
}

static int EmitCodeArg (cmd_code_t * prog_code, int * n_bytes, cmd_code_t code, int val)
{
    assert (prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(cmd_code_t)); // emit cmd code
    *n_bytes += sizeof(cmd_code_t);

    memcpy((prog_code + *n_bytes), &val, sizeof(int));         // emit immed val
    *n_bytes += sizeof(int);

    return 0;
}

static int EmitCodeReg (cmd_code_t * prog_code, int * n_bytes, cmd_code_t code, char reg_id)
{
    assert(prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(cmd_code_t)); // emit cmd code
    *n_bytes += sizeof(cmd_code_t);

    memcpy((prog_code + *n_bytes), &reg_id, sizeof(char));     // emit reg_id
    *n_bytes += sizeof(char);

    return 0;
}

// Write
static int EmitCodeSum (cmd_code_t * prog_code, int * n_bytes, cmd_code_t code, int val, char reg_id)
{
    assert(prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(cmd_code_t));   // emit cmd code
    *n_bytes += sizeof(cmd_code_t);

    memcpy((prog_code + *n_bytes), &val, sizeof(int));           // emit immed val
    *n_bytes += sizeof(int);

    memcpy((prog_code + *n_bytes), &reg_id, sizeof(char));       // emit reg_id
    *n_bytes += sizeof(char);

    return 0;
}

// WriteOpNoArg
static int EmitCodeNoArg (cmd_code_t * prog_code, int * n_bytes, char code)
{
    assert (prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(cmd_code_t)); // emit cmd code
    *n_bytes += sizeof(cmd_code_t);

    return 0;
}

static int CorrectRegId (int reg_id)
{
    if (reg_id >= 0 && reg_id < MAX_REGS)
        return 1;

    return 0;
}

/**
 * @return position from which to scan the label name
*/
int IsLabel(const char * token)
{
    assert (token);

    const char * col_pos = strchr(token, ':');
    char *       temp    = 0;

    if (col_pos)
    {
        int scan_res = sscanf(col_pos, "%s", temp);
        if (scan_res)
        {
            fprintf(stderr, "SyntaxError! \"%s\" after \":\" in label name\n", temp);

            return 0;
        }

        if (isalpha(*token))
        {
            token++;

            while (isalnum(*token) || *token == '_') token++;

            if (token == col_pos)
                return 1;

            return 0;
        }
    }

    return 0;
}

int LabelCtor (Label labels[], int n_lbls, int byte_pos, const char * name)
{
    char * name_no_col = strdup(name);

    char * col_pos = strchr(name_no_col, ':');
    *col_pos = 0;

    size_t hash = HashMod(name_no_col, strlen(name_no_col));

    if (LabelFind(labels, n_lbls, hash) != -1)
    {
        fprintf(stderr, "Syntax Error! Two labels with same name found!\n");
        abort();
    }

    labels[n_lbls] = {byte_pos, hash, name_no_col};

    return 0;
}

int LabelDtor (Label labels[], int n_lbls)
{
    for (int i = 0; i < n_lbls; i++)
    {
        free(labels[i].name);
    }

    return 0;
}

int LabelFind (Label labels[], int n_lbls, size_t hash)
{
    for (int i = 0; i < n_lbls; i++)
    {
        if (labels[i].hash == hash)
        {
            return labels[i].cmd_ptr;
        }
        if (i == n_lbls - 1)
        {
            return -1;
        }
    }

    return -1;
}

size_t HashMod(const char * word, size_t size)
{
    assert (word);

    size_t hash = 0;

    for (size_t i = 0; i < size; i++)
    {
        hash += (size_t) word[i];
        hash %= 1000000007;
    }

    return hash;
}
