/*
DEF_CMD (name, num, text,
                        spu_code,
                        have_arg,
                        code_have_arg <- for asm)
*/
DEF_CMD (HLT, 31, "hlt", {
                            PRINTF_INTERMED_INFO("# (%s - %3ld) Hlt encountered, goodbye!\n", "proc", ip_init);
                            return REACH_HLT;
                        },
                        0,
                        ;
)

DEF_CMD (PUSH, 1, "push", {
                            Elem_t arg = GetPushArg(prog_code, ip, spu->gp_regs, spu->RAM);

                            PushStack(&spu->stk, arg);

                            ip += CalcIpOffset(cmd);

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Push GetArg -> %d\n", "proc", ip_init, arg);

                        },
                        1,
                        {

                            ProcessPushArguments(prog_code, &n_bytes, &text);
                        }
)

DEF_CMD (POP, 2, "pop", {
                            if (cmd & ARG_TYPE_MSK)
                            {
                                Elem_t * arg_ptr = GetPopArgAddr(prog_code, ip, spu->gp_regs, spu->RAM);

                                if (arg_ptr == NULL)
                                {
                                    fprintf(stderr, "Processor Error! SetArg couldnt return stuff\n");
                                    abort();
                                }

                                pop_err = POP_NO_ERR;

                                *arg_ptr = PopStack(&spu->stk, &pop_err);

                                ip += CalcIpOffset(cmd);

                                PRINTF_INTERMED_INFO("# (%s - %3ld) Pop number to %p\n", "proc", ip_init, arg_ptr);
                            }
                            else
                            {
                                pop_err = POP_NO_ERR;
                                PopStack(&spu->stk, &pop_err);

                                PRINTF_INTERMED_INFO("# (%s - %3ld) Pop number\n", "proc", ip_init);

                                ip += CalcIpOffset(cmd);
                            }
                        },
                        1,
                        {
                            int  symbs  = 0;
                            char reg_id = 0;
                            int  val    = 0;

                            if (sscanf(text, "[ r%cx + %d ] %n", &reg_id, &val, &symbs) == 2 ||
                                sscanf(text, "[ %d + r%cx ] %n", &val, &reg_id, &symbs) == 2)
                            {
                                reg_id -= 'a';
                                if (!CorrectRegId(reg_id))
                                {
                                    fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed!\n", reg_id + 'a', reg_id + 'a');
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
                                    fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed!\n", reg_id + 'a', reg_id + 'a');
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
                                    fprintf(stderr, "Syntax Error! Register \"r%cx\" (%d) not allowed!\n", reg_id + 'a', reg_id + 'a');
                                    abort();
                                }
                                EmitCodeReg(prog_code, &n_bytes, ARG_REGTR_VAL | CMD_POP, reg_id);
                                text += symbs;
                            }
                            }
)

DEF_CMD (IN, 3, "in",   {
                        fprintf(stdout, "\n>> ");

                        fscanf(stdin, "%d", &val);

                        val *= STK_PRECISION;

                        PushStack(&spu->stk, val);

                        ip += CalcIpOffset(cmd);
                        },
                        0,
                        ;
)

DEF_CMD (OUT, 4, "out", {
                        pop_err = POP_NO_ERR;

                        val = PopStack(&spu->stk, &pop_err);

                        fprintf(stdout, "\n<< %.2f\n", (float) val / STK_PRECISION);

                        ip += CalcIpOffset(cmd);
                        },
                        0,
                        ;
)

DEF_CMD (ADD, 5, "add", {
                        pop_err = POP_NO_ERR;
                        val = PopStack(&spu->stk, &pop_err) + PopStack(&spu->stk, &pop_err);
                        PushStack(&spu->stk, val);

                        PRINTF_INTERMED_INFO("# (%s - %3ld) Add: %d\n", "proc", ip_init, val);

                        ip += CalcIpOffset(cmd);
                        },
                        0,
                        ;
)

DEF_CMD (SUB, 6, "sub", {
                        pop_err = POP_NO_ERR;
                        val -= PopStack(&spu->stk, &pop_err);
                        val += PopStack(&spu->stk, &pop_err);
                        PushStack(&spu->stk, val);

                        PRINTF_INTERMED_INFO("# (%s - %3ld) Sub: %d\n", "proc", ip_init, val);

                        ip += CalcIpOffset(cmd);
                        },
                        0,
                        ;
)

DEF_CMD (MUL, 7, "mul", {
                        pop_err = POP_NO_ERR;
                        val = MultInts(PopStack(&spu->stk, &pop_err), PopStack(&spu->stk, &pop_err));

                        PRINTF_INTERMED_INFO("# (%s - %3ld) Mul: %d\n", "proc", ip_init, val);

                        PushStack(&spu->stk, val);

                        ip += CalcIpOffset(cmd);
                        },
                        0,
                        ;
)

DEF_CMD (DIV, 8, "div", {
                        pop_err = POP_NO_ERR;

                        Elem_t denominator = PopStack(&spu->stk, &pop_err);
                        Elem_t numerator   = PopStack(&spu->stk, &pop_err);

                        val = 0;
                        val = DivideInts(numerator, denominator);

                        PushStack(&spu->stk, val);

                        PRINTF_INTERMED_INFO("# (%s - %3ld) Div: %d\n", "proc", ip_init, val);

                        ip += CalcIpOffset(cmd);
                        },
                        0,
                        ;
)

