#include "pch.h"
#include "thread_local_structure.h"
#include "module_interoperation.h"
#include "thread_manager.h"
#include "assembly_functions.h"
#include "program_state_manager.h"
#include "executions_backend_functions.h"

#include "../logger_module/logging.h"
#include "../module_mediator/module_part.h"
#include "../module_mediator/fsi_types.h"

namespace backend {
    module_mediator::return_value deallocate_thread(module_mediator::return_value thread_id) {
        return module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_thread(),
            thread_id
        );
    }

    void deallocate_program_container(module_mediator::return_value container_id) {
        module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_program_container(),
            container_id
        );
    }

    void delete_running_thread() {
        thread_local_structure* thread_structure = get_thread_local_structure();

        module_mediator::return_value thread_id = thread_structure->currently_running_thread_information.thread_id;
        module_mediator::return_value thread_group_id = thread_structure->currently_running_thread_information.thread_group_id;
        void* thread_state = thread_structure->currently_running_thread_information.thread_state;
        std::uint64_t preferred_stack_size = thread_structure->currently_running_thread_information.preferred_stack_size;

        program_state_manager program_state_manager{
            static_cast<char*>(thread_state)
        };

        module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_thread_memory(),
            thread_id,
            reinterpret_cast<void*>(program_state_manager.get_stack_start(preferred_stack_size))
        );

        module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_thread_memory(),
            thread_id,
            thread_state
        );

        bool is_delete_container = get_thread_manager().delete_thread(thread_group_id, thread_id);

        LOG_INFO(interoperation::get_module_part(), "Deallocating a thread. ID: " + std::to_string(thread_id));
        deallocate_thread(thread_id);
        if (is_delete_container) {
            LOG_INFO(interoperation::get_module_part(), "Deallocating a thread group. ID: " + std::to_string(thread_group_id));
            deallocate_program_container(thread_group_id);
        }
    }

    void program_resume() {
        get_thread_local_structure()->currently_running_thread_information.state = scheduler::thread_states::running;
    }

    void thread_terminate() {
        LOG_PROGRAM_INFO(interoperation::get_module_part(), "Thread was terminated.");

        get_thread_local_structure()->currently_running_thread_information.put_back_structure = nullptr;
        delete_running_thread();
    }

    [[noreturn]] void call_module_error(module_mediator::module_part::call_error error) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-default"

        switch (error) {
        case module_mediator::module_part::call_error::function_is_not_visible:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Called module function is not visible. Thread terminated.");
            break;

        case module_mediator::module_part::call_error::invalid_arguments_string:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect arguments were used for the module function call. Thread terminated.");
            break;

        case module_mediator::module_part::call_error::unknown_index:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Module function does not exist. Thread terminated.");
            break;

        case module_mediator::module_part::call_error::no_error: break;
        }

