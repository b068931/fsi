#include "pch.h"
#include "logging.h"
#include "backend_functions.h"

#include "../logger_module/logging.h"

namespace {
    enum class log_type : std::uint8_t {
        info,
        warning,
        error
    };

    void cleanse_string(module_mediator::memory string, module_mediator::eight_bytes size) {
        assert(string != nullptr && size != 0);
        char* string_data = static_cast<char*>(string);
        string_data[size - 1] = '\0';
    }

    module_mediator::return_value generic_log_message(module_mediator::arguments_string_type bundle, log_type log) {
        auto arguments = module_mediator::arguments_string_builder::unpack<
            module_mediator::memory, 
            module_mediator::eight_bytes, 
            module_mediator::memory, 
            module_mediator::memory,
            module_mediator::memory>(bundle);

        module_mediator::eight_bytes line_number = std::get<1>(arguments);
        auto [file_name_pointer, file_name_pointer_size] = backend::decay_pointer(std::get<0>(arguments));
        auto [function_name_pointer, function_name_pointer_size] = backend::decay_pointer(std::get<2>(arguments));
        auto [module_name_pointer, module_name_pointer_size] = backend::decay_pointer(std::get<3>(arguments));
        auto [message_pointer, message_pointer_size] = backend::decay_pointer(std::get<4>(arguments));

        std::size_t log_endpoints[] {
            interoperation::index_getter::logger_program_info(),
            interoperation::index_getter::logger_program_warning(),
            interoperation::index_getter::logger_program_error()
        };

        if (message_pointer == nullptr) {
            LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Null message passed to logging function.");
            return module_mediator::execution_result_terminate;
        }

        cleanse_string(message_pointer, message_pointer_size);
        if (file_name_pointer != nullptr) {
            cleanse_string(file_name_pointer, file_name_pointer_size);
        }

        if (module_name_pointer != nullptr) {
            cleanse_string(module_name_pointer, module_name_pointer_size);
        }

        if (function_name_pointer != nullptr) {
            cleanse_string(function_name_pointer, function_name_pointer_size);
        }

        module_mediator::fast_call<
            module_mediator::memory,
            module_mediator::eight_bytes,
            module_mediator::memory,
            module_mediator::memory,
            module_mediator::memory
        >(
            interoperation::get_module_part(),
            interoperation::index_getter::logger(),
            log_endpoints[static_cast<std::uint8_t>(log)],
            file_name_pointer,
            line_number,
            function_name_pointer,
            module_name_pointer,
            message_pointer
        );

        return module_mediator::execution_result_continue;
    }
}

module_mediator::return_value info(module_mediator::arguments_string_type bundle) {
    return generic_log_message(bundle, log_type::info);
}

module_mediator::return_value warning(module_mediator::arguments_string_type bundle) {
    return generic_log_message(bundle, log_type::warning);
}

module_mediator::return_value error(module_mediator::arguments_string_type bundle) {
    return generic_log_message(bundle, log_type::error);
}
