#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"
#include "commands.h"
#include "./text_processing_lib/text_buf.h"

static int    LabelCtor     (Label labels[], int lbl_id, int byte_pos, char * name);
static int    EmitCodeArg   (char ** prog_code, char code, int val);
static int    EmitCodeReg   (char ** prog_code, char code, char reg_id);
static int    EmitCodeNoArg (char ** prog_code, char code);
static int    TokenizeText  (char ** text_ready, size_t n_lines, char * text_tokenized);
static char   ScanRegId     (const char * token);
static int    CorrectRegId  (int reg_id);

static int IsLabel (const char * token);

const int MAX_REGS = 20;

int main() {

    fprintf(stdout, "\n"
                    "# Assembler by NeTort, 2023\n"
                    "# Working...\n"
                    "# If something is wrong it will call you looser, dont cry\n\n");

    AssembleMath("ex2.txt", "ex2_translated.txt");

    return 0;
}

/**
 * @brief change commands from fin_name to their codes (from usr_cmd) fout_name. cmd_arr of usr_cmd structs is formed using ParseCmdNames func
*/
enum ASM_OUT AssembleMath (const char * fin_name, const char * fout_name) {

    assert(fin_name);
    assert(fout_name);

    //==================== READ TEXT FROM PROGRAM FILE AND SPLIT IT INTO LINES ==========================

    char *  in_buf  = NULL;
    char ** in_text = NULL; // program text split into lines

    size_t n_lines = ReadText(fin_name, &in_text, &in_buf);

    //==================== PREPROCESS EACH LINE OF THE TEXT ==========================
    PreprocessProgram(in_text, n_lines);

    //============= PUT TEXT IN ARRAY OF WORDS SEPEARATED BY BLANKS ==================

    char * text_tokenized = (char *) calloc (n_lines * CMDS_PER_LINE * MAX_CMD, sizeof(char));
    printf("BEFORE TOKENIZE\n");
    int n_tokens = TokenizeText(in_text, n_lines, text_tokenized);
    printf("after\n");
    char * prog_code = (char *) calloc(n_tokens, sizeof(int));

    printf("BEFORE TRANSLATE\n");
    int n_bytes = TranslateProgram(text_tokenized, prog_code);
    printf("after\n");

    WriteCodeBin("translated.bin", prog_code, n_bytes); // TODO filename to const

    // TODO to func
    free(prog_code);
    free(in_buf);
    free(in_text);
    free(text_tokenized); // strange thing that we free memory not from the part of code where it was allocated

    return ASM_OUT_NO_ERR;
}

