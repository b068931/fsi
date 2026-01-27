#ifndef COMPILED_PROGRAM_STRUCTURE_H
#define COMPILED_PROGRAM_STRUCTURE_H

#include "pch.h"

struct compiled_program {
	std::uint32_t main_function_index;
	std::uint64_t preferred_stack_size;

	void** compiled_functions;
	std::uint32_t functions_count;

	void** exposed_functions;
	std::uint32_t exposed_functions_count;

	void* jump_table;
	std::uint64_t jump_table_size;

	void** program_strings;
	std::uint64_t program_strings_count;
};

#endif
