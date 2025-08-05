# FSI (From Scratch Interpreter)

> A toy programming language built from scratch to explore how software works at a lower level.

> Refer to "documentation\installation.txt" for installation instructions.

## 📋 Table of Contents
- [Introduction](#-introduction)
- [What is FSI?](#-what-is-fsi)
- [How It Works](#-how-it-works)
- [Project Structure](#-project-structure)
- [Getting Started](#-getting-started)
- [Current Limitations](#-current-limitations)

## 🚀 Introduction

Modern software is built on layers: hardware provides abstractions for the OS, the OS introduces concepts like files and processes, and applications rely on these foundations. As software grows more complex, many programs now depend on entire platforms like .NET rather than running as standalone binaries. While this makes development more efficient, it can also leave students wondering: **What is actually happening under the hood?**

I believe the best way to learn is through hands-on experience. To deepen my understanding of how software works at a lower level, I started working on a project called "FSI." This is a toy programming language designed to help me explore how software is built and executed. Since I wanted to stay motivated, I made sure to keep the design as simple as possible—prioritizing progress over perfection. 

## 🧩 What is FSI?

FSI is, at its core, a learning experiment. Everything I've discovered about compilers, interpreters, and machine code has been a byproduct of this project. While FSI is not designed for real-world use, its development was guided by several key principles inspired by established technologies:

### Design Principles

1. **Dual Representation (Text & Binary)**  
   FSI programs exist in both human-readable text and a binary format. This approach mirrors how languages like Java and C# use bytecode (JVM Bytecode, CIL) as an intermediate representation, making them platform-independent. Similarly, compilers like LLVM and GNU tools employ intermediate languages for optimization and flexibility. In FSI's case it serves more as a way to establish a clear separation of concerns between source code parsing and execution.

2. **Modular Execution Engine**  
   The execution engine is extremely modular. Core components are separated into dynamically linked libraries:
   - `execution-module`
   - `resource-module`
   - `program-loader`
   - `logger-module`
   - `prts` (Program RunTime Services)
   
   These modules communicate via a defined interface, theoretically allowing implementation in different languages and providing a way for FSI programs to interact with any module. In other words, I use C ABI with function arguments serialized in a string of characters with some type information preserved.

3. **Minimalist Language Design**  
   FSI was intentionally kept simple. Its structure resembles an intermediate representation rather than a high-level programming language, keeping the focus on understanding execution rather than language complexity.

4. **No Built-in Optimizations**  
   Unlike production compilers, FSI does not include optimization algorithms. Optimizations are a separate challenge that could be explored independently as a future project. For example, registers are "allocated" on a per-instruction basis: all local variables are stored in stack, moved to registers to perform one operation, then "spilled" to stack again. This is an extremely naive approach.

5. **Concurrency Support**  
   Concurrency is one of the most critical aspects of the implementation and arguably the most mature part of the system. Every module is fully reentrant, allowing concurrent access to all its functions.

## 🔄 Concurrency Model

In FSI, each program is represented by a "thread group." When you define the main function in your source code, the engine automatically creates a thread group and an initial thread. These program threads are then picked up by executors (which are essentially system threads).

If a thread blocks or finishes execution, control is transferred back to the engine, which schedules the next available program thread. Despite the complexity of this system, one surprising outcome is its stability—I have yet to encounter an engine crash caused by concurrency errors.

## ⚙️ How It Works

### 1. Translating the Code
The process begins with `translator.exe`, which converts the text-based source code into a binary format. This binary format is unique to FSI and is structured into segments called "runs." Each run stores specific components:
- Function bodies
- Function signatures
- Declared variables
- Module dependencies
- And so on...

This segmentation allows for an easy extensibility of the format.

### 2. Executing the Code
The generated binary is then passed to `mediator.exe`, which compiles it into x86-64 machine instructions and executes it. Before execution, `mediator.exe` loads and configures the modules specified in `modules.txt`.

The engine is not a monolithic system—various modules handle different responsibilities:
- **Execution Module**: Manages program threads and thread groups.
- **Resource Module**: Ensures proper cleanup of memory and resources after execution.
- **Program Loader**: Compiles the program and prepares it for execution by creating all necessary data structures.
- **Logger Module**: Provides logging capabilities for debugging and monitoring. It is also possible to compile engine modules with logging disabled. However, you should always keep this module installed.
- **Program Runtime Services (PRTS)**: Provides a clear interface for FSI programs to interact with the execution engine. Initially, FSI programs could call any function in the engine, but this was later restricted to a specific set of functions to prevent misuse and provide a clear interface.

These modular components interact dynamically to maintain execution flow.

### 3. Program Termination
The execution engine automatically stops when it detects that no active program threads remain. Blocked threads still count as "active" threads. One of the key areas of focus in FSI's development is cooperative multithreading, so it can't just kill threads arbitrarily.

## 📁 Project Structure

The FSI project consists of several modules:

- **bytecode_translator**: Converts source code to FSI binary format.
- **execution_module**: Handles thread management and program execution.
- **generic_parser**: Essentially a library for parsing modules.txt and FSI source code.
- **logger_module**: Provides logging capabilities.
- **module_mediator**: Orchestrates module interactions.
- **program_loader**: Loads translated programs.
- **program_runtime_services**: Provides runtime support for FSI programs.
- **resource_module**: Manages memory and resource allocation.
- **typename_array**: A thing for C++ template metaprogramming.

## 🏁 Getting Started

1. Clone this repository
2. Build the project using Visual Studio 2022
3. Create a simple FSI program. Or use "examples" directory.
4. Compile your program. 
5. Execute your program. You should check bytecode_translator/make.bat for the command line arguments. First is the name of the source file, second is the amount of system threads to use, and the third is the 'debug' section (provides the engine with names of used in your program, does not allow actual step-by-step debugging).

## ⚠️ Current Limitations

- No optimization capabilities
- Restricted language features
- Poor testing for such a complex system (Virtually none)
- Absolutely no documentation (Except this README)
