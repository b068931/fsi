#ifndef AUXILIARY_FUNCTIONS_H
#define AUXILIARY_FUNCTIONS_H

#include <cstdint>
#include <vector>
#include "memory_layouts_builder.h"

enum class termination_codes : std::int32_t {
	stack_overflow = 1,
	nullptr_dereference,
	pointer_out_of_bounds,
	undefined_function_call,
	incorrect_saved_variable_type,
	division_by_zero
};

constexpr std::uint32_t program_termination_code_size = 11;
constexpr std::uint32_t stack_allocation_code_size = 12 + program_termination_code_size;
constexpr std::uint32_t function_save_return_address_size = 12;

template<typename T>
void write_bytes(T value, std::vector<char>& destination) {
	char* symbols = reinterpret_cast<char*>(&value);
	for (std::size_t counter = 0; counter < sizeof(value); ++counter) {
		destination.push_back(symbols[counter]);
	}
}

void generate_program_termination_code(std::vector<char>& destination,  termination_codes error_code);
void generate_stack_allocation_code(std::vector<char>& destination, std::uint32_t size);
void generate_stack_deallocation_code(std::vector<char>& destination, std::uint32_t size);
std::uint32_t generate_function_prologue(std::vector<char>& destination, std::uint32_t allocation_size, const memory_layouts_builder::memory_addresses& locals);
void generate_function_epilogue(std::vector<char>& destination, std::uint32_t deallocation_size, std::uint32_t arguments_deallocation_size);
void* create_executable_function(const std::vector<char>& source);

#endif // !AUXILIARY_FUNCTIONS_H