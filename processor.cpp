#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "processor.h"
#include "stack.h"
#include "text_buf.h"

const char * ASM_FILENAME = "ex1_translated.txt";
const int    CPU_STK_CAPA = 100;
const int    MAX_CMD_CODE = 8;

int *TokenizeCmdIntArr(const char **asm_lines_raw, size_t n_instructs);
int DivideInts(int numerator, int denominator);
int WordsNum(const char *str);

int main() {

    //================================================================== CREATE AND INITIALIZE CPU STRUCTURE

    CPU my_cpu = {};
    CtorStack(&my_cpu.stk, CPU_STK_CAPA);

    //=======================================================================================================

    // here we assume that compiler has finished its work and prepared file ASM_FILENAME for us

    //===================================================== FORM ARRAY WITH SEPARATE PSEUDO-ASM CODE STRINGS

    char        *buf           = NULL;
    const char **asm_lines_raw = NULL;
    int          n_instructs   = ReadText(ASM_FILENAME, &asm_lines_raw, &buf);

    //==================================================== FORM ARRAY OF INT: CMD_CODES AND ARGS ALL TOGETHER

    int * asm_nums = TokenizeCmdIntArr(asm_lines_raw, n_instructs);
    int * asm_nums_init = asm_nums;

    //======================== GO THROUGH THE ASM-LINES ARRAY AND DO ACTIONS CORRESPONDING TO THEIR CMD-CODES
    //-------------------------------- assume that pseudo-asm file has no mistakes in it (temporary solution)

    POP_OUT pop_err = POP_NO_ERR;
    int val = 0;
    int in_var = 0;

    for (int i_nstruct = 0; i_nstruct < n_instructs; i_nstruct++) {

        switch (*asm_nums) {

            case -1:                                                         // hlt

                fprintf(stderr, "hlt encountered, goodbye!\n");
                free(asm_nums_init);
                DtorStack(&my_cpu.stk);
                return 0;

            case 1:                                                         // push imm_val

                fprintf(stderr, "Push\n");
                PushStack(&my_cpu.stk, *++asm_nums);
                break;

            case 2:                                                         // pop

                fprintf(stderr, "Pop\n");
                pop_err = POP_NO_ERR;
                PopStack(&my_cpu.stk, &pop_err);
                break;

            case 3:                                                         // in

                fprintf(stderr, "In: please enter your variable...\n");
                in_var = 0;
                fscanf(stdin, "%d", &in_var);
                PushStack(&my_cpu.stk, in_var);
                break;

            case 4:                                                         // out

                pop_err = POP_NO_ERR;
                fprintf(stderr, "Out:   %d\n", PopStack(&my_cpu.stk, &pop_err));
                break;

            case 11:                                                        // add

                pop_err = POP_NO_ERR;
                val = 0;
                val = PopStack(&my_cpu.stk, &pop_err) + PopStack(&my_cpu.stk, &pop_err);
                PushStack(&my_cpu.stk, val);
                break;

            case 12:                                                        // sub

                pop_err = POP_NO_ERR;
                val = 0;
                val -= PopStack(&my_cpu.stk, &pop_err);
                val += PopStack(&my_cpu.stk, &pop_err);
                PushStack(&my_cpu.stk, val);
                break;

            case 13:                                                        // mul

                pop_err = POP_NO_ERR;
                val = 0;
                val =  PopStack(&my_cpu.stk, &pop_err) * PopStack(&my_cpu.stk, &pop_err);
                PushStack(&my_cpu.stk, val);
                break;

            case 14:                                                        // div

                pop_err = POP_NO_ERR;
                int denominator = PopStack(&my_cpu.stk, &pop_err);
                int numerator =  PopStack(&my_cpu.stk, &pop_err);
                val = 0;
                val = DivideInts(numerator, denominator);
                PushStack(&my_cpu.stk, val);
                break;
        }

        asm_nums++;

    }

    free(asm_nums_init);
    DtorStack(&my_cpu.stk);

    return 0;
}

int WordsNum(const char *str) {

    int n_words = 0;

    int inside_word = 0;

    for (int i = 0; str[i] != 0 && str[i] != '\n' && str[i] != EOF; i++) {

        if (!isspace(str[i])) {
            inside_word = 1;
        }

        else if (inside_word == 1) {
            n_words++;
            inside_word = 0;
        }
    }

    return n_words;
}


int *TokenizeCmdIntArr(const char **asm_lines_raw, size_t n_instructs) {

    stack asm_codes = {};
    CtorStack(&asm_codes, n_instructs * 3);

    int n_words = 0; // i mean integer numbers (cmd codes or cmd args)

    char * asm_line_cpy = (char *) calloc(MAX_CMD_CODE, 1);

    for (int i = 0; i < n_instructs; i++) {

        strcpy(asm_line_cpy, asm_lines_raw[i]);

        char * token = strtok(asm_line_cpy, " ");

        while(token != NULL) {
            n_words++;
            PushStack(&asm_codes, atoi(token));

            token = strtok(NULL, " ");
        }
    }
    free(asm_line_cpy);

    int *asm_lines = (int *) calloc(n_words, sizeof(int));

    for (int i = 0; i < n_words; i++) {
        asm_lines[i] = asm_codes.data.buf[i];
    }

    DtorStack(&asm_codes);

    return asm_lines;
 }

 int DivideInts(int numerator, int denominator) {
    return (int) numerator / denominator;
 }
