#ifndef EXECUTION_MODULE_EXECUTIONS_BACKEND_FUNCTIONS_H
#define EXECUTION_MODULE_EXECUTIONS_BACKEND_FUNCTIONS_H

#include "thread_manager.h"
#include "../module_mediator/module_part.h"

thread_manager& get_thread_manager();

namespace backend {
    [[noreturn]] void call_module_error(
        module_mediator::module_part::call_error error
    );

    [[noreturn]] void call_module(
        std::uint64_t module_id, 
        std::uint64_t function_id, 
        module_mediator::arguments_string_type args_string
    );

    [[maybe_unused]] std::string get_exposed_function_name(
        void* function_address
    );

    module_mediator::return_value deallocate_thread(
        module_mediator::return_value thread_id
    );

    void deallocate_program_container(
        module_mediator::return_value container_id
    );

    void delete_running_thread();

    void program_resume();

    void thread_terminate();

    module_mediator::return_value self_duplicate(
        void* main_function, 
        module_mediator::arguments_string_type initializer
    );

    module_mediator::return_value get_container_running_threads_count(
        module_mediator::return_value container_id
    );

    void fill_in_reg_array_entry(
        std::uint64_t entry_index, 
        char* memory, 
        std::uint64_t value
    );

    module_mediator::return_value create_thread(
        module_mediator::return_value thread_group_id, 
        module_mediator::return_value priority, 
        void* function_address
    );

    module_mediator::return_value create_thread_initializer(
        module_mediator::return_value priority, 
        void* function_address, 
        module_mediator::arguments_string_type initializer
    );

    char* allocate_thread_memory(
        module_mediator::return_value thread_id, 
        std::uint64_t size
    );

    module_mediator::return_value check_function_signature(
        void* function_address, 
        module_mediator::arguments_string_type initializer
    );

    std::uintptr_t apply_initializer_on_thread_stack(
        char* thread_stack_memory, 
        const char* thread_stack_end, 
        const module_mediator::arguments_array_type& arguments
    );

    std::uintptr_t initialize_thread_stack(
        void* function_address, 
        char* thread_stack_memory, 
        char* thread_stack_end, 
        module_mediator::return_value thread_id
    );
}

#endif