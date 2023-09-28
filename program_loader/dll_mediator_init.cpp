#include "pch.h"
#include "declarations.h"
#include "functions.h"
#include "program_loader.h"

dll_part* part = nullptr;

//technically, this function is used during program's lifetime, so it is not necessary to deallocate its memory VirtualFree(::default_function_address, 0, MEM_RELEASE);
void* default_function_address = nullptr; //default address for functions that were declared but weren't initialized (terminates the program)

dll_part* get_dll_part() {
	return ::part;
}
void* get_default_function_address() {
	return ::default_function_address;
}

void initialize_m(dll_part* part) {
	::part = part;

	std::vector<char> default_function_symbols{};
	default_function_symbols.push_back('\x48'); //add rsp, 8 - remove return address
	default_function_symbols.push_back('\x83');
	default_function_symbols.push_back('\xc4');
	default_function_symbols.push_back('\x08');

	generate_program_termination_code(default_function_symbols, termination_codes::undefined_function_call);

	::default_function_address = create_executable_function(default_function_symbols);
}
void free_m() {
	VirtualFree((LPVOID)default_function_address, 0, MEM_RELEASE);
}