DEF_CMD (JMP, 9, "jmp", {
                        ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                        PRINTF_INTERMED_INFO("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
                        },
                        1,
                        {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JMP, cmd_ptr);
                        }
)

DEF_CMD (JA, 10, "ja",  {
                        int cmp_res = PopCmpTopStack(&spu->stk);

                        if (cmp_res > 0)
                        {
                            ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
                        }
                        else
                        {
                            ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                        }

                        },
                        1,
                        {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JA, cmd_ptr);
                        }
)

DEF_CMD (JAE, 11, "jae",  {
                        int cmp_res = PopCmpTopStack(&spu->stk);

                        if (cmp_res >= 0)
                        {
                            ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
                        }
                        else
                        {
                            ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                        }

                        },
                        1,
                        {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JAE, cmd_ptr);
                        }
)

DEF_CMD (JB, 12, "jb",  {
                        int cmp_res = PopCmpTopStack(&spu->stk);

                        if (cmp_res < 0)
                        {
                            ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
                        }
                        else
                        {
                            ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                        }

                        },
                        1,
                        {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JB, cmd_ptr);
                        }
)

DEF_CMD (JBE, 13, "jbe",  {
                        int cmp_res = PopCmpTopStack(&spu->stk);

                        if (cmp_res <= 0)
                        {
                            ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
                        }
                        else
                        {
                            ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                        }

                        },
                        1,
                        {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JBE, cmd_ptr);
                        }
)

DEF_CMD (JE, 14, "je",  {
                        int cmp_res = PopCmpTopStack(&spu->stk);

                        if (cmp_res == 0)
                        {
                            ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
                        }
                        else
                        {
                            ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                        }

                        },
                        1,
                        {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JE, cmd_ptr);
                        }
)

DEF_CMD (JNE, 15, "jne",  {
                        int cmp_res = PopCmpTopStack(&spu->stk);

                        if (cmp_res != 0)
                        {
                            ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Jmp to %lu\n", "proc", ip_init, ip);
                        }
                        else
                        {
                            ip += CalcIpOffset(cmd); // skip integer pointer to a position in code
                        }

                        },
                        1,
                        {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_JNE, cmd_ptr);
                        }
)

DEF_CMD (CALL, 16, "call", {
                            PushStack(&spu->call_stk, (Elem_t)(ip + sizeof(cmd_code_t) + sizeof(Elem_t)));

                            ip = *(Elem_t *)(prog_code + ip + sizeof(cmd_code_t));

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Call to %lu\n", "proc", ip_init, ip);
                           },
                           1,
                           {
                            int cmd_ptr = ProcessJmpArguments(&text, n_run, labels, n_lbls);

                            EmitCodeArg(prog_code, &n_bytes, ARG_IMMED_VAL | CMD_CALL, cmd_ptr);
                            }
)

DEF_CMD (RET, 17, "ret", {
                            pop_err = POP_NO_ERR;
                            ip = PopStack(&spu->call_stk, &pop_err);

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Ret to %lu\n", "proc", ip_init, ip);
                         },
                         0,
                         ;
)

DEF_CMD (SQRT, 18, "sqrt", {
                            pop_err = POP_NO_ERR;
                            val = PopStack(&spu->stk, &pop_err);

                            val = (int) sqrt(val * STK_PRECISION);

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Sqrt: %d\n", "proc", ip_init, val);

                            PushStack(&spu->stk, val);

                            ip += CalcIpOffset(cmd);
                          },
                          0,
                          ;
)

DEF_CMD (SQR, 19, "sqr", {
                            pop_err = POP_NO_ERR;
                            val = PopStack(&spu->stk, &pop_err);

                            val = val * val / STK_PRECISION;

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Sqr: %d\n", "proc", ip_init, val);

                            PushStack(&spu->stk, val);

                            ip += CalcIpOffset(cmd);
                          },
                          0,
                          ;
)

DEF_CMD (MOD, 20, "mod", {
                            pop_err = POP_NO_ERR;
                            Elem_t denominator = PopStack(&spu->stk, &pop_err);
                            Elem_t numerator   = PopStack(&spu->stk, &pop_err);

                            val = CalcMod(numerator, denominator);

                            PushStack(&spu->stk, val);

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Mod: %d\n", "proc", ip_init, val);

                            ip += CalcIpOffset(cmd);
                          },
                          0,
                          ;
)

DEF_CMD (IDIV, 21, "idiv", {
                            pop_err = POP_NO_ERR;
                            Elem_t denominator = PopStack(&spu->stk, &pop_err);
                            Elem_t numerator   = PopStack(&spu->stk, &pop_err);

                            val = CalcIdiv(numerator, denominator);

                            PushStack(&spu->stk, val);

                            PRINTF_INTERMED_INFO("# (%s - %3ld) Idiv: %d\n", "proc", ip_init, val);

                            ip += CalcIpOffset(cmd);
                          },
                          0,
                          ;
)

DEF_CMD (FRAME, 22, "frame", {
                                for (int i = 0; i < SPU_GRAM_HIGHT; i++, printf("\n"))
                                {
                                    for (int j = 0; j < SPU_GRAM_WIDTH; j++)
                                    {
                                        printf("%c%c", spu->RAM[GRAM_MAPPING + i * SPU_GRAM_HIGHT + j] / STK_PRECISION,
                                                       spu->RAM[GRAM_MAPPING + i * SPU_GRAM_HIGHT + j] / STK_PRECISION);
                                    }
                                }

                                ip += CalcIpOffset(cmd);
                        },
                        0,
                        ;
)
