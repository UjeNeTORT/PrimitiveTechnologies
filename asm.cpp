#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"
#include "commands.h"
#include "./text_processing_lib/text_buf.h"

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
static int  LabelFind     (Label labels[], int n_lbls, char * token);

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

const int MAX_REGS = 26; //* duplicate, register is to be checked in processor

int main(int argc, char * argv[]) {

    fprintf(stdout, "\n"
                    "# Assembler by NeTort, 2023\n"
                    "# Working...\n"
                    "# If something is wrong it will call you looser, dont cry\n\n");


    for (int argn = 0; argn < argc; argn++)
    {
        if (strcmp(argv[argn], "--finname") == 0)
        {
            AssembleMath(argv[argn + 1], BIN_FILENAME);
            argn++;
        }
    }


    return 0;
}

/**
 * @brief change commands from fin_name to their codes (from usr_cmd) fout_name. cmd_arr of usr_cmd structs is formed using ParseCmdNames func
*/
AsmResType AssembleMath (const char * fin_name, const char * fbinout_name) {

    assert(fin_name);
    assert(fbinout_name);

    //============= READ TEXT FROM PROGRAM FILE AND SPLIT IT INTO LINES ==============

    char *  in_buf  = NULL;
    char ** in_text = NULL; // program text split into lines

    size_t n_lines = ReadText(fin_name, &in_text, &in_buf);

    //==================== PREPROCESS EACH LINE OF THE TEXT ==========================

    DecommentProgram(in_text, n_lines);

    //============= PUT TEXT IN ARRAY OF WORDS SEPEARATED BY BLANKS ==================

    char * text_tokenized = (char *) calloc (n_lines * CMDS_PER_LINE * MAX_CMD, sizeof(char));

    int n_tokens = TokenizeText(in_text, n_lines, text_tokenized);

    //========================== CREATE BYTECODE ARRAY ===============================

    char * prog_code = (char *) calloc(n_tokens, sizeof(int));

    //====================== TRANSLATE TEXT INTO BYTE-CODES ==========================

    int n_bytes = TranslateProgram(text_tokenized, prog_code);

    //========================== OUTPUT TO BINARY FILE ===============================

    WriteCodeBin(fbinout_name, prog_code, n_bytes);

    //====================== FREE ALL THE ALLOCATED MEMORY ===========================

    // TODO to func
    free(prog_code);
    free(in_buf);
    free(in_text);
    free(text_tokenized); // strange thing that we free memory not from the part of code where it was allocated

    return ASM_OUT_NO_ERR;
}

int DecommentProgram (char ** text, size_t n_lines) {

    assert(text);

    //====================== DEL COMMENTS =======================
    char * comm_pos = 0;

    for (size_t i = 0; i < n_lines; i++)
    {
        comm_pos = strchr(text[i], ';');
        if (comm_pos)
            *comm_pos = '\0';
    }

    return 0;
}

