#ifndef RESOURCE_MODULE
#define RESOURCE_MODULE

#include "pch.h"
#include "../module_mediator/module_part.h"

#ifdef RESOURCEMODULE_EXPORTS
#define RESOURCEMODULE_API extern "C" __declspec(dllexport)
#else
#define RESOURCEMODULE_API extern "C" __declspec(dllimport) 
#endif

RESOURCEMODULE_API module_mediator::return_value add_container_on_destroy(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value add_thread_on_destroy(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value duplicate_container(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value get_preferred_stack_size(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value create_new_program_container(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value create_new_thread(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value allocate_program_memory(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value allocate_thread_memory(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value deallocate_program_memory(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value deallocate_thread_memory(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value deallocate_program_container(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value deallocate_thread(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value get_running_threads_count(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value get_program_container_id(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value get_jump_table(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value get_jump_table_size(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value verify_thread_memory(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value verify_program_memory(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API void initialize_m(module_mediator::module_part*);
RESOURCEMODULE_API void free_m();

#endif // !RESOURCE_MODULE
