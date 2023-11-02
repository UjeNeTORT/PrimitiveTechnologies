options = -O2 -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations  \
-Wmissing-include-dirs -Wswitch-enum -Wswitch-default -pipe -fexceptions           						\
-Wctor-dtor-privacy -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers 							\
-Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel 			\
-Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE -fsanitize=address

asm_flags = --finname testcases/ex7.txt

DEFAULT = $(wildcard stacklib/*.cpp)
ASM = $(DEFAULT) assembler/asm.cpp $(wildcard text_processing_lib/*.cpp)
DISASM = $(DEFAULT) disassembler/disasm.cpp
Main_files = $(DEFAULT)  processor/spu.cpp $(wildcard text_processing_lib/*.cpp)

start: compile_asm compile_disasm compile_proc run_asm run_disasm run

compile_asm:
	g++ $(ASM) -o assembler/asm.exe $(options) $(debug)

compile_disasm:
	g++ $(DISASM) -o disassembler/disasm.exe $(options) $(debug)

compile_proc:
	g++ $(Main_files) -o processor/start.exe $(options) $(debug)

run_asm:
	assembler/asm.exe $(asm_flags)

run_disasm:
	disassembler/disasm.exe

run:
	processor/start.exe
