#include "pch.h"

#include "execution_backend_functions.h"
#include "module_interoperation.h"
#include "control_code_templates.h"
#include "thread_manager.h"
#include "runtime_traps.h"

#include "../logger_module/logging.h"
#include "../program_loader/program_termination_codes.h"

namespace {
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

        backend::thread_terminate();
        CONTROL_CODE_LOAD_EXECUTION_THREAD(&backend::get_thread_local_structure()->execution_thread_state);
    }

    void show_error(std::uint64_t error_code) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcovered-switch-default"

        switch (static_cast<program_loader::termination_codes>(error_code)) {
        case program_loader::termination_codes::stack_overflow:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Stack overflow. This can happen during function call, module function call, function prologue.");
            break;

        case program_loader::termination_codes::nullptr_dereference:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Uninitialized pointer dereference.");
            break;

        case program_loader::termination_codes::pointer_out_of_bounds:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Pointer index is out of bounds.");
            break;

        case program_loader::termination_codes::undefined_function_call:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Undefined function call. Called function has its own signature but it does not have a body.");
            break;

        case program_loader::termination_codes::incorrect_saved_variable_type:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Saved variable has different type.");
            break;

        case program_loader::termination_codes::division_by_zero:
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Division by zero.");
            break;

        default:  // NOLINT(clang-diagnostic-covered-switch-default)
            LOG_PROGRAM_FATAL(interoperation::get_module_part(), "Unknown termination code. The process will be terminated.");
            std::terminate();
        }

#pragma clang diagnostic pop

        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Program execution error. Thread terminated.");
    }
}

namespace runtime_traps {
    [[noreturn]] void runtime_module_call(std::uint64_t module_id, std::uint64_t function_id, module_mediator::arguments_string_type args_string) {
        module_mediator::return_value action_code = interoperation::get_module_part()->call_module_visible_only(
            module_id,
            function_id,
            args_string, &call_module_error
        );

        switch (action_code)
        {
        case module_mediator::execution_result_continue:
            backend::program_resume();
            CONTROL_CODE_RESUME_PROGRAM_EXECUTION(backend::get_thread_local_structure()->currently_running_thread_information.thread_state);

        case module_mediator::execution_result_switch:
            CONTROL_CODE_LOAD_EXECUTION_THREAD(&backend::get_thread_local_structure()->execution_thread_state);

        case module_mediator::execution_result_terminate:
            LOG_PROGRAM_INFO(interoperation::get_module_part(), "Requested thread termination.");

            backend::thread_terminate();
            CONTROL_CODE_LOAD_EXECUTION_THREAD(&backend::get_thread_local_structure()->execution_thread_state);

        case module_mediator::execution_result_block:
            LOG_PROGRAM_INFO(interoperation::get_module_part(), "Was blocked.");

            backend::get_thread_manager().block(backend::get_thread_local_structure()->currently_running_thread_information.thread_id);
            CONTROL_CODE_LOAD_EXECUTION_THREAD(&backend::get_thread_local_structure()->execution_thread_state);

        default:
            LOG_PROGRAM_FATAL(interoperation::get_module_part(), "Incorrect return code. Process will be aborted.");

#ifdef ADDRESS_SANITIZER_ENABLED
            __asan_handle_no_return();
#endif

            std::terminate();
        }
    }

    [[noreturn]] void program_termination_request(std::uint64_t error_code) {
        // Non-zero error code means that an error occurred.
        // Otherwise, it is a normal termination (e.g. main function exit).
        if (error_code != 0) {
            show_error(error_code);
        }

        backend::thread_terminate();
        CONTROL_CODE_LOAD_EXECUTION_THREAD(&backend::get_thread_local_structure()->execution_thread_state);
    }
}
