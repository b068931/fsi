#include "pch.h"
#include "memory.h"
#include "backend_functions.h"

namespace {
    void check_if_pointer_is_saved(module_mediator::memory address) {
        unsigned char* saved_variable =
            reinterpret_cast<unsigned char*>(
                module_mediator::fast_call(
                    interoperation::get_module_part(),
                    interoperation::index_getter::execution_module(),
                    interoperation::index_getter::execution_module_get_thread_saved_variable()
                ));

        if (saved_variable[8] == module_mediator::pointer_return_value) {
            if (std::memcmp(saved_variable, static_cast<void*>(&address), sizeof(module_mediator::memory)) == 0) {
                module_mediator::memory null_pointer{};
                std::memcpy(saved_variable, static_cast<void*>(&null_pointer), sizeof(module_mediator::memory));
            }
        }
    }
}

module_mediator::return_value allocate_memory(module_mediator::arguments_string_type bundle) {
    auto [return_address, return_type, size] =
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte, module_mediator::eight_bytes>(bundle);

    if (return_type != module_mediator::pointer_return_value) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect return type. (allocate_pointer)");
        return module_mediator::execution_result_terminate;
    }

    module_mediator::memory allocated_memory = backend::allocate_program_memory(size);
    if (allocated_memory == nullptr) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Failed memory allocation.");
    }

    std::memcpy(return_address, static_cast<void*>(&allocated_memory), sizeof(module_mediator::memory));
    return module_mediator::execution_result_continue;
}

module_mediator::return_value deallocate_memory(module_mediator::arguments_string_type bundle) {
    auto [return_address, return_type] =
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte>(bundle);

    if (return_type != module_mediator::pointer_return_value) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect return type. (deallocate_pointer)");
        return module_mediator::execution_result_terminate;
    }

    module_mediator::memory address{};
    std::memcpy(static_cast<void*>(&address), return_address, sizeof(module_mediator::memory));
    if (address == nullptr) return module_mediator::execution_result_continue;

    backend::deallocate_program_memory(address);
    check_if_pointer_is_saved(address);

    module_mediator::memory null_pointer{};
    std::memcpy(return_address, static_cast<void*>(&null_pointer), sizeof(module_mediator::memory));

    return module_mediator::execution_result_continue;
}

module_mediator::return_value get_allocated_size(module_mediator::arguments_string_type bundle) {
    auto [return_address, type, address] =
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte, module_mediator::memory>(bundle);
    
    if (type != module_mediator::eight_bytes_return_value) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Incorrect return type. (get_allocated_size)");
        return module_mediator::execution_result_terminate;
    }

    std::uint64_t size{};
    std::memcpy(&size, address, sizeof(std::uint64_t));

    std::memcpy(return_address, &size, sizeof(module_mediator::eight_bytes));
    return module_mediator::execution_result_continue;
}