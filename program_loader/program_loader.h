#ifndef PROGRAM_LOADER
#define PROGRAM_LOADER

#include "pch.h"
#include "declarations.h"

COMPILERMODULE_API module_mediator::return_value load_program_to_memory(module_mediator::arguments_string_type bundle);
COMPILERMODULE_API module_mediator::return_value free_program(module_mediator::arguments_string_type bundle);

COMPILERMODULE_API module_mediator::return_value check_function_arguments(module_mediator::arguments_string_type bundle);
COMPILERMODULE_API module_mediator::return_value get_function_name(module_mediator::arguments_string_type bundle);

COMPILERMODULE_API void initialize_m(module_mediator::dll_part* part);
COMPILERMODULE_API void free_m();

#endif // !COMPILER_MODULE