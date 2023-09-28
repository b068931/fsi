#ifndef COMPLEX_ARITHMETIC_INSTRUCTION_BUILDER_H
#define COMPLEX_ARITHMETIC_INSTRUCTION_BUILDER_H

#include <utility> //std::forward
#include "arithmetic_instruction_builder.h"

class complex_arithmetic_instruction_builder : public arithmetic_instruction_builder {
protected:
	template<typename... args>
	complex_arithmetic_instruction_builder(
		args&&... instruction_builder_args
	)
		:arithmetic_instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{}

	void check_if_r8_is_zero() {
		this->write_bytes('\x49'); //cmp r8, 0
		this->write_bytes('\x83');
		this->write_bytes('\xf8');
		this->write_bytes('\x00');

		this->write_bytes('\x75');
		this->write_bytes(static_cast<uint8_t>(::program_termination_code_size));

		this->generate_program_termination_code(termination_codes::division_by_zero);
	}

	void move_r8_to_rbx() {
		this->write_bytes('\x49'); //mov rbx, r8
		this->write_bytes('\x8b');
		this->write_bytes('\xd8');
	}
	void store_value_from_rax_to_rbx() {
		uint8_t rex = 0;
		switch (this->get_active_type()) {
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
			this->write_bytes(rex);
		}

		if (this->get_active_type() == 0b00) {
			this->write_bytes('\x88');
		}
		else {
			this->write_bytes('\x89');
		}

		this->write_bytes('\x03'); //r/m
	}
};

#endif // !COMPLEX_ARITHMETIC_INSTRUCTION_BUILDER_H