//* works only with preprocessed program
//* any error inside translator leads to abort of assembly program with error message (no return codes due to no need)
int TranslateProgram (char * text, char * prog_code) {

    assert(text);
    assert(*text);
    assert(prog_code);

    Label labels[MAX_LBLS] = {};
    int n_lbls = 0;

    char token[MAX_CMD] = "";
    char temp_token[MAX_CMD] = "";
    int n_bytes = 0;

    char reg_id = 0;
    int  val    = 0;
    int  symbs  = 0;

    char * text_init = text;
    char * prog_code_init = prog_code;

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

            if (strcmp(token, "push") == 0)
            {
                if (sscanf(text, "[ r%cx + %d ] %n", &reg_id, &val, &symbs) == 2 ||
                    sscanf(text, "[ %d + r%cx ] %n", &val, &reg_id, &symbs) == 2)
                {
                    reg_id -= 'a';
                    if (!CorrectRegId(reg_id))
                    {
                        fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed! Get rekt! hahahaah\n", reg_id + 'a');
                        abort();
                    }

                    EmitCodeSum(prog_code, &n_bytes, ARG_MEMRY_VAL | ARG_IMMED_VAL | ARG_REGTR_VAL | CMD_PUSH, val, reg_id);

                    text += symbs;
                }
                else if (sscanf(text, " [ r%cx ] %n", &reg_id, &symbs) == 1)
                {
                    reg_id -= 'a';
                    if (!CorrectRegId(reg_id))
                    {
                        fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed! Get rekt! hahahaah\n", reg_id + 'a');
                        abort();
                    }

                    EmitCodeReg(prog_code, &n_bytes, ARG_MEMRY_VAL | ARG_REGTR_VAL | CMD_PUSH, reg_id);

                    text += symbs;
                }
                else if (sscanf(text, " [ %d ] %n", &val, &symbs) == 1)
                {

                    EmitCodeArg(prog_code, &n_bytes, ARG_MEMRY_VAL | ARG_IMMED_VAL | CMD_PUSH, val);

                    text += symbs;
                }
                else if (sscanf(text, "r%cx + %d %n", &reg_id, &val, &symbs) == 2 ||
                         sscanf(text, "%d + r%cx %n", &val, &reg_id, &symbs) == 2)
                {

                    reg_id -= 'a';
                    if (!CorrectRegId(reg_id))
                    {
                        fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed! Get rekt! hahahaah\n", reg_id + 'a');
                        abort();
                    }

                    EmitCodeSum(prog_code, &n_bytes, ARG_IMMED_VAL | ARG_REGTR_VAL | CMD_PUSH, val, reg_id);

                    text += symbs;
                }
                else if (sscanf(text, "r%cx %n", &reg_id, &symbs) == 1)
                {
                    reg_id -= 'a';
                    if (!CorrectRegId(reg_id))
                    {
                        // assert(!"Syntax Error!");
                        fprintf(stderr, "Syntax Error! Register \"r%cx\" not allowed! Get rekt! hahahaah\n", reg_id + 'a');
                        abort();
                    }

                    EmitCodeReg(prog_code, &n_bytes, ARG_REGTR_VAL | CMD_PUSH, reg_id);

                    text += symbs;
                }
                else if (sscanf(text, "%d %n", &val, &symbs) == 1)
                {
                    EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_PUSH, val);

                    text += symbs;
                }
                else
                {
                    fprintf(stderr, "Syntax Error! No command after \"push\" matches its argument type\n");
                    abort();
                }
            }
            else if (strcmp(token, "pop") == 0)
            {
                if (sscanf(text, "[ r%cx + %d ] %n", &reg_id, &val, &symbs) == 2 ||
                    sscanf(text, "[ %d + r%cx ] %n", &val, &reg_id, &symbs) == 2)
                {
                    reg_id -= 'a';
                    if (!CorrectRegId(reg_id))
                    {
                        fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed! Get rekt! hahahaah\n", reg_id + 'a', reg_id + 'a');
                        abort();
                    }

                    EmitCodeSum(prog_code, &n_bytes, ARG_MEMRY_VAL | ARG_REGTR_VAL | ARG_IMMED_VAL | CMD_POP, val, reg_id);

                    text += symbs;
                }
                else if (sscanf(text, "[ r%cx ] %n", &reg_id, &symbs) == 1)
                {
                    reg_id -= 'a';
                    if (!CorrectRegId(reg_id))
                    {
                        fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed! Get rekt! hahahaah\n", reg_id + 'a', reg_id + 'a');
                        abort();
                    }

                    EmitCodeReg(prog_code, &n_bytes, ARG_MEMRY_VAL | ARG_REGTR_VAL | CMD_POP, reg_id);

                    text += symbs;
                }
                else if (sscanf(text, "[ %d ] %n", &val, &symbs) == 1)
                {
                    EmitCodeArg(prog_code, &n_bytes, ARG_MEMRY_VAL | ARG_IMMED_VAL | CMD_POP, val);

                    text += symbs;
                }
                else if (sscanf(text, "r%cx %n", &reg_id, &symbs) == 1)
                {
                    reg_id -= 'a';
                    if (!CorrectRegId(reg_id))
                    {
                        fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed! Get rekt! hahahaah\n", reg_id + 'a', reg_id + 'a');
                        abort();
                    }

                    EmitCodeReg(prog_code, &n_bytes, ARG_REGTR_VAL | CMD_POP, reg_id);

                    text += symbs;
                }
                else
                {
                    EmitCodeNoArg(prog_code, &n_bytes, CMD_POP);
                }

                }
            else if (strcmp(token, "hlt") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_HLT);
            }
            else if (strcmp(token, "in")  == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_IN);
            }
            else if (strcmp(token, "out") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_OUT);
            }
            else if (strcmp(token, "add") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_ADD);
            }
            else if (strcmp(token, "sub") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_SUB);
            }
            else if (strcmp(token, "mul") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_MUL);
            }
            else if (strcmp(token, "sqrt") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_SQRT);
            }
            else if (strcmp(token, "div") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_DIV);
            }
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
            else if (strcmp(token, "call") == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to call given!\n");
                    abort();
                }

                text += symbs;

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_CALL, cmd_ptr);
            }
            else if (strcmp(token , "ret") == 0)
            {
                EmitCodeNoArg(prog_code, &n_bytes, CMD_RET);
            }
            else if (strcmp(token, "jmp") == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to jmp given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JMP, cmd_ptr);
            }
            else if (strcmp(token, "ja")  == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to ja given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JA, cmd_ptr);
            }
            else if (strcmp(token, "jae") == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to jae given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JAE, cmd_ptr);
            }
            else if (strcmp(token, "jb")  == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to jb given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JB, cmd_ptr);
            }
            else if (strcmp(token, "jbe") == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to jbe given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JBE, cmd_ptr);
            }
            else if (strcmp(token, "je")  == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to je given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JE, cmd_ptr);
            }
            else if (strcmp(token, "jne") == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to jne given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JNE, cmd_ptr);
            }
            else if (strcmp(token, "jf")  == 0)
            {
                char lbl_name[MAX_CMD] = "";

                if (sscanf(text, "%s %n", lbl_name, &symbs) != 1)
                {
                    fprintf(stderr, "Syntax Error! No label to jf given! Bye bye looser!\n");
                    abort();
                }

                text += symbs; // "rax" len of string (assume all registers consist of )

                int cmd_ptr = -1;

                if (n_run == RUN_LBL_UPD)
                {
                    cmd_ptr = LabelFind(labels, n_lbls, lbl_name);

                    if (cmd_ptr == -1)
                    {
                        fprintf(stderr, "Syntax Error! No label named \"%s\" found on second run.\n", lbl_name);
                        abort();
                    }
                }

                EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JF, cmd_ptr);
            }
            else
        {
            fprintf(stderr, "# Syntax error! No command \"%s\" (%d) found. Bye bye looser!\n", token, n_bytes);
            abort();
        }

            memset(token, 0, MAX_CMD); // clean memory in token // todo catch errors
            memset(temp_token, 0, MAX_CMD);

        }
    }
    LabelDtor(labels, n_lbls);

    return n_bytes;
}

