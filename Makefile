OPTIONS = -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations       \
-Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wconversion 						\
-Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers 			\
-Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel 			\
-Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE -fsanitize=address

ASM_TARGET = ./asm.exe
SPU_TARGET = ./spu.exe

COMPILER = g++
OBJ = my_hash.o stack.o onegin.o spu.o asm.o disasm.o

ASM_OPTS = --finname testcases/ex5.txt
SPU_OPTS = --finname translated.bin

start : stack.o onegin.o spu.o asm.o disasm.o
	$(COMPILER) onegin.o asm.o -o $(ASM_TARGET) $(OPTIONS)
	$(ASM_TARGET) $(ASM_OPTS)

	$(COMPILER) my_hash.o stack.o spu.o -o $(SPU_TARGET) $(OPTIONS)
	$(SPU_TARGET) $(SPU_OPTS)

stack.o :
	$(COMPILER) ./stacklib/stack.cpp -c -o stack.o
	$(COMPILER) ./stacklib/my_hash.cpp -c -o my_hash.o

onegin.o :
	$(COMPILER) ./text_processing_lib/text_buf.cpp -c -o onegin.o

spu.o :
	$(COMPILER) ./processor/spu.cpp -c -o spu.o

asm.o : spu.o
	$(COMPILER) ./assembler/asm.cpp -c -o asm.o

disasm.o : spu.o
	$(COMPILER) ./disassembler/disasm.cpp -c -o disasm.o

clean:
	rm -f $(ASM_TARGET)
	rm -f $(SPU_TARGET)
	rm -f *.o

