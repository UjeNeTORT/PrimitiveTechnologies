#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../enums.h"

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
const char * DISASM_FILENAME            = "disasmed.txt";

static size_t ReadByteCode (const char * in_fname, cmd_code_t ** prog_code);
static int    DisAssemble  (const cmd_code_t * prog_code, size_t n_bytes, const char * out_fname);

int main()
{
    fprintf(stdout, "\n"
                    "# Disassembler by NeTort, 2023\n"
                    "# Working...\n\n");

    cmd_code_t * prog_code = NULL;
    size_t n_bytes = ReadByteCode(BIN_FILENAME, &prog_code);

    DisAssemble(prog_code, n_bytes, DISASM_FILENAME);

    free(prog_code);

    return 0;
}

size_t ReadByteCode (const char * in_fname, cmd_code_t ** prog_code)
{
    assert(in_fname);
    assert(prog_code);

    FILE * in_file = fopen(in_fname, "rb");

    // read size of the long long byte code array
    size_t n_bytes = 0;
    fread(&n_bytes, sizeof(size_t), 1, in_file);

    // read byte code array: form and fill prog_code array
    *prog_code = (cmd_code_t *) calloc(n_bytes, sizeof(cmd_code_t));
    assert(*prog_code);

    size_t readen = 0;
    readen = fread(*prog_code, sizeof(cmd_code_t), n_bytes, in_file);
    assert(readen == n_bytes);

    fclose(in_file);

    return n_bytes;
}

int DisAssemble (const cmd_code_t * prog_code, size_t n_bytes, const char * out_fname)
{
    assert (prog_code);

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
                if (prog_code[ip] & ARG_IMMED_VAL)
                {
                    val = *(int *)(prog_code + ip + 1);
                    reg_id = *(char *)(prog_code + ip + 1 + 4);
                }
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


                if (prog_code[ip] & ARG_IMMED_VAL)
                {
                    memcpy(&val, prog_code + ip + 1, sizeof(int));
                    memcpy(&reg_id, prog_code + ip + 1 + 4, sizeof(char));
                }
                else
                    // reg_id = *(char *)(prog_code + ip + 1);
                    memcpy(&reg_id, prog_code + ip + 1, sizeof(char));

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

            case CMD_FRAME:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "frame", symbs);
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

            case CMD_DIV:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "div", symbs);
                ip++;

                break;
            }

            case CMD_SQRT:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "sqrt", symbs);
                ip++;

                break;
            }

            case CMD_SQR:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "sqr", symbs);
                ip++;

                break;
            }

            case CMD_MOD:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "mod", symbs);
                ip++;

                break;
            }

            case CMD_IDIV:
            {
                FPRINTF_LISTING_NOARG(fout, ip, prog_code[ip], "idiv", symbs);
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

    return 0;
}

// int DisasmPop(FILE * stream, cmd_code_t * prog_code, size_t ip, const char * name, int symbs)
// {
//     fprintf(stream, "(%lu) %d %n", id, cmd, &symbs);
//     if (cmd & ARG_IMMED_VAL)
//     {
//         fprintf(stream, "%d ", imm_arg);
//     }
//     if (cmd & ARG_REGTR_VAL)
//     {
//         fprintf(stream, "%d ", reg_id);
//     }
//     symbs = LISTING_CODE_TEXT_DISTANCE - symbs;
//     for (int i = 0; i < symbs; i++)
//         fprintf(stream, " ");

//     fprintf(stream, "%s ", name);
//     if (cmd & ARG_MEMRY_VAL)
//     {
//         fprintf(stream, "[");
//     }
//     if (cmd & ARG_REGTR_VAL)
//     {
//         fprintf(stream, "r%cx", 'a' + reg_id);
//         if (cmd & ARG_IMMED_VAL) {
//             fprintf(stream, " + ");
//         }
//     }
//     if (cmd & ARG_IMMED_VAL)
//     {
//         fprintf(stream, "%d", imm_arg);
//     }
//     if (cmd & ARG_MEMRY_VAL)
//     {
//         fprintf(stream, "]");
//     }
//     fprintf(stream, "\n");
// }