int WriteCodeBin (const char * fout_name, char * prog_code, size_t n_tokens) {

    assert(fout_name);
    assert(prog_code);

    FILE * fout = fopen(fout_name, "wb");
    assert(fout);

    // put size in the beginning (this can be the beginning of signature-maker function}
    fwrite(&n_tokens, sizeof(size_t), 1, fout); // TODO check return val

    fwrite(prog_code, sizeof(*prog_code), n_tokens, fout); // TODO check return val

    fclose(fout);

    return 0; // TODO return enum
}

int TokenizeText (char ** text, size_t n_lines, char * text_tokenized) {

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
    int scan_res = 0;
    while ((scan_res = sscanf(text_tokenized, "%s %n", &token, &symbs)) != 0 && scan_res != EOF) {

        text_tokenized += symbs;
        n_tokens++;
    }

    // free(text_tokenized); // todo free part we didnt use

    return n_tokens;
}

static int EmitCodeArg (char * prog_code, int * n_bytes, char code, int val)
{
    assert (prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(char)); // emit cmd code
    *n_bytes += sizeof(char);

    memcpy((prog_code + *n_bytes), &val, sizeof(int));   // emit immed val
    *n_bytes += sizeof(int);

    return 0;
}

static int EmitCodeReg (char * prog_code, int * n_bytes, char code, char reg_id) {

    assert(prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(char)); // emit cmd code
    *n_bytes += sizeof(char);

    memcpy((prog_code + *n_bytes), &reg_id, sizeof(char)); // emit reg_id
    *n_bytes += sizeof(char);

    return 0;
}

// Write
static int EmitCodeSum (char * prog_code, int * n_bytes, char code, int val, char reg_id)
{
    assert(prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(char));   // emit cmd code
    *n_bytes += sizeof(char);

    memcpy((prog_code + *n_bytes), &val, sizeof(int));     // emit immed val
    *n_bytes += sizeof(int);

    memcpy((prog_code + *n_bytes), &reg_id, sizeof(char)); // emit reg_id
    *n_bytes += sizeof(char);

    return 0;
}

static int EmitCodeNoArg (char * prog_code, int * n_bytes, char code) {

    assert (prog_code);

    memcpy((prog_code + *n_bytes), &code, sizeof(char)); // emit cmd code
    *n_bytes += sizeof(char);

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
    // todo both :label and label: types of labels
    // todo make it real soviet function

    assert (token);

    const char * col_pos = 0;
    char * temp = 0;

    if ((col_pos = strchr(token, ':')))
    {
        if (sscanf(col_pos, "%s", temp))
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

    if (LabelFind(labels, n_lbls, name_no_col) != -1)
    {
        fprintf(stderr, "Syntax Error! Two labels with same name found!\n");
        abort();
    }

    labels[n_lbls] = {byte_pos, name_no_col};

    return 0; // todo return enum
}

int LabelDtor (Label labels[], int n_lbls) {

    for (int i = 0; i < n_lbls; i++)
    {
        free(labels[i].name);
    }

    return 0;
}

int LabelFind (Label labels[], int n_lbls, char * token)
{
    for (int i = 0; i < n_lbls; i++)
    {
        if (strcmp(labels[i].name, token) == 0)
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
