#ifndef GENERAL_FUNCTION_CALL_BUILDER_H
#define GENERAL_FUNCTION_CALL_BUILDER_H

#include <stdint.h>
#include <utility>
#include <vector>
#include "instruction_builder.h"

class general_function_call_builder : public instruction_builder {
protected:
	template<typename... args>
	general_function_call_builder(
		const std::vector<char>* machine_codes,
		args&&... instruction_builder_args
	)
		:instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{
		this->assert_statement(!machine_codes, "Builder does not use machine codes."); //this builder does not use machine codes
	}

	uint8_t translate_current_type_to_memory_layouts_builder_type() {
		uint8_t translated_type = 0; //0 - byte, 1 - two_bytes, 2 - four_bytes, 3 - eight_bytes, 4 - pointer
		if (this->get_current_variable_type() == 0b1111) {
			translated_type = 4;
		}
		else if (this->get_current_variable_type() == 0b1110) {
			translated_type = 3;
		}
		else {
			translated_type = this->get_current_variable_type() & 0b11;
		}

		return translated_type;
	}

	template<typename T>
	void place_immediate(T value, int32_t displacement, uint8_t rm) {
		using get_value_type = decltype(value->get_value());
		this->create_immediate_instruction(value->get_value());

		if constexpr (std::is_same_v<get_value_type, uint8_t>) {
			this->move_value_from_reg000_to_memory(0b00, true, displacement, rm);
		}
		else if (std::is_same_v<get_value_type, uint16_t>) {
			this->move_value_from_reg000_to_memory(0b01, true, displacement, rm);
		}
		else if (std::is_same_v<get_value_type, uint32_t>) {
			this->move_value_from_reg000_to_memory(0b10, true, displacement, rm);
		}
		else if (std::is_same_v<get_value_type, uint64_t>) {
			this->move_value_from_reg000_to_memory(0b11, true, displacement, rm);
		}
	}
	void move_value_from_reg000_to_memory(uint8_t active_type, bool is_R, int32_t displacement, uint8_t rm) {
		uint8_t rex = 0;
		if (is_R) {
			rex |= 0b01000100;
		}

		switch (active_type) {
		case 0b01: {
			this->write_bytes('\x66');
			break;
		}
		case 0b11: {
			rex |= 0b01001000;
			break;
		}
		}

		if (rex != 0) {
			this->write_bytes(static_cast<char>(rex));
		}

		if (active_type == 0b00) {
			this->write_bytes('\x88');
		}
		else {
			this->write_bytes('\x89');
		}

		this->write_bytes<char>('\x80' | (rm & 0b111));
		this->write_bytes(displacement);
	}
};

#endif // !GENERAL_FUNCTION_CALL_BUILER
