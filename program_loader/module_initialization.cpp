#include "pch.h"
#include "program_loader.h"
#include "program_functions.h"
#include "module_interoperation.h"
#include "exposed_functions_management.h"
#include "module_function_call_builder.h"

// Must be initialized inside program heap so that it can live longer on program close.
extern std::unordered_map<std::uintptr_t, exposed_function_data>* exposed_functions;
std::unordered_map<std::uintptr_t, exposed_function_data>* exposed_functions = nullptr;

namespace {
    module_mediator::module_part* part = nullptr;
}

namespace interoperation {
    module_mediator::module_part* get_module_part() {
        return part;
    }
}

void initialize_m(module_mediator::module_part* module_part) {
    // add rsp, 8 - remove return address
    std::vector default_function_symbols{ '\x48', '\x83', '\xc4', '\x08' };
    generate_program_termination_code(
        default_function_symbols, 
        program_loader::termination_codes::undefined_function_call
    );

    part = module_part;
    exposed_functions = new std::unordered_map<std::uintptr_t, exposed_function_data>{};

    logger_module::global_logging_instance::set_logging_enabled(true);
}

void free_m() {
    logger_module::global_logging_instance::set_logging_enabled(false);
    if (exposed_functions != nullptr && !exposed_functions->empty()) {
        std::cerr << "*** WARNING: Not all exposed functions were removed before "
                     "program loader module unload. Possible memory leak.\n";
    }

    delete exposed_functions;
}
