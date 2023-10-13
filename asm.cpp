#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"
#include "commands.h"
#include "text_buf.h"

static int EmitCodeArg (long long * prog_code);
static int EmitCode    (long long * prog_code);

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

    int buf_size = 0;

    long long n_lines = ReadText(fin_name, &in_text, &in_buf, &buf_size);

    PreprocessProgram(in_text, n_lines);

    long long * prog_code = (long long *) calloc(n_lines, sizeof(long long));

    TranslateProgram(in_text, n_lines, prog_code);


    WriteCodeTxt(fout_name, prog_code, n_lines);

    WriteCodeBin("translated.bin", prog_code, n_lines); // TODO filename to vars

    // TODO to func
    free(prog_code);
    free(in_buf);
    free(in_text);

    return ASM_OUT_NO_ERR;
}

int PreprocessProgram (char ** text, int n_lines) {

    assert(text);

    //====================== DEL COMMENTS =======================
    char * comm_pos = 0;

    for (int i = 0; i < n_lines; i++) {

        comm_pos = strchr(text[i], ';');
        if (comm_pos)
            *comm_pos = '\0';
    }

    return 0;
}

//* works only with preprocessed program
int TranslateProgram (char ** text_ready, int n_lines, long long * prog_code) {

    assert(text_ready);
    assert(*text_ready);
    assert(prog_code);

    //====================== GET ALL THE COMMANDS DATA FROM ARRAY FROM COMMANDS_H =======================

    int n_cmds = sizeof(CMDS_ARR) / sizeof(*CMDS_ARR);

    long long
         cmd_val  = 0;
    int  scan_res = __INT_MAX__;
    int  cmd_id   = 0;
    char reg_letr = 0;
    int  reg_id   = 0;

    char * curr_cmd_name = (char *) calloc(1, MAX_CMD);            // temporary variable supposed to contain keyword (push pop etc)
    char * curr_cs       = (char *) calloc(MAX_CMD, sizeof(char)); // contains text line

    for (int pos = 0; pos < n_lines; pos++) {

        strncpy(curr_cs, text_ready[pos], MAX_CMD);

        // ========================= LOOKING FOR "PUSH RCX" STRINGS =========================

        if (sscanf(curr_cs, "%s r%cx", curr_cmd_name, &reg_letr) == 2) {

            reg_id = reg_letr - 'a' + 1; // todo to func

            if (reg_id > 4) {

                fprintf(stderr, "only 4 registers allowed! (%d)\n", reg_id);
                abort();
            }

            cmd_val = reg_id;
            cmd_id |= GetCmdCode(CMDS_ARR, curr_cmd_name, n_cmds); // todo if 0 abort
            cmd_id |= ARG_REGTR_VAL;
        }

        // ===================== LOOKING FOR "PUSH 513"-LIKE STRINGS =========================

        else if (sscanf(curr_cs, "%s %lld", curr_cmd_name, &cmd_val) == 2) {

            cmd_id |= GetCmdCode(CMDS_ARR, curr_cmd_name, n_cmds);
            cmd_id |= ARG_IMMED_VAL;
        }
        else if (sscanf(curr_cs, "%s", curr_cmd_name) == 1) {

            cmd_id |= GetCmdCode(CMDS_ARR, curr_cmd_name, n_cmds);
        }

        // ============================== NONE OF THESE MATCH ===============================

        else {
            fprintf(stderr, "AssembleMath: invalid input, string: \"%s\"\n", text_ready[pos]);

            free(curr_cmd_name);
            free(curr_cs);
            return ASM_OUT_ERR;
        }

        // ========== PUT CMD_ID AND CMD_VAL IN LONG LONG CELL IN ARRAY CODE_SEG ===========

        prog_code[pos] = cmd_id;
        prog_code[pos] <<= 32;
        prog_code[pos] |= cmd_val;

        cmd_id  = 0;
        cmd_val = 0;
        reg_id  = 0;

    }

    free(curr_cmd_name);
    free(curr_cs);

    return ASM_OUT_NO_ERR;
}

