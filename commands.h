/*
DEF_CMD (name, num, text,
                        spu_code,
                        have_arg,
                        code_have_arg <- for asm,
                        disasm_code)
*/


#define SPU_CODE
#define ASM_CODE
#define DISASM_CODE

#define ASSERT_REG_ID(reg_id)                                                                                   \
    if (!CorrectRegId(reg_id))                                                                                  \
        {                                                                                                       \
            fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed!\n", reg_id + 'a', reg_id + 'a'); \
            abort();                                                                                            \
        }                                                                                                       \

#define INCR_IP(cmd)         \
    ip += CalcIpOffset(cmd); \

#define PUSH(val)               \
    PushStack(&spu->stk, val);  \

#define POP()                       \
    PopStack(&spu->stk, &pop_err)   \

#define JMP(ip)                                          \
    *(Elem_t *)(prog_code + (ip) + sizeof(cmd_code_t));  \

// ===============================================================================================
DEF_CMD (HLT, 31, "hlt", 0,

SPU_CODE
    {
        printf_intermed_info("# (%s - %3ld) Hlt encountered, goodbye!\n", "proc", ip_init);

        return REACH_HLT;
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "hlt");
    }
)

// ===============================================================================================
DEF_CMD (PUSH, 1, "push", 1,

SPU_CODE
    {
        Elem_t arg = GetPushArg(prog_code, ip, spu->gp_regs, spu->RAM);

        PUSH(arg);

        INCR_IP(cmd);

        printf_intermed_info("# (%s - %3ld) Push GetArg -> %d\n", "proc", ip_init, arg);
    },

ASM_CODE
    {
        ProcessPushArguments(prog_code, &n_bytes, &text);
    },

DISASM_CODE
    {
        fprintf_push(fout, prog_code, ip, "push");
    }
)

// ===============================================================================================
DEF_CMD (POP, 2, "pop", 1,

SPU_CODE
    {
        if (cmd & ARG_TYPE_MSK)
        {
            Elem_t * arg_ptr = GetPopArgAddr(prog_code, ip, spu->gp_regs, spu->RAM);

            if (arg_ptr == NULL)
            {
                fprintf(stderr, "Processor Error! SetArg couldnt return stuff\n");
                abort();
            }

            pop_err = POP_NO_ERR;
            *arg_ptr = POP();

            printf_intermed_info("# (%s - %3ld) Pop number to %p\n", "proc", ip_init, arg_ptr);

            INCR_IP(cmd);
        }
        else
        {
            pop_err = POP_NO_ERR;
            POP();

            printf_intermed_info("# (%s - %3ld) Pop number\n", "proc", ip_init);

            INCR_IP(cmd);
        }
    },

ASM_CODE
    {
        ProcessPopArguments(prog_code, &n_bytes, &text);
    },

DISASM_CODE
    {
        fprintf_pop(fout, prog_code, ip, "pop");
    }
)

// ===============================================================================================
DEF_CMD (IN, 3, "in", 0,

SPU_CODE
    {
        fprintf(stdout, "\n>> ");

        if (fscanf(stdin, "%d", &val) != 1)
        {
            fprintf(stderr, "Value for \"in\" is not given or incorrect\n");
            abort();
        }

        val *= STK_PRECISION;

        PUSH(val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "in");
    }
)

// ===============================================================================================
DEF_CMD (OUT, 4, "out", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;

        fprintf(stdout, "\n<< %.2f\n", (float) POP() / STK_PRECISION);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "out");
    }
)

// ===============================================================================================
DEF_CMD (ADD, 5, "add", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;

        val = POP() + POP();
        PUSH(val);

        printf_intermed_info("# (%s - %3ld) Add: %d\n", "proc", ip_init, val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "add");
    }
)

// ===============================================================================================
DEF_CMD (SUB, 6, "sub", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;
        val -= POP();
        val += POP();
        PUSH(val);

        printf_intermed_info("# (%s - %3ld) Sub: %d\n", "proc", ip_init, val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "sub");
    }
)

// ===============================================================================================
DEF_CMD (MUL, 7, "mul", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;
        val = MultInts(POP(), POP());

        printf_intermed_info("# (%s - %3ld) Mul: %d\n", "proc", ip_init, val);

        PUSH(val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "mul");
    }
)

// ===============================================================================================
DEF_CMD (DIV, 8, "div", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;

        Elem_t denominator = POP();
        Elem_t numerator   = POP();

        val = 0;
        val = DivideInts(numerator, denominator);

        PUSH(val);

        printf_intermed_info("# (%s - %3ld) Div: %d\n", "proc", ip_init, val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "div");
    }
)

