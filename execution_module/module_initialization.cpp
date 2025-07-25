#include "pch.h"
#include "module_interoperation.h"
#include "execution_module.h"
#include "thread_manager.h"
#include "assembly_functions.h"
#include "executions_backend_functions.h"

#include "../logger_module/logging.h"
#include "../program_loader/program_functions.h"

namespace {
    module_mediator::module_part* part = nullptr;
    char* program_control_functions_addresses = nullptr;
    thread_manager manager{};

    void show_error(std::uint64_t error_code) {
        switch (static_cast<termination_codes>(error_code)) {
        case termination_codes::stack_overflow:
            LOG_PROGRAM_ERROR(::part, "Stack overflow. This can happen during function call, module function call, function prologue.");
            break;

        case termination_codes::nullptr_dereference:
            LOG_PROGRAM_ERROR(::part, "Uninitialized pointer dereference.");
            break;

        case termination_codes::pointer_out_of_bounds:
            LOG_PROGRAM_ERROR(::part, "Pointer index is out of bounds.");
            break;

        case termination_codes::undefined_function_call:
            LOG_PROGRAM_ERROR(::part, "Undefined function call. Called function has its own signature but it does not have a body.");
            break;

        case termination_codes::incorrect_saved_variable_type:
            LOG_PROGRAM_ERROR(::part, "Saved variable has different type.");
            break;

        case termination_codes::division_by_zero:
            LOG_PROGRAM_ERROR(::part, "Division by zero.");
            break;

        default:  // NOLINT(clang-diagnostic-covered-switch-default)
            LOG_PROGRAM_FATAL(::part, "Unknown termination code. The process will be terminated with 'abort'.");
            std::abort();
        }

        LOG_PROGRAM_ERROR(::part, "Program execution error. Thread terminated.");
    }
    [[noreturn]] void inner_terminate(std::uint64_t error_code) {
        show_error(error_code);

        thread_terminate();
        load_execution_thread(get_thread_local_structure()->execution_thread_state);
    }
    [[noreturn]] void end_program() {
        thread_terminate();
        load_execution_thread(get_thread_local_structure()->execution_thread_state);
    }
}

module_mediator::module_part* get_module_part() {
    return ::part;
}
char* get_program_control_functions_addresses() {
    return ::program_control_functions_addresses;
}
thread_manager& get_thread_manager() {
    return ::manager;
}

void initialize_m(module_mediator::module_part* module_part) {
    ::part = module_part;
    ::program_control_functions_addresses = new char[4 * sizeof(std::uint64_t)] {};

    inner_fill_in_reg_array_entry(
        0,
        ::program_control_functions_addresses,
        reinterpret_cast<std::uintptr_t>(&special_call_module)
    );

    inner_fill_in_reg_array_entry(
        1,
        ::program_control_functions_addresses,
        reinterpret_cast<std::uintptr_t>(&inner_call_module)
    );

    inner_fill_in_reg_array_entry(
        2,
        ::program_control_functions_addresses,
        reinterpret_cast<std::uintptr_t>(&inner_terminate)
    );

    inner_fill_in_reg_array_entry(
        3,
        ::program_control_functions_addresses,
        reinterpret_cast<std::uintptr_t>(&end_program)
    );
}

void free_m() {
    delete[] ::program_control_functions_addresses;
}