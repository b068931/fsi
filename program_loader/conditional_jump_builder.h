#ifndef JCC_BUILDER_H
#define JCC_BUILDER_H

#include "pch.h"
#include "machine_codes_instruction_builder.h"

class conditional_jump_builder : public machine_codes_instruction_builder {
	const std::uint32_t jump_rel32 = 7;

public:
	template<typename... args>
	conditional_jump_builder(
		args&&... instruction_builder_args
	)
		:machine_codes_instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{
		this->assert_statement(this->get_arguments_count() == 1, "This instruction must have only one argument.");
	}

	virtual void visit(std::unique_ptr<specialized_variable> variable) override {
		this->write_bytes('\x66'); //mov ax, [rcx]
		this->write_bytes('\x8b');
		this->write_bytes('\x01');

		this->write_bytes('\x48'); //sub rsp, 2
		this->write_bytes('\x83');
		this->write_bytes('\xec');
		this->write_bytes('\x02');

		this->write_bytes('\x66'); //mov [rsp], ax
		this->write_bytes('\x89');
		this->write_bytes('\x04');
		this->write_bytes('\x24');

		this->write_bytes('\x66'); //popfw
		this->write_bytes('\x9d');

		for (std::size_t index = 0; index < this->get_codes_count(); ++index) { //jcc rel32
			this->write_bytes(this->get_code(index));
		}
		this->write_bytes(this->jump_rel32);

		this->write_bytes('\x41'); //jmp [r11+disp32]
		this->write_bytes('\xff');
		this->write_bytes('\xa3');
		this->write_bytes(static_cast<std::uint32_t>(this->get_jump_point_table_index(variable->get_id())));
	}
	virtual void build() override {
		this->zero_rax();
		this->self_call_next();
	}
};

#endif // !JCC_BUILDER_H
