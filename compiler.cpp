#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "commands.h"
#include "text_buf.h"

int notmain() {

    AssembleMath("ex1.txt", "ex1_translated.txt", "user_commands.txt");

    return 0;
}

/**
 * @brief change commands from fin_name to their codes (from usr_cmd) fout_name. cmd_arr of usr_cmd structs is formed using ParseCmdNames func
*/
enum ASM_OUT AssembleMath(const char * fin_name, const char * fout_name, const char * cmds_file) {

    assert(fin_name);
    assert(fout_name);

    char *        in_buf  = NULL;
    const char ** in_text = NULL;

    int n_lines = ReadText(fin_name, &in_text, &in_buf);

    int n_cmds = 0;
    usr_cmd * cmds_arr = ParseCmdNames(cmds_file, &n_cmds);

    char * curr_cmd_name = (char *) calloc(1, MAX_CMD);
    int    cmd_val       = 0;
    int    prev_val      = 0;
    int    scan_res      = __INT_MAX__;

    FILE * fout = fopen(fout_name, "w");

    for (int nline = 0; nline < n_lines; nline++) {

        scan_res = sscanf(in_text[nline], "%s %d", curr_cmd_name, &cmd_val);

        if (scan_res == 2 || scan_res == 1) {

            for (int cmd_cnt = 0; cmd_cnt < n_cmds; cmd_cnt++) {

                if (strcmp(curr_cmd_name, cmds_arr[cmd_cnt].name) == 0) {

                    if (scan_res == 2)
                        fprintf(fout, "%d %d\n", cmds_arr[cmd_cnt].code, cmd_val);
                    else
                        fprintf(fout, "%d\n",    cmds_arr[cmd_cnt].code);

                    break;
                }

                if (cmd_cnt == n_cmds - 1) {

                    fprintf(stderr, "AssembleMath: command name (%s) not found in \"%s\"\n", curr_cmd_name, cmds_file);
                    fclose(fout);
                    abort();
                }
            }
        }

        else if (scan_res == EOF || scan_res == 0) {
            break;
        }

        else {
            fprintf(stderr, "AssembleMath: couldnt read anything from string %d\n", nline);
            fclose(fout);
            abort();
        }

    }

    fclose(fout);

    free(curr_cmd_name);

    return ASM_OUT_NO_ERR;
}

/**
 * @brief parse text file where commands are stored and put them in the cmd_arr array usr_cmd structs
*/
usr_cmd * ParseCmdNames(const char * filename, int * n_cmds) {

    assert(filename);

    char *        cmd_buf = NULL;
    const char ** text    = NULL;

    int n_lines = ReadText(filename, &text, &cmd_buf);

    usr_cmd curr_cmd = CmdCtor();

    usr_cmd * cmd_arr = (usr_cmd *) calloc(n_lines, sizeof(usr_cmd));
    if (!cmd_arr) {
        fprintf(stderr, "ParseCommands: CANT ALLOCATE MEMORY FOR COMMANDS ARRAY\n");
        return NULL;
    }

    size_t  cmd_cnt  = 0;

    for (int nline = 0; nline < n_lines; nline++) {

        int scan_res = sscanf(text[nline], "%s %d", curr_cmd.name, &curr_cmd.code);

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

int ForbiddenCmdCode(int code) {
    // todo
    return 0;
}

usr_cmd CmdCtor() {

    char * name = (char *) calloc(1, MAX_CMD);
    int    code = 0;

    return {name, code};
}

