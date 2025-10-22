#include "pch.h"
#include "program_loader.h"
#include "program_functions.h"
#include "module_interoperation.h"

// Must be initialized inside program heap so that it can live longer on program close.
extern std::unordered_map<
    std::uintptr_t,
    std::pair<
        std::unique_ptr<module_mediator::arguments_string_element[]>,
        std::string>
    >* exposed_functions;

namespace {
	//technically, this function is used during program's lifetime, so it is not necessary to deallocate its memory VirtualFree(::default_function_address, 0, MEM_RELEASE);
	void* default_function_address = nullptr; //default address for functions that were declared but weren't initialized (terminates the program)
	module_mediator::module_part* part = nullptr;
}

namespace interoperation {
	module_mediator::module_part* get_module_part() {
		return ::part;
	}
}

void* get_default_function_address() {
	return ::default_function_address;
}

void initialize_m(module_mediator::module_part* module_part) {
	std::vector<char> default_function_symbols{};
	default_function_symbols.push_back('\x48'); //add rsp, 8 - remove return address
	default_function_symbols.push_back('\x83');
	default_function_symbols.push_back('\xc4');
	default_function_symbols.push_back('\x08');

	generate_program_termination_code(default_function_symbols, termination_codes::undefined_function_call);
	exposed_functions = new std::unordered_map<
		std::uintptr_t,
		std::pair<
			std::unique_ptr<module_mediator::arguments_string_element[]>,
			std::string>
        >{};

	::part = module_part;
	::default_function_address = create_executable_function(default_function_symbols);
}

void free_m() {
	VirtualFree((LPVOID)default_function_address, 0, MEM_RELEASE);
    delete exposed_functions;
}