#pragma clang diagnostic pop

        thread_terminate();
        load_execution_thread(get_thread_local_structure()->execution_thread_state);
    }

    [[noreturn]] void call_module(std::uint64_t module_id, std::uint64_t function_id, module_mediator::arguments_string_type args_string) {
        module_mediator::return_value action_code = interoperation::get_module_part()->call_module_visible_only(
            module_id,
            function_id,
            args_string, &call_module_error
        );

        //all these functions are declared as [[noreturn]]
        switch (action_code)
        {
        case module_mediator::execution_result_continue:
            program_resume();
            resume_program_execution(get_thread_local_structure()->currently_running_thread_information.thread_state);

        case module_mediator::execution_result_switch:
            load_execution_thread(get_thread_local_structure()->execution_thread_state);

        case module_mediator::execution_result_terminate:
            LOG_PROGRAM_INFO(interoperation::get_module_part(), "Requested thread termination.");

            thread_terminate();
            load_execution_thread(get_thread_local_structure()->execution_thread_state);

        case module_mediator::execution_result_block:
            LOG_PROGRAM_INFO(interoperation::get_module_part(), "Was blocked.");

            get_thread_manager().block(get_thread_local_structure()->currently_running_thread_information.thread_id);
            load_execution_thread(get_thread_local_structure()->execution_thread_state);

        default:
            LOG_PROGRAM_FATAL(interoperation::get_module_part(), "Incorrect return code. Process will be aborted.");
            std::terminate();
        }
    }

    module_mediator::return_value self_duplicate(void* main_function, module_mediator::arguments_string_type initializer) {
        assert(get_thread_local_structure()->initializer == nullptr && "possible memory leak");
        get_thread_local_structure()->initializer = initializer;
        if (initializer != nullptr) { //you can not initialize a thread_group with memory because this can lead to data race
            constexpr auto pointer_type_index = 
                module_mediator::arguments_string_builder::get_type_index<module_mediator::memory>;

            module_mediator::arguments_string_element arguments_count = initializer[0];
            module_mediator::arguments_string_type arguments_types = initializer + 1;
            for (module_mediator::arguments_string_element index = 0; index < arguments_count; ++index) {
                if (arguments_types[index] == static_cast<module_mediator::arguments_string_element>(pointer_type_index)) {
                    delete[] initializer;
                    get_thread_local_structure()->initializer = nullptr;

                    LOG_PROGRAM_ERROR(
                        interoperation::get_module_part(),
                        "Shared memory between thread groups is not allowed. Thread group cannot depend on another thread group."
                    );

                    return module_mediator::module_failure;
                }
            }
        }

        return module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_duplicate_container(),
            get_thread_local_structure()->currently_running_thread_information.thread_group_id,
            main_function
        );
    }

    module_mediator::return_value get_container_running_threads_count(module_mediator::return_value container_id) {
        return module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_get_running_threads_count(),
            container_id
        );
    }

    void fill_in_register_array_entry(std::uint64_t entry_index, char* memory, std::uint64_t value) {
        std::memcpy(memory + sizeof(std::uint64_t) * entry_index, &value, sizeof(std::uint64_t));
    }

    module_mediator::return_value create_thread(
        module_mediator::return_value thread_group_id, 
        module_mediator::return_value priority, 
        void* function_address
    ) {
        thread_local_structure* thread_structure = get_thread_local_structure();
        LOG_PROGRAM_INFO(interoperation::get_module_part(), "Creating a new thread.");

        thread_structure->program_function_address = function_address;
        thread_structure->priority = priority;

        return module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_create_new_thread(),
            thread_group_id
        );
    }

    module_mediator::return_value create_thread_initializer(
        module_mediator::return_value priority, 
        void* function_address, 
        module_mediator::arguments_string_type initializer
    ) {
        assert(get_thread_local_structure()->initializer == nullptr && "possible memory leak");
        get_thread_local_structure()->initializer = initializer;
        return create_thread(
            get_thread_local_structure()->currently_running_thread_information.thread_group_id,
            priority,
            function_address
        );
    }

    char* allocate_thread_memory(module_mediator::return_value thread_id, std::uint64_t size) {
        return reinterpret_cast<char*>(
            module_mediator::fast_call<module_mediator::return_value, module_mediator::eight_bytes>(
                interoperation::get_module_part(),
                interoperation::index_getter::resource_module(),
                interoperation::index_getter::resource_module_allocate_thread_memory(),
                thread_id,
                size
            ));
    }

    module_mediator::return_value check_function_signature(void* function_address, module_mediator::arguments_string_type initializer) {
        std::unique_ptr<module_mediator::arguments_string_element[]> default_signature{ module_mediator::arguments_string_builder::get_types_string() };
        module_mediator::arguments_string_type alleged_signature = default_signature.get();
        if (initializer != nullptr) {
            alleged_signature = initializer;
        }

        return module_mediator::fast_call<module_mediator::memory, std::uintptr_t>(
            interoperation::get_module_part(),
            interoperation::index_getter::program_loader(),
            interoperation::index_getter::program_loader_check_function_arguments(),
            alleged_signature,
            reinterpret_cast<std::uintptr_t>(function_address)
        );
    }

    // Used for debugging purposes. If you disable logging, this function will not be called.
    [[maybe_unused]] std::string get_exposed_function_name(void* function_address) {
        module_mediator::return_value functions_symbols_address = module_mediator::fast_call(
            interoperation::get_module_part(),
            interoperation::index_getter::program_loader(),
            interoperation::index_getter::program_loader_get_function_name(),
            reinterpret_cast<std::uintptr_t>(function_address)
        );

        std::string function_name{ "[UNKNOWN_FUNCTION_NAME]" };
        char* function_symbols = reinterpret_cast<char*>(functions_symbols_address);
        if (function_symbols != nullptr) {
            function_name = function_symbols;
        }

        return function_name;
    }

    std::uintptr_t apply_initializer_on_thread_stack(
        char* thread_stack_memory, 
        const char* thread_stack_end, 
        const module_mediator::arguments_array_type& arguments,
        module_mediator::return_value thread_id
    ) {
        for (auto [argument_type, argument_address] : std::ranges::reverse_view(arguments)) {
            std::size_t type_size = module_mediator::arguments_string_builder::get_type_size_by_index(argument_type);
            if (thread_stack_memory + type_size >= thread_stack_end) {
                LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Not enough stack space to initialize thread stack.");
                return reinterpret_cast<std::uintptr_t>(nullptr);
            }

            if (
                // Create new memory descriptor only if we are creating a new thread.
                get_thread_local_structure()->currently_running_thread_information.thread_id != thread_id &&
                argument_type == module_mediator::arguments_string_builder::get_type_index<module_mediator::memory>
            ) {
                // Memory size, base address, and cross-thread sharing pointer
                constexpr std::size_t memory_descriptor_size = sizeof(std::uint64_t) * 3;

                module_mediator::memory old_descriptor_address{};
                std::memcpy(
                    &old_descriptor_address, 
                    argument_address, 
                    sizeof(module_mediator::memory)
                );

                module_mediator::return_value verification_result = module_mediator::fast_call<
                    module_mediator::return_value, 
                    module_mediator::memory
                >(
                    interoperation::get_module_part(),
                    interoperation::index_getter::resource_module(),
                    interoperation::index_getter::resource_module_verify_thread_memory(),
                    get_thread_local_structure()->currently_running_thread_information.thread_id,
                    old_descriptor_address
                );

                if (verification_result == module_mediator::module_failure) {
                    LOG_PROGRAM_ERROR(
                        interoperation::get_module_part(), 
                        std::format(
                            "Thread memory verification failed for address {}.",
                            reinterpret_cast<std::uintptr_t>(old_descriptor_address)
                        )
                    );

                    return reinterpret_cast<std::uintptr_t>(nullptr);
                }

                char* new_memory_descriptor = allocate_thread_memory(thread_id, memory_descriptor_size);
                if (new_memory_descriptor == nullptr) {
                   LOG_PROGRAM_ERROR(
                       interoperation::get_module_part(), 
                       "Failed to allocate memory for memory descriptor during thread stack initialization."
                   );

                   return reinterpret_cast<std::uintptr_t>(nullptr);
                }

                std::memcpy(
                    new_memory_descriptor, 
                    old_descriptor_address, 
                    memory_descriptor_size
                );

                module_mediator::memory cross_thread_sharing{};
                std::memcpy(
                    &cross_thread_sharing,
                    new_memory_descriptor + sizeof(std::uint64_t) * 2,
                    sizeof(module_mediator::memory)
                );

                assert(cross_thread_sharing != nullptr && "cross-thread sharing pointer must not be null");
                std::atomic_ref cross_thread_sharing_synchronous = std::atomic_ref(
                    *static_cast<std::uint64_t*>(cross_thread_sharing)
                );

                [[maybe_unused]] std::size_t previous = 
                    cross_thread_sharing_synchronous.fetch_add(1, std::memory_order_relaxed);

                assert(previous != 0 && "cross-thread sharing counter must not be zero");
                assert(sizeof(&new_memory_descriptor) == type_size && "unexpected type size");

                std::memcpy(
                    thread_stack_memory,
                    &new_memory_descriptor,
                    type_size
                );
            }
            else {
                std::memcpy(
                    thread_stack_memory,
                    argument_address,
                    type_size
                );
            }

            thread_stack_memory += type_size;
        }

        return reinterpret_cast<std::uintptr_t>(thread_stack_memory);
    }

    std::uintptr_t initialize_thread_stack(
        void* function_address, 
        char* thread_stack_memory, 
        const char* thread_stack_end, 
        module_mediator::return_value thread_id
    ) {
        if (check_function_signature(function_address, get_thread_local_structure()->initializer) != module_mediator::module_success) {
            delete[] get_thread_local_structure()->initializer;
            get_thread_local_structure()->initializer = nullptr;

            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                "Function signature for function '" + get_exposed_function_name(function_address) +
                "' does not match."
            );

            return reinterpret_cast<std::uintptr_t>(nullptr);
        }

        module_mediator::arguments_string_type initializer = get_thread_local_structure()->initializer;
        if (initializer != nullptr) {
            module_mediator::arguments_array_type arguments =
                module_mediator::arguments_string_builder::convert_to_arguments_array(get_thread_local_structure()->initializer);

            std::uintptr_t result = apply_initializer_on_thread_stack(
                thread_stack_memory, 
                thread_stack_end, 
                arguments, 
                thread_id
            );

            delete[] get_thread_local_structure()->initializer;
            get_thread_local_structure()->initializer = nullptr;

            return result;
        }

        return reinterpret_cast<std::uintptr_t>(thread_stack_memory);
    }
}

