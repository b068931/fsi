#ifndef RUNTIME_TRAPS_H
#define RUNTIME_TRAPS_H

#include "../module_mediator/module_part.h"

namespace runtime_traps {
    [[noreturn]] void program_termination_request(std::uint64_t error_code);
    [[noreturn]] void runtime_module_call(
        std::uint64_t module_id, 
        std::uint64_t function_id, 
        module_mediator::arguments_string_type args_string
    );
}

#endif 
