options = -Wformat=2

asm_flags = --finname testcases/ex5.txt
spu_flags = --finname translated.bin

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
	processor/start.exe $(spu_flags)
