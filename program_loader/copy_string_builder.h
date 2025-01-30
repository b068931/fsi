#ifndef COPY_STRING_BUILDER_H
#define COPY_STRING_BUILDER_H

#include "instruction_builder.h"

class copy_string_builder : public instruction_builder {
public:
	template<typename... args>
	copy_string_builder(
		const std::vector<char>* machine_codes,
		args&&... instruction_builder_args
	)
		:instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{
		this->assert_statement(this->get_arguments_count() == 2 && !machine_codes, "This instruction must have only two arguments.");
	}

	virtual void visit(std::unique_ptr<pointer> pointer) override {
		this->assert_statement(this->get_argument_index() == 0, "Pointer must be used as the first argument.", pointer->get_id());
		this->load_pointer_info(this->get_variable_info(pointer->get_id()));
	}
	virtual void visit(std::unique_ptr<specialized_variable> string) override {
		this->assert_statement(this->get_argument_index() == 1, "String must be used as the second argument.", string->get_id());
		auto& found_string_info = this->get_string_info(string->get_id());
		this->create_immediate_instruction<std::uint64_t>(found_string_info.second - 1);
		this->generate_pointer_size_check();

		this->write_bytes('\x4c'); //mov rcx, r8
		this->write_bytes('\x89');
		this->write_bytes('\xc1');

		this->write_bytes('\x49'); //mov rdi, qword ptr [r15 + 8]
		this->write_bytes('\x8b');
		this->write_bytes('\x7f');
		this->write_bytes('\x08');

		this->write_bytes('\x48'); //mov rsi, address of the string
		this->write_bytes('\xbe');
		this->write_bytes<std::uint64_t>(reinterpret_cast<uintptr_t>(found_string_info.first.get()));

		this->write_bytes('\xf3'); //rep movsb + REX.W
		this->write_bytes('\x48');
		this->write_bytes('\xa4');
	}

	virtual void build() override {
		this->move_rcx_to_rbx();

		this->self_call_next();
		this->self_call_next();

		this->move_rbx_to_rcx();
	}
};

#endif