# Processor Assembler and Disassembler
## How to install and run 
1. ``` git clone https://github.com/UjeNeTORT/Processor ```
2. You now have to create txt file with your program and run it with processor, to see list of available commands open "user_commands.txt" (it may not be up to date because i have to edit it manually each time i add command, and i sometimes forget to do it) 
3. In your freshly cloned repository there is a MakeFile, open it and type in ```asm_flags``` path to your txt file
4. ``` make ```
## Examples
Examples are available in ```testcases``` folder.
The most remarkable are:
ex5.txt - factorial calculator
ex6.txt - draws in VRAM circle of variable size
ex8.txt - solves quadratic equation
## Notes
- All values are stored in integer stack. To change calculation precision we store all values multiplied by ```STK_PRECISION```. It leads to reduce in max integer you can store in stack. For example, if you try to run ex5.txt (calculate n!) for n = 10 you will get < 0 value due to overflow.
- VRAM is mapped on RAM. If you are to use VRAM, make sure you address correct position in RAM. ```VRAM_MAPPING``` stores position from which VRAM is mapped on RAM. Commands to direct interaction with VRAM are not implemented yet, so you have to manually set offset for addressing VRAM (example is in the beginning of ex6.txt).
- Program will run only if you love cats! Make sure you fed at least 2 cats yesterday. If not, pet a cat immediately before running! 
