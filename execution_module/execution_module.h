#ifndef EXECUTION_MODULE
#define EXECUTION_MODULE

#include "../module_mediator/module_part.h"

#ifdef EXECUTIONMODULE_EXPORTS
#define EXECUTIONMODULE_API extern "C" __declspec(dllexport)
#else
#define EXECUTIONMODULE_API extern "C" __declspec(dllimport)
#endif // EXECUTIONMODULE_EXPORTS

EXECUTIONMODULE_API module_mediator::return_value on_container_creation(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value on_thread_creation(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value register_deferred_callback(module_mediator::arguments_string_type bundle);

EXECUTIONMODULE_API module_mediator::return_value self_duplicate(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value self_priority(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value get_thread_saved_variable(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle);

EXECUTIONMODULE_API module_mediator::return_value get_current_thread_id(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value get_current_thread_group_id(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value make_runnable(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value start(module_mediator::arguments_string_type bundle);
EXECUTIONMODULE_API module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle);

EXECUTIONMODULE_API void initialize_m(module_mediator::module_part*);
EXECUTIONMODULE_API void free_m();

#endif // !EXECUTION_MODULE_H
