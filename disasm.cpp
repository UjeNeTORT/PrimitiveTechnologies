#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"

#define FPRINTF_LISTING_NOARG(stream, id, cmd, name, symbs)  \
    fprintf(stream, "(%lu) %d %n", id, cmd, &symbs);         \
    symbs = LISTING_CODE_TEXT_DISTANCE - symbs;              \
                                                             \
    for (int i = 0; i < symbs; i++)                          \
        fprintf(stream, " ");                                \
                                                             \
    fprintf(stream, "%s\n", name);                           \

#define FPRINTF_LISTING_JMP(stream, id, cmd, target, name, symbs)  \
    fprintf(stream, "(%lu) %d %d %n", id, cmd, target, &symbs);    \
    symbs = LISTING_CODE_TEXT_DISTANCE - symbs;                    \
                                                                   \
    for (int i = 0; i < symbs; i++)                                \
        fprintf(stream, " ");                                      \
                                                                   \
    fprintf(stream, "%s %d\n", name, target);                      \

#define FPRINTF_PUSH(stream, id, cmd, imm_arg, reg_id, name, symbs) \
    fprintf(stream, "(%lu) %d %n", id, cmd, &symbs);                \
    if (cmd & ARG_IMMED_VAL)                                        \
    {                                                               \
        fprintf(stream, "%d ", imm_arg);                            \
    }                                                               \
    if (cmd & ARG_REGTR_VAL)                                        \
    {                                                               \
        fprintf(stream, "%d ", reg_id);                             \
    }                                                               \
    symbs = LISTING_CODE_TEXT_DISTANCE - symbs;                     \
    for (int i = 0; i < symbs; i++)                                 \
        fprintf(stream, " ");                                       \
                                                                    \
    fprintf(stream, "%s ", name);                                   \
    if (cmd & ARG_MEMRY_VAL)                                        \
    {                                                               \
        fprintf(stream, "[");                                       \
    }                                                               \
    if (cmd & ARG_REGTR_VAL)                                        \
    {                                                               \
        fprintf(stream, "r%cx", 'a' + reg_id);                      \
        if (cmd & ARG_IMMED_VAL) {                                  \
            fprintf(stream, " + ");                                 \
        }                                                           \
    }                                                               \
    if (cmd & ARG_IMMED_VAL)                                        \
    {                                                               \
        fprintf(stream, "%d", imm_arg);                             \
    }                                                               \
    if (cmd & ARG_MEMRY_VAL)                                        \
    {                                                               \
        fprintf(stream, "]");                                       \
    }                                                               \
    fprintf(stream, "\n");

#define FPRINTF_POP(stream, id, cmd, imm_arg, reg_id, name, symbs)  \
    fprintf(stream, "(%lu) %d %n", id, cmd, &symbs);                \
    if (cmd & ARG_IMMED_VAL)                                        \
    {                                                               \
        fprintf(stream, "%d ", imm_arg);                            \
    }                                                               \
    if (cmd & ARG_REGTR_VAL)                                        \
    {                                                               \
        fprintf(stream, "%d ", reg_id);                             \
    }                                                               \
    symbs = LISTING_CODE_TEXT_DISTANCE - symbs;                     \
    for (int i = 0; i < symbs; i++)                                 \
        fprintf(stream, " ");                                       \
                                                                    \
    fprintf(stream, "%s ", name);                                   \
    if (cmd & ARG_MEMRY_VAL)                                        \
    {                                                               \
        fprintf(stream, "[");                                       \
    }                                                               \
    if (cmd & ARG_REGTR_VAL)                                        \
    {                                                               \
        fprintf(stream, "r%cx", 'a' + reg_id);                      \
        if (cmd & ARG_IMMED_VAL) {                                  \
            fprintf(stream, " + ");                                 \
        }                                                           \
    }                                                               \
    if (cmd & ARG_IMMED_VAL)                                        \
    {                                                               \
        fprintf(stream, "%d", imm_arg);                             \
    }                                                               \
    if (cmd & ARG_MEMRY_VAL)                                        \
    {                                                               \
        fprintf(stream, "]");                                       \
    }                                                               \
    fprintf(stream, "\n");

const int    LISTING_CODE_TEXT_DISTANCE = 20;
const char * DISASM_FILENAME = "disasmed.txt";

