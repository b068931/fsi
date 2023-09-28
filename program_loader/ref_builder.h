#ifndef REF_BUILDER_H
#define REF_BUILDER_H

#include "instruction_builder.h"

class ref_builder : public instruction_builder {
public:
	template<typename... args>
	ref_builder(
		const std::vector<char>* machine_codes,
		args&&... instruction_builder_arguments
	) 
		:instruction_builder{ std::forward<args>(instruction_builder_arguments)... }
	{
		this->assert_statement(!machine_codes, "This instruction must have no machine codes.");
		this->assert_statement(this->get_arguments_count() == 2, "This instruction must have only two arguments.");
	}

	virtual void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
		this->assert_statement(pointer->get_active_type() == 0b11, "You must use 'ebyte' as the active type.", pointer->get_id());
		this->accumulate_pointer_offset(pointer.get());
		this->add_base_address_to_pointer_dereference(pointer.get());
		if (this->get_argument_index() == 0) {
			this->use_r8_on_reg('\x8b', false, 0b11); //mov rax, r8
		}
		else {
			this->use_r8_on_reg('\x8b', true, 0b11, true); //mov r8, [r8]
		}
	}
	virtual void visit(std::unique_ptr<pointer> pointer) override {
		if (this->get_argument_index() == 0) {
			this->load_variable_address(pointer->get_id());
		}
		else {
			this->load_pointer_info(this->get_variable_info(pointer->get_id()));

			this->write_bytes('\x4d'); //mov r8, r15
			this->write_bytes('\x89');
			this->write_bytes('\xf8');
		}
	}

	virtual void build() override {
		this->self_call_next();
		this->self_call_next();

		this->write_bytes('\x4c'); //mov [rax], r8
		this->write_bytes('\x89');
		this->write_bytes('\x00');
	}
};

#endif