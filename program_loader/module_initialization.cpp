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
    // Technically, this function is used during program's lifetime, so it is not necessary to deallocate its memory VirtualFree(::default_function_address, 0, MEM_RELEASE);
    // Default address for functions that were declared but weren't initialized (terminates the program)
    void* default_function_address = nullptr; 
    module_mediator::module_part* part = nullptr;
}

namespace interoperation {
    module_mediator::module_part* get_module_part() {
        return part;
    }
}

void* get_default_function_address() {
    return default_function_address;
}

void initialize_m(module_mediator::module_part* module_part) {
    // add rsp, 8 - remove return address
    std::vector default_function_symbols{ '\x48', '\x83', '\xc4', '\x08' };
    generate_program_termination_code(
        default_function_symbols, 
        program_loader::termination_codes::undefined_function_call
    );

    part = module_part;
    default_function_address = create_executable_function(default_function_symbols);
    exposed_functions = new std::unordered_map<std::uintptr_t, exposed_function_data>{};

    logger_module::global_logging_instance::set_logging_enabled(true);
}

void free_m() {
    logger_module::global_logging_instance::set_logging_enabled(false);

    VirtualFree(default_function_address, 0, MEM_RELEASE);
    if (exposed_functions != nullptr && !exposed_functions->empty()) {
        std::cerr << "*** WARNING: Not all exposed functions were removed before "
                     "program loader module unload. Possible memory leak.\n";
    }

    delete exposed_functions;
}
