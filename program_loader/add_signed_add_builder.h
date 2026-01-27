#ifndef ADD_SIGNED_ADD_BUILDER_H
#define ADD_SIGNED_ADD_BUILDER_H

#include "pch.h"
#include "arithmetic_instruction_builder.h"

class add_signed_add_builder : public arithmetic_instruction_builder {
private:
	void add_r8_to_rax() {
		std::uint8_t rex = '\x44';

		switch (this->get_active_type()) {
		case 0b01: {
			this->write_bytes('\x66');
			break;
		}

		case 0b11: {
			rex |= 0b00001000;
			break;
		}

		default: break;
		}

		this->write_bytes(rex);

		this->write_bytes(this->get_code(this->get_active_type()));
		this->write_bytes('\x00');
	}

public:
	using instruction_builder::visit;

	template<typename... args>
	add_signed_add_builder(
		args&&... instruction_builder_args
	)
		:arithmetic_instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{
		this->assert_statement(this->get_arguments_count() >= 2, "Instruction must have at least two arguments.");
	}

	virtual void visit(std::unique_ptr<regular_variable> variable) override {
		this->assert_statement(variable->is_valid_active_type(), "Variable has an incorrect active type.", variable->get_id());
		if (this->get_argument_index() == 0) {
			this->load_variable_address(variable->get_id());
			this->set_active_type(variable->get_active_type());
		}
		else {
			this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', true, variable.get());
		}
	}
	virtual void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
		this->accumulate_pointer_offset(pointer.get());
		this->add_base_address_to_pointer_dereference(pointer.get());

		if (this->get_argument_index() == 0) {
			this->use_r8_on_reg('\x8b', false, 0b11);
			this->set_active_type(pointer->get_active_type());
		}
		else {
			this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type(), true);
		}
	}

	virtual void build() override {
		while (this->get_arguments_count() != 0) {
			std::uint8_t current_argument_index = this->get_argument_index();
			this->self_call_next();

			if (current_argument_index != 0) {
				this->add_r8_to_rax();
			}
		}
	}
};

#endif // !ADD_SIGNED_ADD_BUILDER_H
