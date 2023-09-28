#ifndef EXECUTION_MODULE
#define EXECUTION_MODULE

#include "pch.h"

#ifdef EXECUTIONMODULE_EXPORTS
#define EXECUTIONMODULE_API extern "C" __declspec(dllexport)
#else
#define EXECUTIONMODULE_API extern "C" __declspec(dllimport)
#endif // EXECUTIONMODULE_EXPORTS

EXECUTIONMODULE_API return_value on_container_creation(arguments_string_type bundle);
EXECUTIONMODULE_API return_value on_thread_creation(arguments_string_type bundle);

EXECUTIONMODULE_API return_value self_duplicate(arguments_string_type bundle);
EXECUTIONMODULE_API return_value self_priority(arguments_string_type bundle);
EXECUTIONMODULE_API return_value get_thread_saved_variable(arguments_string_type bundle);
EXECUTIONMODULE_API return_value dynamic_call(arguments_string_type bundle);

EXECUTIONMODULE_API return_value self_block(arguments_string_type);
EXECUTIONMODULE_API return_value get_current_thread_id(arguments_string_type bundle);
EXECUTIONMODULE_API return_value get_current_thread_group_id(arguments_string_type bundle);
EXECUTIONMODULE_API return_value make_runnable(arguments_string_type bundle);
EXECUTIONMODULE_API return_value start(arguments_string_type bundle);
EXECUTIONMODULE_API return_value create_thread(arguments_string_type bundle);
EXECUTIONMODULE_API return_value run_program(arguments_string_type bundle);

EXECUTIONMODULE_API void initialize_m(dll_part*);

#endif // !EXECUTION_MODULE_H
