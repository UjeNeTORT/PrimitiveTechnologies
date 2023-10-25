options = -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations  \
-Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion 		\
-Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers 		\
-Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel 		\
-Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE -fsanitize=address

DEFAULT = $(wildcard stacklib/*.cpp)
ASM = $(DEFAULT) asm.cpp $(wildcard text_processing_lib/*.cpp)
DISASM = $(DEFAULT) disasm.cpp
Main_files = $(DEFAULT)  spu.cpp $(wildcard text_processing_lib/*.cpp)

start: compile_asm compile_disasm compile_proc run_asm run_disasm run

compile_asm:
	g++ $(ASM) -o asm.exe $(options) $(debug)

compile_disasm:
	g++ $(DISASM) -o disasm.exe $(options) $(debug)

compile_proc:
	g++ $(Main_files) -o start.exe $(options) $(debug)

run_asm:
	./asm.exe

run_disasm:
	./disasm.exe

run:
	./start.exe
