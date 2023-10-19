#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"
#include "spu.h"
#include "commands.h"
#include "text_buf.h"

static int    EmitCodeArg   (int ** prog_code, int code, int val);
static int    EmitCodeNoArg (int ** prog_code, int code);
static char * TokenizeText  (char ** text_ready, size_t n_lines);

int notmain() {

    AssembleMath("ex1.txt", "ex1_translated.txt", "user_commands.txt");

    return 0;
}

/**
 * @brief change commands from fin_name to their codes (from usr_cmd) fout_name. cmd_arr of usr_cmd structs is formed using ParseCmdNames func
*/
enum ASM_OUT AssembleMath (const char * fin_name, const char * fout_name, const char * cmds_file) {

    assert(fin_name);
    assert(fout_name);

    //==================== READ TEXT FROM PROGRAM FILE AND SPLIT IT INTO LINES ==========================

    char *  in_buf  = NULL;
    char ** in_text = NULL; // program text split into lines

    size_t n_lines = ReadText(fin_name, &in_text, &in_buf);

    //==================== PREPROCESS EACH LINE OF THE TEXT ==========================
    PreprocessProgram(in_text, n_lines);

    //============= PUT TEXT IN ARRAY OF WORDS SEPEARATED BY BLANKS ==================
    char * text_tokenized = TokenizeText(in_text, n_lines);

    int * prog_code = (int *) calloc(n_lines * CMDS_PER_LINE, sizeof(size_t));

    int n_cmds = TranslateProgram(text_tokenized, n_lines, prog_code);

    // WriteCodeTxt(fout_name, prog_code, n_cmds);

