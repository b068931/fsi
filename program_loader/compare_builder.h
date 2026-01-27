#ifndef COMPARE_BUILDER_H
#define COMPARE_BUILDER_H

#include "pch.h"
#include "machine_codes_instruction_builder.h"

class compare_builder : public machine_codes_instruction_builder {
private:
	std::uint8_t argument_index; //first argument will be placed in rax, while second one will be placed in r8

	template<typename T>
	void place_immediate_value(T value) {
		this->zero_r8();
		this->create_immediate_instruction(value->get_value());
		if (this->argument_index == 0) {
			this->use_r8_on_reg('\x8b', false, 0b11);
		}

		++this->argument_index;
	}
public:
	template<typename... args>
	compare_builder(
		args&&... instruction_builder_args
	)
		:machine_codes_instruction_builder{ std::forward<args>(instruction_builder_args)... },
		argument_index{ 0 }
	{
		this->assert_statement(this->check_if_active_types_match() && this->get_arguments_count() == 2,
			"Active types for this instruction must match. Arguments count must be equal to 2."); //this instruction works only with 2 arguments
	}

	virtual void visit(std::unique_ptr<variable_imm<std::uint8_t>> value) override {
		this->place_immediate_value(value.get());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint16_t>> value) override {
		this->place_immediate_value(value.get());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint32_t>> value) override {
		this->place_immediate_value(value.get());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint64_t>> value) override {
		this->place_immediate_value(value.get());
	}
	virtual void visit(std::unique_ptr<regular_variable> variable) override {
		this->assert_statement(variable->is_valid_active_type(), "Variable has incorrect active type.", variable->get_id());
		bool r8_or_rax = this->argument_index == 1; //check in which register we should put this value depending on its index
		if (r8_or_rax) { //r8 = true, rax = false
			this->zero_r8();
		}

		this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', r8_or_rax, variable.get());
		++this->argument_index;
	}
	virtual void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
		this->accumulate_pointer_offset(pointer.get());
		this->add_base_address_to_pointer_dereference(pointer.get());

		if (this->argument_index == 0) {
			this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type());
		}
		else {
			this->zero_r15(); //move value stored at the address in r8 to r15, zero r8 and then move it back

			this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type(), true, 0b111);
			this->zero_r8();

			this->write_bytes('\x4d'); //mov r8, r15
			this->write_bytes('\x8b');
			this->write_bytes('\xc7');
		}

		++this->argument_index;
	}

	virtual void build() override {
		this->zero_rax(); //make sure that used registers do not contain any unexpected values, we do this because we are going to use whole registers and not their parts

		this->self_call_next();
		this->self_call_next();

		this->write_bytes('\x49'); //cmp rax, r8
		this->write_bytes(this->get_code_back());
		this->write_bytes('\xc0');

		this->write_bytes('\x66'); //pushfw
		this->write_bytes('\x9c');

		this->write_bytes('\x66'); //mov ax, [rsp]
		this->write_bytes('\x8b');
		this->write_bytes('\x04');
		this->write_bytes('\x24');

		this->write_bytes('\x66'); //mov [rcx], ax - save lower 16 bits of eflags to thread state (we need to save flags that are affected by cmp to restore them later)
		this->write_bytes('\x89');
		this->write_bytes('\x01');

		this->write_bytes('\x48'); //add rsp, 2
		this->write_bytes('\x83');
		this->write_bytes('\xc4');
		this->write_bytes('\x02');
	}
};

#endif // !COMPARE_BUILDER_H
