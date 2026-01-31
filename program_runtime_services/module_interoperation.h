#ifndef PROGRAM_RUNTIME_SERVICES_MODULE_INTEROPERATION_H
#define PROGRAM_RUNTIME_SERVICES_MODULE_INTEROPERATION_H

#include "../module_mediator/module_part.h"
#include "../module_mediator/fsi_types.h"

#ifdef PROGRAMRUNTIMESERVICES_EXPORTS
#define PROGRAMRUNTIMESERVICES_API extern "C" __declspec(dllexport)
#else
#define PROGRAMRUNTIMESERVICES_API extern "C" __declspec(dllimport) 
#endif

PROGRAMRUNTIMESERVICES_API void initialize_m(module_mediator::module_part* part);
PROGRAMRUNTIMESERVICES_API void free_m();

namespace interoperation {
    module_mediator::module_part* get_module_part();
    module_mediator::return_value verify_thread_memory(
        module_mediator::return_value thread_id,
        module_mediator::memory pointer
    );

    module_mediator::return_value get_current_thread_id();
    module_mediator::return_value get_current_thread_group_id();

    module_mediator::return_value thread_allocate(
        module_mediator::return_value thread_id, 
        module_mediator::eight_bytes size
    );

    void thread_deallocate(
        module_mediator::return_value thread_id, 
        module_mediator::memory pointer
    );

    module_mediator::return_value thread_group_allocate(
        module_mediator::return_value thread_group_id, 
        module_mediator::eight_bytes size
    );

    void thread_group_deallocate(
        module_mediator::return_value thread_group_id, 
        module_mediator::memory pointer
    );

    class index_getter {
    public:
        static std::size_t program_loader() {
            static std::size_t index = get_module_part()->find_module_index("progload");
            return index;
        }

        static std::size_t program_loader_check_function_arguments() {
            static std::size_t index = get_module_part()->find_function_index(program_loader(), "check_function_arguments");
            return index;
        }

        static std::size_t execution_module() {
            static std::size_t index = get_module_part()->find_module_index("excm");
            return index;
        }

        static std::size_t execution_module_get_current_thread_group_id() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "get_current_thread_group_id");
            return index;
        }

        static std::size_t execution_module_get_current_thread_id() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "get_current_thread_id");
            return index;
        }

        static std::size_t execution_module_create_thread() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "create_thread");
            return index;
        }

        static std::size_t execution_module_get_thread_saved_variable() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "get_thread_saved_variable");
            return index;
        }

        static std::size_t execution_module_self_duplicate() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "self_duplicate");
            return index;
        }

        static std::size_t execution_module_self_priority() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "self_priority");
            return index;
        }

        static std::size_t execution_module_dynamic_call() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "dynamic_call");
            return index;
        }

        static std::size_t execution_module_make_runnable() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "make_runnable");
            return index;
        }

        static std::size_t execution_module_register_deferred_callback() {
            static std::size_t index = get_module_part()->find_function_index(execution_module(), "register_deferred_callback");
            return index;
        }

        static std::size_t resource_module() {
            static std::size_t index = get_module_part()->find_module_index("resm");
            return index;
        }

        static std::size_t resource_module_allocate_thread_memory() {
            static std::size_t index = get_module_part()->find_function_index(resource_module(), "allocate_thread_memory");
            return index;
        }

        static std::size_t resource_module_deallocate_thread_memory() {
            static std::size_t index = get_module_part()->find_function_index(resource_module(), "deallocate_thread_memory");
            return index;
        }

        static std::size_t resource_module_verify_thread_memory() {
            static std::size_t index = get_module_part()->find_function_index(resource_module(), "verify_thread_memory");
            return index;
        }

        static std::size_t resource_module_allocate_program_memory() {
            static std::size_t index = get_module_part()->find_function_index(resource_module(), "allocate_program_memory");
            return index;
        }

        static std::size_t resource_module_deallocate_program_memory() {
            static std::size_t index = get_module_part()->find_function_index(resource_module(), "deallocate_program_memory");
            return index;
        }

        static std::size_t resource_module_get_jump_table() {
            static std::size_t index = get_module_part()->find_function_index(resource_module(), "get_jump_table");
            return index;
        }

        static std::size_t resource_module_get_jump_table_size() {
            static std::size_t index = get_module_part()->find_function_index(resource_module(), "get_jump_table_size");
            return index;
        }

        static std::size_t logger() {
            static std::size_t index = get_module_part()->find_module_index("logger");
            return index;
        }

        static std::size_t logger_program_info() {
            static std::size_t index = get_module_part()->find_function_index(logger(), "program_info");
            return index;
        }

        static std::size_t logger_program_warning() {
            static std::size_t index = get_module_part()->find_function_index(logger(), "program_warning");
            return index;
        }

        static std::size_t logger_program_error() {
            static std::size_t index = get_module_part()->find_function_index(logger(), "program_error");
            return index;
        }
    };
}

#endif