    WriteCodeBin("translated.bin", prog_code, n_cmds); // TODO filename to vars

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
int TranslateProgram (char * text, size_t n_lines, int * prog_code) {

    assert(text);
    assert(*text);
    assert(prog_code);

    char cmd[MAX_CMD] = "";
    int n_cmds = 0;

    char reg_id = 0;
    int  arg    = 0;
    int  symbs  = 0;

    while (*text) {

        if (sscanf(text, "%s %n", cmd, &symbs) <= 0)
            break;

        text += symbs;

        if (strcmp(cmd, "push") == 0) {

            if (sscanf(text, "r%cx %n", &reg_id, &symbs) == 1) {

                text += symbs; // "rax" len of string (assume all registers consist of )
                n_cmds++;

                reg_id -= 'a';
                if (reg_id < 0 || reg_id > SPU_REGS_NUM - 1)       // TODO to function calculation of register id
                    fprintf(stderr, "Register %d is incorrect!\n", reg_id);

                EmitCodeArg(&prog_code, ARG_REGTR_VAL | CMD_PUSH, reg_id); // TODO EmitCodeReg in order not to waste 4 bytes for reg index which is no more than 256
            }
            else if (sscanf(text, "%d %n", &arg, &symbs) == 1) {

                text += symbs; // "123" -> skipping 3 bytes //* here was DecDigitsIn func
                n_cmds++;

                EmitCodeArg(&prog_code, ARG_IMMED_VAL | CMD_PUSH, arg);
            }
            else {

                fprintf(stderr, "SyntaxError! No argument after \"push\"\n");
                abort();
            }
        }
        else if (strcmp(cmd, "pop") == 0) {

            if (sscanf(text, "r%cx %n", &reg_id, &symbs) == 1) {

                text += symbs; // "rax" len of string (assume all registers consist of )
                n_cmds++;

                reg_id -= 'a';
                if (reg_id < 0 || reg_id > SPU_REGS_NUM - 1)       // TODO to function calculation of register id
                    fprintf(stderr, "Register %d is incorrect!\n", reg_id);

                EmitCodeArg(&prog_code, ARG_REGTR_VAL | CMD_POP, reg_id); // TODO EmitCodeReg in order not to waste 4 bytes for reg index which is no more than 256

            }
            else if (sscanf(text, "%d %n", &arg, &symbs) == 1) { // useless as there are no immed val args for pop

                text += symbs; // "123" -> skipping 3 bytes

                n_cmds++;

                EmitCodeArg(&prog_code, ARG_IMMED_VAL | CMD_POP, arg);
            }
            else {

                EmitCodeNoArg(&prog_code, CMD_POP);
            }
        }
        else if (strcmp(cmd, "hlt") == 0) {

            EmitCodeNoArg(&prog_code, CMD_HLT);
        }
        else if (strcmp(cmd, "in") == 0) {

            EmitCodeNoArg(&prog_code, CMD_IN);
        }
        else if (strcmp(cmd, "out") == 0) {

            EmitCodeNoArg(&prog_code, CMD_OUT);
        }
        else if (strcmp(cmd, "add") == 0) {

            EmitCodeNoArg(&prog_code, CMD_ADD);
        }
        else if (strcmp(cmd, "sub") == 0) {

            EmitCodeNoArg(&prog_code, CMD_SUB);
        }
        else if (strcmp(cmd, "mul") == 0) {

            EmitCodeNoArg(&prog_code, CMD_MUL);
        }
        else if (strcmp(cmd, "div") == 0) {

            EmitCodeNoArg(&prog_code, CMD_DIV);
        }
        else {

            fprintf(stderr, "Syntax error! No command \"%s\" found. Bye bye looser!\n", cmd);
            abort();
        }

        memset(cmd, 0, MAX_CMD); // clean memory in cmd // todo catch errors

        n_cmds++;
    }

    return n_cmds;
}

//! broken until fix
int WriteCodeTxt(const char * fout_name, char * prog_code, size_t n_cmds) {

    assert(fout_name);
    assert(prog_code);

    FILE * fout = fopen(fout_name, "wb");
    assert(fout);

    int cmd_id  = 0;
    int cmd_val = 0;

    for (int ip = 0; ip < n_cmds; ip++) {

        cmd_id  = prog_code[ip] >> 32;
        cmd_val = prog_code[ip] & 0xFFFFFFFF; // 32 zeros 32 1s

        if (cmd_val)
            fprintf(fout, "%d %d\n", cmd_id, cmd_val);
        else
            fprintf(fout, "%d\n", cmd_id);

        cmd_id  = 0;
        cmd_val = 0;
    }

    fclose(fout);

    return 0; // todo enum
}

int WriteCodeBin (const char * fout_name, int * prog_code, size_t n_cmds) {

    assert(fout_name);
    assert(prog_code);

    FILE * fout = fopen(fout_name, "wb");
    assert(fout);

    // put size in the beginning (this can be the beginning of signature-maker function}
    fwrite(&n_cmds, sizeof(size_t), 1, fout); // TODO check return val

    fwrite(prog_code, sizeof(*prog_code), n_cmds, fout); // TODO check return val

    fclose(fout);

    return 0; // TODO return enum
}

char * TokenizeText (char ** text, size_t n_lines) {

    assert(text);

    char * text_tokenized = (char *) calloc (n_lines * CMDS_PER_LINE * MAX_CMD, sizeof(char));
    char * text_tokenized_init = text_tokenized;

    size_t line_size = 0;

    for (size_t line = 0; line < n_lines; line++) {

        line_size = strlen(text[line]);

        strncpy(text_tokenized, text[line], line_size);

        text_tokenized += line_size;
        *text_tokenized = ' ';
        text_tokenized++;

    }
    *text_tokenized++ = 0;

    // free(text_tokenized); // todo free part we didnt use
    return text_tokenized_init;
}

static int EmitCodeArg (int ** prog_code_ptr, int code, int val) {

    assert (prog_code_ptr);
    assert (*prog_code_ptr);

    *(*prog_code_ptr)++ = code;

    *(*prog_code_ptr)++ = val;


    return 0;
}

static int EmitCodeNoArg (int ** prog_code_ptr, int code) {

    assert (prog_code_ptr);
    assert (*prog_code_ptr);

    *(*prog_code_ptr)++ = code;

    return 0;
}