int PreprocessProgram (char ** text, size_t n_lines) {

    assert(text);

    //====================== DEL COMMENTS =======================
    char * comm_pos = 0;

    for (size_t i = 0; i < n_lines; i++) {

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
    int lbl_id = 0;

    char token[MAX_CMD] = "";
    char temp_token[MAX_CMD] = "";
    int n_bytes = 0;

    char reg_id = 0;
    int  arg    = 0;
    int  symbs  = 0;

    while (*text) {

        if (sscanf(text, "%s %n", token, &symbs) <= 0)
            break;

        text += symbs;

        if (strcmp(token, "push") == 0) {

            sscanf(text, "%s %n", &temp_token, &symbs);

            if ((reg_id = ScanRegId(temp_token)) != -1) {

                if (!CorrectRegId(reg_id)) {

                    fprintf(stderr, "SyntaxError! Register \"%s\" is not allowed!\n", token);
                    abort();
                }

                text += strlen(temp_token);
                n_bytes += sizeof(char);

                EmitCodeReg(&prog_code, ARG_REGTR_VAL | CMD_PUSH, reg_id);
            }
            else if (sscanf(text, "%d %n", &arg, &symbs) == 1) {

                text += symbs; // "123" -> skipping 3 bytes
                n_bytes += sizeof(int);

                EmitCodeArg(&prog_code, ARG_IMMED_VAL | CMD_PUSH, arg);
            }
            else {

                fprintf(stderr, "# SyntaxError! No argument after \"push\"\n");
                abort();
            }
        }
        else if (strcmp(token, "pop") == 0) {

            sscanf(text, "%s %n", &temp_token, &symbs);

            if ((reg_id = ScanRegId(temp_token)) != -1) {

                if (!CorrectRegId(reg_id)) {

                    fprintf(stderr, "SyntaxError! Register \"%s\" is not allowed!\n", token);
                    abort();
                }

                text += strlen(temp_token);
                n_bytes += sizeof(char);

                EmitCodeReg(&prog_code, ARG_REGTR_VAL | CMD_POP, reg_id);
            }
            else {

                EmitCodeNoArg(&prog_code, CMD_POP);
            }
        }
        else if (strcmp(token, "hlt") == 0) {

            EmitCodeNoArg(&prog_code, CMD_HLT);
        }
        else if (strcmp(token, "in") == 0) {

            EmitCodeNoArg(&prog_code, CMD_IN);
        }
        else if (strcmp(token, "out") == 0) {

            EmitCodeNoArg(&prog_code, CMD_OUT);
        }
        else if (strcmp(token, "add") == 0) {

            EmitCodeNoArg(&prog_code, CMD_ADD);
        }
        else if (strcmp(token, "sub") == 0) {

            EmitCodeNoArg(&prog_code, CMD_SUB);
        }
        else if (strcmp(token, "mul") == 0) {

            EmitCodeNoArg(&prog_code, CMD_MUL);
        }
        else if (strcmp(token, "div") == 0) {

            EmitCodeNoArg(&prog_code, CMD_DIV);
        }
        else if (IsLabel(token)) {

            LabelCtor(labels, lbl_id, n_bytes, token);
            lbl_id++;
            n_bytes -= sizeof(char); // by default it treats labels as command or argument and increases number of bytes, so we need to decrease it when we figured out it is label
        }
        else if (strcmp(token, "jmp") == 0) {

            char lbl_temp[MAX_CMD] = "";
            if (sscanf(text, "%s %n", lbl_temp, &symbs) == 1) {

                text += symbs; // "rax" len of string (assume all registers consist of )
                n_bytes += sizeof(char);

                for (int i = 0; i < lbl_id; i++) {

                    if (strcmp(labels[i].name, lbl_temp) == 0) {

                        EmitCodeArg(&prog_code, CMD_JMP, labels[i].cmd_ptr);
                        break;
                    }

                    if (i == lbl_id - 1) {

                        fprintf(stderr, "SyntaxError! No label \"%s\" found in labels array!\n", lbl_temp);
                        abort();
                    }
                }
            }
            else {

                fprintf(stderr, "Syntax Error! No label to jmp given! Bye bye looser!\n");
                abort();
            }
        }
        else {

            fprintf(stderr, "# Syntax error! No command \"%s\" found. Bye bye looser!\n", token);
            abort();
        }

        memset(token, 0, MAX_CMD); // clean memory in token // todo catch errors
        memset(temp_token, 0, MAX_CMD);

        n_bytes += sizeof(char); //*
    }

    return n_bytes;
}

//! dont use - not working for mem-effective byte-code
int WriteCodeTxt(const char * fout_name, char * prog_code, size_t n_tokens) {

    assert(fout_name);
    assert(prog_code);

    FILE * fout = fopen(fout_name, "wb");
    assert(fout);

    int cmd_id  = 0;

    for (size_t ip = 0; ip < n_tokens; ip++) {

        cmd_id  = prog_code[ip];
        fprintf(fout, "%d ", cmd_id);

        if (prog_code[ip] != CMD_HLT && (prog_code[ip] & ARG_IMMED_VAL || prog_code[ip] & ARG_REGTR_VAL))
            fprintf(fout, "%d", prog_code[++ip]);

        fprintf(fout, "\n");

        cmd_id  = 0;
    }

    fclose(fout);

    return 0; // todo enum
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

        line_size = strlen(text[line]);

        strncpy(text_tokenized, text[line], line_size); //! SEGMENTATION FAULT

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

static int EmitCodeArg (char ** prog_code_ptr, char code, int val) {

    assert (prog_code_ptr);
    assert (*prog_code_ptr);

    memcpy(*prog_code_ptr, &code, sizeof(char));
    *prog_code_ptr += sizeof(char);

    memcpy(*prog_code_ptr, &val, sizeof(int));
    *prog_code_ptr += sizeof(int);

    return 0;
}

static int EmitCodeReg (char ** prog_code_ptr, char code, char reg_id) {

    assert(prog_code_ptr);

    memcpy(*prog_code_ptr, &code, sizeof(char));
    *prog_code_ptr += sizeof(char);

    memcpy(*prog_code_ptr, &reg_id, sizeof(char));
    *prog_code_ptr += sizeof(char);

    return 0;
}

static int EmitCodeNoArg (char ** prog_code_ptr, char code) {

    assert (prog_code_ptr);
    assert (*prog_code_ptr);

    memcpy(*prog_code_ptr, &code, sizeof(char));
    *prog_code_ptr += sizeof(char);

    return 0;
}

/**
 * @brief receives token and says if it is register or not
*/
static char ScanRegId (const char * token) {

    assert(token);

    char reg_id = 0;
    int symbs = 0;

    if (sscanf(token, "r%cx %n", &reg_id, &symbs)) {

        reg_id -= 'a';

        return reg_id;
    }

    return -1; // token does not match register template
}

static int CorrectRegId (int reg_id) {

    if (reg_id >= 0 && reg_id < MAX_REGS)
        return 1;

    return 0;
}

/**
 * @return position from which to scan the label name
*/
int IsLabel(const char * token) {

    // todo both :label and label: types of labels
    // todo make it real soviet function

    assert (token);

    const char * col_pos = 0;
    char * temp = 0;
    if (col_pos = strchr(token, ':')) {

        if (sscanf(col_pos, "%s", temp)) {

            fprintf(stderr, "SyntaxError! \"%s\" after \":\" in label name\n", temp);
            return 0;
        }

        if (isalpha(*token)) {

            token++;

            while (isalnum(*token)) token++;

        if (token == col_pos)
            return 1;

        return 0;
        }
    }

    return 0;

}

int LabelCtor (Label labels[], int lbl_id, int byte_pos, char * name) {

    char temp[MAX_CMD] = "";
    strncpy(temp, name, MAX_CMD);

    char * col_pos = strchr(temp, ':');
    *col_pos = 0;

    labels[lbl_id] = {byte_pos, temp};

    return 0; // todo return enum
}
