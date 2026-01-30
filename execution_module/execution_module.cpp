#include "pch.h"
#include "module_interoperation.h"
#include "execution_module.h"
#include "program_state_manager.h"
#include "execution_backend_functions.h"
#include "thread_local_structure.h"
#include "thread_manager.h"

#include "../module_mediator/fsi_types.h"
#include "../module_mediator/module_part.h"
#include "../program_loader/program_functions.h"
#include "../logger_module/logging.h"

module_mediator::return_value on_thread_creation(module_mediator::arguments_string_type bundle) {
    auto [container_id, thread_id, preferred_stack_size] =
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, module_mediator::return_value, std::uint64_t>(bundle);

    thread_local_structure* thread_structure = backend::get_thread_local_structure();

    char* thread_state_memory = backend::allocate_thread_memory(thread_id, program_state_manager::thread_state_size);
    char* thread_stack_memory = backend::allocate_thread_memory(thread_id, preferred_stack_size);
    
    // One byte + eight bytes are reserved to be used with "save" and "load" instructions.
    char* thread_stack_end = thread_stack_memory + preferred_stack_size - (sizeof(module_mediator::one_byte) + sizeof(std::uint64_t));

    void* program_jump_table = std::bit_cast<void*>(
        module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_get_jump_table(),
            container_id
        )
    );

    // Fill in state_buffer - start.
    // Fill in program main function address.
    backend::fill_in_register_array_entry( 
        1, 
        thread_state_memory, 
        reinterpret_cast<std::uintptr_t>(thread_structure->program_function_address)
    );

    // Fill in jump table address.
    backend::fill_in_register_array_entry(
        2,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(program_jump_table)
    );

    // Fill in thread state address. I don't quite remember what purpose this serves.
    backend::fill_in_register_array_entry(
        3,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(thread_state_memory)
    );

    std::uintptr_t result = backend::initialize_thread_stack(
        thread_structure->program_function_address, 
        thread_stack_memory, 
        thread_stack_end, 
        thread_id
    );

    if (result == reinterpret_cast<std::uintptr_t>(nullptr)) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Thread stack initialization has failed.");

        module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_thread_memory(),
            thread_id,
            thread_state_memory
        );
        
        module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_thread_memory(),
            thread_id,
            thread_stack_memory
        );

        module_mediator::return_value result_container_id = backend::deallocate_thread(thread_id);
        if (backend::get_container_running_threads_count(result_container_id) == 0) {
            backend::get_thread_manager().forget_thread_group(result_container_id);
            backend::deallocate_program_container(result_container_id);
        }

        return module_mediator::module_failure;
    }

    // Fill in current stack position with value obtained from stack initializer.
    backend::fill_in_register_array_entry(
        4,
        thread_state_memory,
        result
    );

    // Fill in stack end address. Notice that it had to account for the space that 
    // will be used to save the state of one variable between function calls.
    backend::fill_in_register_array_entry(
        5,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(thread_stack_end)
    );

    // Fill in runtime trap table address. This is mostly a limitation of the current design,
    // as "program_loader" is a separate module, and it can't directly access "execution_module" data.
    backend::fill_in_register_array_entry(
        6,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(backend::get_runtime_trap_table())
    );
    // Fill in thread state - end.

    backend::get_thread_manager().add_thread(
        container_id,
        thread_id,
        thread_structure->priority,
        thread_state_memory,
        program_jump_table
    );

    LOG_PROGRAM_INFO(interoperation::get_module_part(), "New thread has been successfully created.");
    return module_mediator::module_success;
}

module_mediator::return_value on_container_creation(module_mediator::arguments_string_type bundle) {
    auto [container_id, program_main, preferred_stack_size] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, std::uint64_t>(bundle);

    backend::get_thread_manager().add_thread_group(container_id, preferred_stack_size);
    LOG_PROGRAM_INFO(interoperation::get_module_part(), "New thread group has been successfully created.");

    return backend::create_thread(container_id, 0, program_main);
}

module_mediator::return_value register_deferred_callback(module_mediator::arguments_string_type bundle) {
    auto [callback_info] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::memory>(bundle);

    module_mediator::callback_bundle* callback = static_cast<module_mediator::callback_bundle*>(callback_info);
    backend::get_thread_local_structure()->deferred_callbacks.push_back(callback);

    return module_mediator::module_success;
}

module_mediator::return_value self_duplicate(module_mediator::arguments_string_type bundle) {
    auto [main_function_address, main_function_parameters, parameters_size] =
        module_mediator::arguments_string_builder::unpack<void*, void*, std::uint64_t>(bundle);

    module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[parameters_size]{};
    std::memcpy(copy, main_function_parameters, parameters_size);

    return backend::self_duplicate(main_function_address, copy);
}

