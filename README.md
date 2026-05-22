# Chip 8 Emulator and assembler

This is my attempt at making a CHIP-8 emulator, and an assembler for a kind of pseudoassembly I developed.

## Emulator

Copy a file into the rom directory. Then run the main function with a command-line argument of run, i.e. chip8 run.
You can also pass a file name as the second argument, e.g. chip8 run hello (Note that file names are given without extensions,
and the file extension should be .ch8).

If you encounter an error saying that the file cannot be opened, this is most likely because the executable itself will 
lie in one of the CMake build directories, and the script to copy your file to the correct place most likely has not run. 
In this case, go into the same directory as the executable, then make a directory called rom, then paste your file there.

## Assembler
Write your pseudoassembly code in the ./chip8-source directory. See hello-world.c8c for an example. Then run chip8 asm
to assemble the file (once again you can pass the file name as the second argument). The assembled file should appear
in the rom directory (the one next to the executable, not the one in the root). You should then be able to do chip8 run
<filename> to run your newly assembled file.