static int DisAssemble(const char * asm_fname, const char * out_fname);

int main() {

    fprintf(stdout, "\n"
                    "# Disassembler by NeTort, 2023\n"
                    "# Working...\n"
                    "# Like a surgeon carefully slicing flesh of byte code array and showing all its organs\n\n");

    DisAssemble(BIN_FILENAME, DISASM_FILENAME);

    return 0;
}

int DisAssemble(const char * asm_fname, const char * out_fname) {

    assert (asm_fname);
    assert (out_fname);

    FILE * asm_file = fopen(asm_fname, "rb");

    // read size of the long long byte code array
    size_t n_bytes = 0;
    fread(&n_bytes, sizeof(size_t), 1, asm_file);

    // read byte code array: form and fill prog_code array
    char * prog_code = (char *) calloc(n_bytes, sizeof(char));
    assert(prog_code);
    char * const prog_code_init = prog_code;

    size_t readen = 0;
    readen = fread(prog_code, sizeof(char), n_bytes, asm_file);
    assert(readen == n_bytes);

    fclose(asm_file);

    FILE * fout = fopen(out_fname, "wb");

    int val    = 0;
    int reg_id = 0;
    size_t ip  = 0;
    int symbs  = 0;

    while (ip < n_bytes)
    {
        switch (prog_code[ip] & OPCODE_MSK)
        {
            case CMD_HLT:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "hlt", symbs);
                ip++;
                break;
            }

            case CMD_PUSH:
            {
                val = *(int *)(prog_code + ip + 1);

                if (prog_code[ip] & ARG_IMMED_VAL)
                    reg_id = *(char *)(prog_code + ip + 1 + 4);
                else
                    reg_id = *(char *)(prog_code + ip + 1);

                FPRINTF_PUSH(fout, ip, prog_code[ip], val, reg_id, "push", symbs);

                if (prog_code[ip] & ARG_IMMED_VAL)
                    ip += sizeof(int);
                if (prog_code[ip] & ARG_REGTR_VAL)
                    ip += sizeof(char);

                ip += sizeof(char);

                break;
            }

            case CMD_POP:
            {
                val = *(int *)(prog_code + ip + 1);

                if (prog_code[ip] & ARG_IMMED_VAL)
                    reg_id = *(char *)(prog_code + ip + 1 + 4);
                else
                    reg_id = *(char *)(prog_code + ip + 1);

                FPRINTF_POP(fout, ip, prog_code[ip], val, reg_id, "pop", symbs);

                if (prog_code[ip] & ARG_IMMED_VAL)
                    ip += sizeof(int);
                if (prog_code[ip] & ARG_REGTR_VAL)
                    ip += sizeof(char);

                ip += sizeof(char);

                break;
            }

            case CMD_IN:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "in", symbs);
                ip++;

                break;
            }

            case CMD_OUT:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "out", symbs);
                ip++;

                break;
            }

            case CMD_ADD:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "add", symbs);
                ip++;

                break;
            }

            case CMD_SUB:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "sub", symbs);
                ip++;

                break;
            }

            case CMD_MUL:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "mul", symbs);
                ip++;

                break;
            }

            case CMD_SQRT:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "sqrt", symbs);
                ip++;

                break;
            }

            case CMD_DIV:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "div", symbs);
                ip++;

                break;
            }

            case CMD_CALL:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "call", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }

            case CMD_RET:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "ret", symbs);
                ip++;

                break;
            }

            case CMD_JMP:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "jmp", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }
            // IMM
            case CMD_JA:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "ja", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }
            case CMD_JAE:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "jae", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }

            case CMD_JB:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "jb", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }

            case CMD_JBE:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "jbe", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }

            case CMD_JE:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "je", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }

            case CMD_JNE:
            {
                val = *(int *)(prog_code + ip + 1);
                FPRINTF_LISTING_JMP(fout, ip, prog_code[ip], val, "jne", symbs);
                ip += sizeof(char);
                ip += sizeof(int);

                break;
            }

            default:
            {
                fprintf(stderr, "# Syntax Error! No command \"%d\" (%lu) found! Bye bye looser!\n", prog_code[ip], ip);

                return 1;
            }
        }
        val  = 0;
    }

    fclose(fout);
    free(prog_code_init);

    return 0;
}
