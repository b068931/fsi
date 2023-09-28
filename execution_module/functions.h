#ifndef EXECUTION_CONTROL_FUNCTIONS_H
#define EXECUTION_CONTROL_FUNCTIONS_H

#include "pch.h"
#include "thread_local_structure.h"
#include "../dll_mediator/dll_part.h"

extern thread_local_structure* get_thread_local_structure();

extern "C" [[noreturn]] void load_execution_threadf(void*);
extern "C" void load_programf(void*, void*, size_t is_startup);
extern "C" return_value special_call_modulef();
extern "C" [[noreturn]] void resume_program_executionf(void*);

#endif