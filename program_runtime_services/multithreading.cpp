#include "pch.h"
#include "multithreading.h"

#include "../logger_module/logging.h"

namespace {
    module_mediator::return_value get_current_thread_group_jump_table_size() {
        return module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_get_jump_table_size(),
            interoperation::get_current_thread_group_id()
        );
    }

    void* get_current_thread_group_jump_table() {
        return reinterpret_cast<void*>(module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_get_jump_table(),
            interoperation::get_current_thread_group_id()
        ));
    }

    void* get_function_address(std::uint64_t function_displacement) {
        char* jump_table_bytes =
            static_cast<char*>(get_current_thread_group_jump_table());

        void* function_address = nullptr;
        std::memcpy(static_cast<void*>(&function_address), jump_table_bytes + function_displacement, sizeof(void*));

        return function_address;
    }

    bool check_function_displacement(std::uint64_t function_displacement) {
        return function_displacement + sizeof(void*) <= get_current_thread_group_jump_table_size();
    }
}

module_mediator::return_value yield(module_mediator::arguments_string_type) {
    return module_mediator::execution_result_switch;
}
module_mediator::return_value self_terminate(module_mediator::arguments_string_type) {
    return module_mediator::execution_result_terminate;
}
module_mediator::return_value self_priority(module_mediator::arguments_string_type bundle) {
    auto [return_address, type] =
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte>(bundle);

    if (type != module_mediator::eight_bytes_return_value) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(), 
            "Incorrect return type. Eight bytes were expected. (self_priority)"
        );

        return module_mediator::execution_result_terminate;
    }

    module_mediator::return_value priority = module_mediator::fast_call(
        interoperation::get_module_part(),
        interoperation::index_getter::execution_module(),
        interoperation::index_getter::execution_module_self_priority()
    );
    
    std::memcpy(return_address, &priority, sizeof(module_mediator::eight_bytes));
    return module_mediator::execution_result_continue;
}
module_mediator::return_value thread_id(module_mediator::arguments_string_type bundle) {
    auto [return_address, type] =
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte>(bundle);

    if (type != module_mediator::eight_bytes_return_value) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect return type. (thread_id)");
        return module_mediator::execution_result_terminate;
    }

    module_mediator::return_value thread_id = module_mediator::fast_call(
        interoperation::get_module_part(),
        interoperation::index_getter::execution_module(),
        interoperation::index_getter::execution_module_get_current_thread_id()
    );
    
    std::memcpy(return_address, &thread_id, sizeof(module_mediator::eight_bytes));
    return module_mediator::execution_result_continue;
}
module_mediator::return_value thread_group_id(module_mediator::arguments_string_type bundle) {
    auto [return_address, type] =
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte>(bundle);

    if (type != module_mediator::eight_bytes_return_value) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect return type. (thread_group_id)");
        return module_mediator::execution_result_terminate;
    }

    std::uint64_t thread_group_id = interoperation::get_current_thread_group_id();
    std::memcpy(return_address, &thread_group_id, sizeof(module_mediator::eight_bytes));

    return module_mediator::execution_result_continue;
}
module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle) {
    module_mediator::arguments_array_type arguments =
        module_mediator::arguments_string_builder::convert_to_arguments_array(bundle);

    module_mediator::eight_bytes function_displacement{};
    if (!module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&function_displacement, 0, arguments)) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Dynamic call incorrect structure.");
        return module_mediator::execution_result_terminate;
    }

    if (check_function_displacement(function_displacement)) {
        std::pair<module_mediator::arguments_string_type, std::size_t> args_string_size =
            module_mediator::arguments_string_builder::convert_from_arguments_array(arguments.begin() + 1, arguments.end());

        std::unique_ptr<module_mediator::arguments_string_element[]> thread_main_parameters{
            args_string_size.first
        };

        module_mediator::return_value result = module_mediator::fast_call<module_mediator::memory, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::execution_module(),
            interoperation::index_getter::execution_module_dynamic_call(),
            get_function_address(function_displacement),
            thread_main_parameters.get()
        );

        if (result != module_mediator::module_success) {
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Dynamic call failed.");
            return module_mediator::execution_result_terminate;
        }
    }

    return module_mediator::execution_result_continue;
}

module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle) {
    module_mediator::arguments_array_type arguments = 
        module_mediator::arguments_string_builder::convert_to_arguments_array(bundle);

    module_mediator::eight_bytes priority{};
    module_mediator::eight_bytes function_displacement{};
    if (
        !module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&priority, 0, arguments) ||
            !module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&function_displacement, 1, arguments)
    ) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect create_thread call structure.");
        return module_mediator::execution_result_terminate;
    }

    if (check_function_displacement(function_displacement)) {
        std::pair<module_mediator::arguments_string_type, std::size_t> args_string_size = 
            module_mediator::arguments_string_builder::convert_from_arguments_array(arguments.begin() + 2, arguments.end());

        std::unique_ptr<module_mediator::arguments_string_element[]> thread_main_parameters{
            args_string_size.first
        };

        module_mediator::return_value result = module_mediator::fast_call<
            module_mediator::return_value,
            module_mediator::memory,
            module_mediator::memory,
            module_mediator::eight_bytes
        >(
            interoperation::get_module_part(),
            interoperation::index_getter::execution_module(),
            interoperation::index_getter::execution_module_create_thread(),
            priority,
            get_function_address(function_displacement),
            thread_main_parameters.get(),
            args_string_size.second
        );

        if (result != module_mediator::module_success) {
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Can not create a new thread.");
            return module_mediator::execution_result_terminate;
        }
    }

    return module_mediator::execution_result_continue;
}
module_mediator::return_value create_thread_group(module_mediator::arguments_string_type bundle) {
    module_mediator::arguments_array_type arguments =
        module_mediator::arguments_string_builder::convert_to_arguments_array(bundle);

    module_mediator::eight_bytes function_displacement{};
    if (!module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&function_displacement, 0, arguments)) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect create_thread_group call structure.");
        return module_mediator::execution_result_terminate;
    }

    if (check_function_displacement(function_displacement)) {
        std::pair<module_mediator::arguments_string_type, std::size_t> args_string_size =
            module_mediator::arguments_string_builder::convert_from_arguments_array(arguments.begin() + 1, arguments.end());

        std::unique_ptr<module_mediator::arguments_string_element[]> thread_main_parameters{
            args_string_size.first
        };

        module_mediator::return_value result = module_mediator::fast_call<
            module_mediator::memory,
            module_mediator::memory,
            module_mediator::eight_bytes
        >(
            interoperation::get_module_part(),
            interoperation::index_getter::execution_module(),
            interoperation::index_getter::execution_module_self_duplicate(),
            get_function_address(function_displacement),
            thread_main_parameters.get(),
            args_string_size.second
        );

        if (result != module_mediator::module_success) {
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Can not create a new thread group.");
            return module_mediator::execution_result_terminate;
        }
    }

    return module_mediator::execution_result_continue;
}