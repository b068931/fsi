# What is this?
This project is a reworked version of my private repository.
FSI (these letters literally mean nothing) is a programming language with a syntax that resembles an assembler, but with a bare-bones amount of instructions, so that it is not dependent on a particular architecture. But 'Engine' (a program that executes binary files) works only on x86-64 under Windows systems.

## Is it intended to solve some particular problem?
FSI has no purpose at all. I just wanted to write something that can generate machine instructions. Why? Because implementing your own solution is the best way to learn. In this case, I was trying to understand how a language compiler/translator (maybe it is not the best way to phrase it because of the way programs are executed - neither 'translator' nor 'engine' create .exe files) might work.

## How does it work?
1. 'translator.exe' generates a binary file that contains bytecode. (e.g. `translator source.text out.binary include-debug/no-debug`)
2. You pass this file to 'mediator.exe' that compiles it into x64 instructions and loads these instructions into the process' address space and executes them. (`mediator out.binary 4`. The last argument denotes the number of available executors.)
3. Write `okimdone` to exit from 'mediator.exe'. This will kill your program if it is still executing.

Alternatively:
1. Use make.bat. (e.g. `make source.text 4 no-debug`. The second argument - number of executors.)
2. Write `okimdone` to exit from 'mediator.exe'. This will kill your program if it is still executing.

The description of the internals of the source code files requires much more time than I have at the time of writing this README. You should look at the source code instead.

# Building the project
As you might have noticed by the .sln file this project is intended to be opened using Visual Studio. Instructions:
1. Open sln file.
2. Select build options (You should be able to choose from 'Debug', 'Release', or 'Release No .PDB .LIB .EXP')
3. Go to the output directory and call the executables from the command prompt or start your project directly from Visual Studio. You'll need to select dll\_mediator as your starting project.
