#ifndef PROGRAM_LOADER
#define PROGRAM_LOADER

#include "pch.h"
#include "declarations.h"

COMPILERMODULE_API return_value load_program_to_memory(arguments_string_type bundle);
COMPILERMODULE_API return_value free_program(arguments_string_type bundle);

COMPILERMODULE_API return_value check_function_arguments(arguments_string_type bundle);
COMPILERMODULE_API return_value get_function_name(arguments_string_type bundle);

COMPILERMODULE_API void initialize_m(dll_part* part);
COMPILERMODULE_API void free_m();

#endif // !COMPILER_MODULE