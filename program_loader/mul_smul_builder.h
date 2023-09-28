#ifndef MUL_SMUL_BUILDER_H
#define MUL_SMUL_BUILDER_H

#include <memory> //std::unique_ptr
#include <utility> //std::forward
#include "complex_arithmetic_instruction_builder.h"

class mul_smul_builder : public complex_arithmetic_instruction_builder {
private:
	void multiply_rax_using_r8() {
		this->use_r8_on_reg_with_two_opcodes(this->get_code_front(), this->get_code(1), false, this->get_active_type(), false, this->get_code_back());
	}

public:
	template<typename... args>
	mul_smul_builder(
		args&&... instruction_builder_args
	)
		:complex_arithmetic_instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{
		this->assert_statement(this->get_arguments_count() >= 2, "This instruction must have at least two arguments.");
	}

	virtual void visit(std::unique_ptr<regular_variable> variable) override {
		this->assert_statement(variable->is_valid_active_type(), "Variable has an incorrect active type.", variable->get_id());
		if (this->get_argument_index() == 0) {
			this->load_variable_address(variable->get_id(), true);
			this->move_r8_to_rbx();

			this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', false, variable.get());
			this->set_active_type(variable->get_active_type());
		}
		else {
			this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', true, variable.get());
			this->multiply_rax_using_r8();
		}
	}
	virtual void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
		this->accumulate_pointer_offset(pointer.get());
		this->add_base_address_to_pointer_dereference(pointer.get());

		if (this->get_argument_index() == 0) {
			this->move_r8_to_rbx();

			this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type());
			this->set_active_type(pointer->get_active_type());
		}
		else {
			this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type(), true);
			this->multiply_rax_using_r8();
		}
	}

	virtual void build() override {
		this->zero_rax();
		while (this->get_arguments_count() != 0) {
			this->self_call_next();
		}

		this->store_value_from_rax_to_rbx();
	}
};

#endif // !MUL_SMUL_BUILDER_H