// ===============================================================================================
DEF_CMD (JMP, 9, "jmp", 1,

SPU_CODE
    {
        ip = JMP(ip);

        printf_intermed_info("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JMP, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "jmp");
    }
)

// ===============================================================================================
DEF_CMD (JA, 10, "ja", 1,

SPU_CODE
    {
        int cmp_res = PopCmpTopStack(&spu->stk);

        if (cmp_res > 0)
        {
            ip = JMP(ip);

            printf_intermed_info("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
        }
        else
        {
            INCR_IP(cmd);
        }

    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JA, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "ja");
    }
)

// ===============================================================================================
DEF_CMD (JAE, 11, "jae", 1,

SPU_CODE
    {
        int cmp_res = PopCmpTopStack(&spu->stk);

        if (cmp_res >= 0)
        {
            ip = JMP(ip);

            printf_intermed_info("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
        }
        else
        {
            INCR_IP(cmd);
        }

    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JAE, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "jae");
    }
)

// ===============================================================================================
DEF_CMD (JB, 12, "jb", 1,

SPU_CODE
    {
        int cmp_res = PopCmpTopStack(&spu->stk);

        if (cmp_res < 0)
        {
            ip = JMP(ip);

            printf_intermed_info("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
        }
        else
        {
            INCR_IP(cmd);
        }

    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JB, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "jb");
    }
)

// ===============================================================================================
DEF_CMD (JBE, 13, "jbe", 1,

SPU_CODE
    {
        int cmp_res = PopCmpTopStack(&spu->stk);

        if (cmp_res <= 0)
        {
            ip = JMP(ip);

            printf_intermed_info("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
        }
        else
        {
            INCR_IP(cmd);
        }
    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JBE, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "jbe");
    }
)

// ===============================================================================================
DEF_CMD (JE, 14, "je", 1,

SPU_CODE
    {
        int cmp_res = PopCmpTopStack(&spu->stk);

        if (cmp_res == 0)
        {
            ip = JMP(ip);

            printf_intermed_info("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
        }
        else
        {
            INCR_IP(cmd);
        }
    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JE, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "je");
    }
)

// ===============================================================================================
DEF_CMD (JNE, 15, "jne", 1,

SPU_CODE
    {
        int cmp_res = PopCmpTopStack(&spu->stk);

        if (cmp_res != 0)
        {
            ip = JMP(ip);

            printf_intermed_info("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
        }
        else
        {
            INCR_IP(cmd);
        }
    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JNE, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "jne");
    }
)

// ===============================================================================================
DEF_CMD (CALL, 16, "call", 1,

SPU_CODE
    {
        PushStack(&spu->call_stk, (Elem_t)(ip + sizeof(cmd_code_t) + sizeof(Elem_t)));

        ip = JMP(ip);

        printf_intermed_info("# (%s - %3ld) Call to %lu\n", "proc", ip_init, ip);
    },

ASM_CODE
    {
        int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

        EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_CALL, cmd_ptr);
    },

DISASM_CODE
    {
        fprintf_listing_jmp(fout, prog_code, ip, "call");
    }
)

// ===============================================================================================
DEF_CMD (RET, 17, "ret", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;
        ip = PopStack(&spu->call_stk, &pop_err);

        printf_intermed_info("# (%s - %3ld) Ret to %lu\n", "proc", ip_init, ip);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "ret");
    }
)

// ===============================================================================================
DEF_CMD (SQRT, 18, "sqrt", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;
        val = POP();

        val = (int) sqrt(val * STK_PRECISION);

        printf_intermed_info("# (%s - %3ld) Sqrt: %d\n", "proc", ip_init, val);

        PUSH(val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "sqrt");
    }
)

// ===============================================================================================
DEF_CMD (SQR, 19, "sqr", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;
        val = POP();

        val = val * val / STK_PRECISION;

        printf_intermed_info("# (%s - %3ld) Sqr: %d\n", "proc", ip_init, val);

        PUSH(val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "sqr");
    }
)

// ===============================================================================================
DEF_CMD (MOD, 20, "mod", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;
        Elem_t denominator = POP();
        Elem_t numerator   = POP();

        val = CalcMod(numerator, denominator);

        PUSH(val);

        printf_intermed_info("# (%s - %3ld) Mod: %d\n", "proc", ip_init, val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "mod");
    }
)

// ===============================================================================================
DEF_CMD (IDIV, 21, "idiv", 0,

SPU_CODE
    {
        pop_err = POP_NO_ERR;
        Elem_t denominator = POP();
        Elem_t numerator   = POP();

        val = CalcIdiv(numerator, denominator);

        PUSH(val);

        printf_intermed_info("# (%s - %3ld) Idiv: %d\n", "proc", ip_init, val);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "idiv");
    }
)

// ===============================================================================================
DEF_CMD (FRAME, 22, "frame", 0,

SPU_CODE
    {
        ShowFrame(spu);

        INCR_IP(cmd);
    },

ASM_CODE
    {
        ;
    },

DISASM_CODE
    {
        fprintf_listing_no_arg(fout, prog_code, ip, "frame");
    }
)
