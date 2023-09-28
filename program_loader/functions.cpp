#include "pch.h"
#include "functions.h"

uint32_t nullify_function_pointer_variables(std::vector<char>& destination, const memory_layouts_builder::memory_addresses& locals) {
	uint32_t instructions_size = 0;
	for (const auto& variable : locals) {
		if (variable.second.second == 4) { //if local is a pointer
			destination.push_back('\x48');
			destination.push_back('\xc7');
			destination.push_back('\x85');
			write_bytes(variable.second.first, destination);
			write_bytes<uint32_t>(0, destination);

			instructions_size += 11;
		}
	}

	return instructions_size;
}

void generate_program_termination_code(std::vector<char>& destination, termination_codes error_code) {
	destination.push_back('\x48'); //mov rcx, error_code
	destination.push_back('\xc7');
	destination.push_back('\xc1');
	::write_bytes<int32_t>(static_cast<int32_t>(error_code), destination);
	
	destination.push_back('\x41'); //jmp [r10 + 16]
	destination.push_back('\xff');
	destination.push_back('\x62');
	destination.push_back('\x10');
}
void generate_stack_allocation_code(std::vector<char>& destination, uint32_t size) {
	if (size == 0) return;

	destination.push_back('\x48'); //add rbp, size
	destination.push_back('\x81');
	destination.push_back('\xc5');

	write_bytes(size, destination);

	destination.push_back('\x4c'); //cmp rbp, r9
	destination.push_back('\x39');
	destination.push_back('\xcd');

	destination.push_back('\x72'); //jb end
	destination.push_back(char{ program_termination_code_size });

	generate_program_termination_code(destination, termination_codes::stack_overflow);

	//:end
}
void generate_stack_deallocation_code(std::vector<char>& destination, uint32_t size) {
	if (size == 0) return;

	destination.push_back('\x48'); //sub rbp, size
	destination.push_back('\x81');
	destination.push_back('\xed');

	write_bytes(size, destination);
}
uint32_t generate_function_prologue(std::vector<char>& destination, uint32_t allocation_size, const memory_layouts_builder::memory_addresses& locals) {
	destination.push_back('\x48'); //mov rax, [rsp]
	destination.push_back('\x8b');
	destination.push_back('\x04');
	destination.push_back('\x24');

	destination.push_back('\x48'); //mov [rbp], rax
	destination.push_back('\x89');
	destination.push_back('\x45');
	destination.push_back('\x00');

	destination.push_back('\x48'); //add rsp, 8
	destination.push_back('\x83');
	destination.push_back('\xc4');
	destination.push_back('\x08');

	generate_stack_allocation_code(destination, allocation_size + 8); //8 = return address
	return nullify_function_pointer_variables(destination, locals) + stack_allocation_code_size + 12;
}
void generate_function_epilogue(std::vector<char>& destination, uint32_t deallocation_size, uint32_t arguments_deallocation_size) {
	generate_stack_deallocation_code(destination, deallocation_size + 8); //8 = return address

	destination.push_back('\x48'); //mov rax, rbp
	destination.push_back('\x89');
	destination.push_back('\xe8');

	generate_stack_deallocation_code(destination, arguments_deallocation_size);

	destination.push_back('\xff'); //jmp [rax]
	destination.push_back('\x20');
}
void* create_executable_function(const std::vector<char>& source) {
	char* destination = (char*)VirtualAlloc(NULL, source.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (destination == NULL) {
		return nullptr;
	}

	std::copy(source.cbegin(), source.cend(), destination);

	DWORD previous_protection = 0;
	PDWORD previous_protection_pointer = &previous_protection;
	VirtualProtect(destination, source.size(), PAGE_EXECUTE, previous_protection_pointer); //W^E

	FlushInstructionCache(GetCurrentProcess(), destination, source.size());
	return destination;
}