module_mediator::return_value self_priority(module_mediator::arguments_string_type) {
    return backend::get_thread_local_structure()->currently_running_thread_information.priority;
}

module_mediator::return_value get_thread_saved_variable(module_mediator::arguments_string_type) {
    char* thread_state = static_cast<char*>(backend::get_thread_local_structure()->currently_running_thread_information.thread_state) + 40;
    module_mediator::memory thread_stack_end{};

    std::memcpy(&thread_stack_end, thread_state, sizeof(module_mediator::memory));
    return reinterpret_cast<std::uintptr_t>(thread_stack_end);
}

module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle) {
    auto [function_address, function_arguments_data] =
        module_mediator::arguments_string_builder::unpack<void*, void*>(bundle);

    module_mediator::arguments_string_type function_arguments_string =
        static_cast<module_mediator::arguments_string_type>(function_arguments_data);

    if (backend::check_function_signature(function_address, function_arguments_string) != module_mediator::module_success) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(), "A function signature for function '" + backend::get_exposed_function_name(function_address) +
            "' does not match passed arguments. (dynamic call)"
        );

        return module_mediator::module_failure;
    }

    program_state_manager state_manager{ 
        static_cast<char*>(backend::get_thread_local_structure()->currently_running_thread_information.thread_state) 
    };

    std::uintptr_t program_return_address = state_manager.get_return_address();
    module_mediator::arguments_array_type function_arguments = module_mediator::arguments_string_builder::convert_to_arguments_array(
        function_arguments_string
    );

    function_arguments.insert(
        function_arguments.cbegin(),
        {
            static_cast<module_mediator::arguments_string_element>(
                module_mediator::arguments_string_builder::get_type_index<std::uintptr_t>
            ),
            &program_return_address
        }
    );

    std::uintptr_t new_current_stack_position = backend::apply_initializer_on_thread_stack(
        std::bit_cast<char*>(state_manager.get_current_stack_position()),
        std::bit_cast<char*>(state_manager.get_stack_end()),
        function_arguments,
        backend::get_thread_local_structure()->currently_running_thread_information.thread_id
    );

    if (new_current_stack_position == reinterpret_cast<std::uintptr_t>(nullptr)) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "The thread stack initialization has failed. (dynamic call)");
        return module_mediator::module_failure;
    }

    state_manager.set_current_stack_position(
        new_current_stack_position - module_mediator::arguments_string_builder::get_type_size_by_index(
            module_mediator::arguments_string_builder::get_type_index<std::uintptr_t>
        )
    );
    
    state_manager.set_return_address(
        reinterpret_cast<std::uintptr_t>(static_cast<char*>(function_address) + function_save_return_address_size)
    );

    return module_mediator::module_success;
}

module_mediator::return_value get_current_thread_id(module_mediator::arguments_string_type) {
    return backend::get_thread_local_structure()->currently_running_thread_information.thread_id;
}

module_mediator::return_value get_current_thread_group_id(module_mediator::arguments_string_type) {
    return backend::get_thread_local_structure()->currently_running_thread_information.thread_group_id;
}

module_mediator::return_value make_runnable(module_mediator::arguments_string_type bundle) {
    auto [thread_id] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value>(bundle);

    if (backend::get_thread_manager().make_runnable(thread_id)) {
        LOG_PROGRAM_INFO(interoperation::get_module_part(), "Made a thread with id '" + std::to_string(thread_id) + "' runnable again.");
    }
    else {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Was unable to make a thread with {} runnable again. This most likely indicates a data race.",
                thread_id
            )
        );

        return module_mediator::module_failure;
    }

    return module_mediator::module_success;
}

module_mediator::return_value start(module_mediator::arguments_string_type bundle) {
    auto [thread_count] =
        module_mediator::arguments_string_builder::unpack<std::uint16_t>(bundle);

    LOG_INFO(
        interoperation::get_module_part(), 
        "Creating execution daemons. Count: " + std::to_string(thread_count)
    );

    backend::get_thread_manager().startup(thread_count);
    return module_mediator::module_success;
}

module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle) {
    auto [priority, main_function_address, main_function_parameters, parameters_size] =
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, void*, std::uint64_t>(bundle);

    module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[parameters_size] {};
    std::memcpy(copy, main_function_parameters, parameters_size);

    return backend::create_thread_initializer(priority, main_function_address, copy);
}