int WriteCodeTxt(const char * fout_name, long long * prog_code, long long prog_code_lines) {

    assert(fout_name);
    assert(prog_code);

    FILE * fout = fopen(fout_name, "wb");
    assert(fout);

    int cmd_id  = 0;
    int cmd_val = 0;

    for (int ip = 0; ip < prog_code_lines; ip++) {

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

int WriteCodeBin (const char * fout_name, long long * prog_code, long long prog_code_lines) {

    assert(fout_name);
    assert(prog_code);

    FILE * fout = fopen(fout_name, "wb");
    assert(fout);

    // put size in the beginning (this can be the beginning of signature-maker function}
    fwrite(&prog_code_lines, sizeof(*prog_code), 1, fout); // TODO check return val

    fwrite(prog_code, sizeof(*prog_code), prog_code_lines, fout); // TODO check return val

    fclose(fout);

    return 0; // TODO return enum
}

/**
 * @brief parse text file where commands are stored and put them in the cmd_arr array usr_cmd structs
 *
 * UNUSED FUCTION
*/
usr_cmd * ParseCmdNames(const char * filename, int * n_cmds) {

    assert(filename);

    char *  cmd_buf = NULL;
    char ** text    = NULL;

    int buf_size = 0;
    int n_lines = ReadText(filename, &text, &cmd_buf, &buf_size);

    usr_cmd curr_cmd = CmdCtor();

    usr_cmd * cmd_arr = (usr_cmd *) calloc(n_lines, sizeof(usr_cmd));
    if (!cmd_arr) {

        fprintf(stderr, "ParseCommands: CANT ALLOCATE MEMORY FOR COMMANDS ARRAY\n");
        return NULL;
    }

    size_t  cmd_cnt  = 0;
    int scan_res     = __INT_MAX__;

    for (int nline = 0; nline < n_lines; nline++) {

        scan_res = sscanf(text[nline], "%s %d", curr_cmd.name, &curr_cmd.code);

        int prev_cmd_code = 0;

        if (scan_res == 2) {

            prev_cmd_code = curr_cmd.code;

            if (!isalpha(curr_cmd.name[0]) && curr_cmd.name[0] != '_') {

                fprintf(stderr, "ParseCommands: invalid command name %s\n", curr_cmd.name);
                abort();
                // todo load default config file
            }
            else if (ForbiddenCmdCode(curr_cmd.code)) {

                fprintf(stderr, "ParseCommands: invalid command code %d\n", curr_cmd.code);
                abort();
                // todo load default config file
            }
            else {

                cmd_arr[cmd_cnt++] = curr_cmd;
            }
        }

        else if (scan_res == 1) {

            if (!isalpha(curr_cmd.name[0]) && curr_cmd.name[0] != '_') {

                fprintf(stderr, "ParseCommands: invalid command name %s\n", curr_cmd.name);
                abort();
                // todo load default config file
            }
            else {

                curr_cmd.code      = ++prev_cmd_code;
                cmd_arr[cmd_cnt++] =   curr_cmd;
            }

        }
        else if (scan_res == 0) {

            ;
        }
        else if (scan_res == EOF) {

            break;
        }
        else {

            fprintf(stderr, "ParseCommands: unknown error occured\n");
            abort();
        }

        curr_cmd = CmdCtor();
    }

    *n_cmds = cmd_cnt;
    return cmd_arr;
}

int GetCmdCode(const usr_cmd * cmd_arr, const char * cmd_name, int cmd_arr_size) {

    for (int i = 0; i < cmd_arr_size; i++) {
        if (strcmp(cmd_arr[i].name, cmd_name) == 0)
            return cmd_arr[i].code;
    }

    return 0;
}

int ForbiddenCmdCode(int code) {
    // todo
    return 0;
}

usr_cmd CmdCtor() {

    char * name = (char *) calloc(1, MAX_CMD);
    int    code = 0;

    return {name, code};
}

