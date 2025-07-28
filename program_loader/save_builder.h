#ifndef SAVE_BUILDER_H
#define SAVE_BUILDER_H

#include "instruction_builder.h"

class save_builder : public instruction_builder {
private:
	std::uint8_t used_variable_type{};

	template<typename T>
	void place_immediate(T value) {
		this->used_variable_type = this->get_current_variable_type() & 0b11;
		this->create_immediate_instruction(value);
	}
	void save_value_from_r8_to_program_stack(std::uint8_t active_type) {
		std::uint8_t rex = 0x45;

		switch (active_type) {
		case 0b01: { //add prefix that indicates that this is 16 bit instruction
			this->write_bytes('\x66');
			break;
		}
		case 0b11: { //add REX prefix
			rex |= 0b01001000;
			break;
		}
		}

		this->write_bytes(static_cast<char>(rex)); //rex
		if (active_type == 0) {
			this->write_bytes('\x88');
		}
		else {
			this->write_bytes('\x89');
		}

		this->write_bytes('\x01');
	}

public:
	template<typename... args>
	save_builder(
		const std::vector<char>* machine_codes,
		args&&... instruction_builder_args
	)
		:instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{
		this->assert_statement(this->get_arguments_count() == 1 && !machine_codes, "This instruction must have only one argument.");
	}

	virtual void visit(std::unique_ptr<variable_imm<std::uint8_t>> value) override {
		this->place_immediate(value->get_value());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint16_t>> value) override {
		this->place_immediate(value->get_value());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint32_t>> value) override {
		this->place_immediate(value->get_value());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint64_t>> value) override {
		this->place_immediate(value->get_value());
	}

	virtual void visit(std::unique_ptr<regular_variable> variable) override {
		this->assert_statement(variable->is_valid_active_type(), "Variable has an incorrect active type.", variable->get_id());
		this->used_variable_type = variable->get_active_type();
		this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', true, variable.get());
	}

	virtual void visit(std::unique_ptr<pointer> pointer) override {
		this->used_variable_type = 4;
		this->load_pointer_info(this->get_variable_info(pointer->get_id()));

		this->write_bytes('\x4d'); //mov r8, r15
		this->write_bytes('\x89');
		this->write_bytes('\xf8');
	}
	virtual void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
		this->used_variable_type = pointer->get_active_type();

		this->accumulate_pointer_offset(pointer.get());
		this->add_base_address_to_pointer_dereference(pointer.get());

		this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type(), true);
	}

	virtual void build() override {
		this->zero_r8();
		this->self_call_next();
		std::uint8_t active_type = this->used_variable_type;
		if (active_type == 4) active_type = 0b11;

		this->save_type_to_program_stack(this->used_variable_type);
		this->save_value_from_r8_to_program_stack(
			this->used_variable_type == 4 ? 3 : this->used_variable_type
		);
	}
};

#endif // !SAVE_BUILDER_H