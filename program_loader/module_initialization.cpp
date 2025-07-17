#include "pch.h"
#include "module_interoperation.h"
#include "program_functions.h"
#include "program_loader.h"

module_mediator::module_part* part = nullptr;

//technically, this function is used during program's lifetime, so it is not necessary to deallocate its memory VirtualFree(::default_function_address, 0, MEM_RELEASE);
void* default_function_address = nullptr; //default address for functions that were declared but weren't initialized (terminates the program)

module_mediator::module_part* get_module_part() {
	return ::part;
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

	::part = module_part;
	::default_function_address = create_executable_function(default_function_symbols);
}

void free_m() {
	VirtualFree((LPVOID)default_function_address, 0, MEM_RELEASE);
}