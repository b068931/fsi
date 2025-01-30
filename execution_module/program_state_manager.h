#ifndef PROGRAM_STATE_MANAGER_H
#define PROGRAM_STATE_MANAGER_H

#include <cstdint>
#include <cstring>

class program_state_manager {
private:
	char* program_state;
	
	static constexpr std::size_t comparsion_state_displacement = 0;
	static constexpr std::size_t return_address_displacement = 8;
	static constexpr std::size_t jump_table_displacement = 16;
	static constexpr std::size_t my_state_address_displacement = 24;
	static constexpr std::size_t current_stack_position_displacement = 32;
	static constexpr std::size_t stack_end_displacement = 40;
	static constexpr std::size_t program_control_functions_displacement = 48;

	template<typename type>
	type generic_read_state_entry(std::size_t displacement) {
		type result{};
		std::memcpy(&result, this->program_state + displacement, sizeof(type));

		return result;
	}

	template<typename type>
	void generic_write_state_entry(std::size_t displacement, type value) {
		std::memcpy(this->program_state + displacement, &value, sizeof(type));
	}

public:
	program_state_manager(char* program_state)
		:program_state{ program_state }
	{}

	std::uint64_t get_comparstion_state() {
		return this->generic_read_state_entry<std::uint64_t>(comparsion_state_displacement);
	}
	void set_comparstion_state(std::uint64_t value) {
		this->generic_write_state_entry<std::uint64_t>(comparsion_state_displacement, value);
	}

	std::uintptr_t get_return_address() {
		return this->generic_read_state_entry<std::uintptr_t>(return_address_displacement);
	}
	void set_return_address(std::uintptr_t value) {
		this->generic_write_state_entry<std::uintptr_t>(return_address_displacement, value);
	}

	std::uintptr_t get_jump_table_address() {
		return this->generic_read_state_entry<std::uintptr_t>(jump_table_displacement);
	}
	void set_jump_table_address(std::uintptr_t value) {
		this->generic_write_state_entry(jump_table_displacement, value);
	}

	std::uintptr_t get_my_state_address() {
		return this->generic_read_state_entry<std::uintptr_t>(my_state_address_displacement);
	}

	std::uintptr_t get_current_stack_position() {
		return this->generic_read_state_entry<std::uintptr_t>(current_stack_position_displacement);
	}
	void set_current_stack_position(std::uintptr_t value) {
		this->generic_write_state_entry<std::uintptr_t>(current_stack_position_displacement, value);
	}

	std::uintptr_t get_stack_end() {
		return this->generic_read_state_entry<std::uintptr_t>(stack_end_displacement);
	}
	void set_stack_end(std::uintptr_t value) {
		this->generic_write_state_entry<std::uintptr_t>(stack_end_displacement, value);
	}

	std::uintptr_t get_program_control_functions() {
		return this->generic_read_state_entry<std::uintptr_t>(program_control_functions_displacement);
	}
	void set_program_control_functions(std::uintptr_t value) {
		this->generic_write_state_entry<std::uintptr_t>(program_control_functions_displacement, value);
	}
};

#endif