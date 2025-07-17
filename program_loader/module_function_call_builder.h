#ifndef MODULE_FUNCTION_CALL_BUILDER_H
#define MODULE_FUNCTION_CALL_BUILDER_H

#include "pch.h"
#include "run_container.h"
#include "memory_layouts_builder.h"
#include "general_function_call_bulider.h"
#include "../module_mediator/module_part.h"

class module_function_call_builder : public general_function_call_builder {
	/*we are going to iterate through elements two times :
	1. collect information about variables, calculate needed stack size,
	allocate it and fill in first part of the args string
	2. fill in second part of the args string*/
private:
	std::uint32_t stack_allocation_size;

	const runs_container::engine_module* associated_module;
	std::size_t function_to_call_index;

	std::int8_t variable_index;
	bool is_second_time;

	std::int32_t current_rbx_displacement;

	std::vector<std::uint8_t> args_types;
	std::vector<std::int32_t> arguments_relative_addresses;
	std::vector<variable*> variables;

	std::uint8_t translate_to_args_string_type(std::uint8_t type_bits) {
		std::uint8_t return_value = 0;
		if (type_bits == 0b1111) {
			return_value = 10;
		}
		else if (type_bits == 0b1110) {
			return_value = 9;
		}
		else {
			std::uint8_t active_type = type_bits & 0b11;
			if (active_type == 0b11) {
				return_value = 8;
			}
			else {
				return_value = active_type * 2;
			}

			if ((type_bits & 0b1100) != 0) {
				++return_value;
			}
		}

		return return_value;
	}
	std::uint8_t get_type_size() {
		std::uint8_t translated_type = this->translate_current_type_to_memory_layouts_builder_type();

		memory_layouts_builder::variable_size size = memory_layouts_builder::get_variable_size(translated_type);
		if (size != memory_layouts_builder::variable_size::UNKNOWN) {
			return static_cast<std::uint8_t>(size);
		}

		return 0;
	}

	void prologue(variable* var) {
		this->args_types.push_back(
			this->translate_to_args_string_type(this->get_current_variable_type())
		);

		this->arguments_relative_addresses.push_back(this->current_rbx_displacement);
		this->current_rbx_displacement += this->get_type_size();

		this->variables.push_back(var);
	}
	void move_value_from_reg000_to_args_string(std::uint8_t active_type, std::int32_t displacement, bool is_R) {
		this->move_value_from_reg000_to_memory(active_type, is_R, displacement, 0b011);
	}

	template<typename T>
	void place_immediate(T value) {
		if (this->is_second_time) {
			this->general_function_call_builder::place_immediate(value, this->arguments_relative_addresses[this->variable_index], 0b011);
			++this->variable_index;

			delete value;
		}
		else {
			this->prologue(value);
		}
	}
	void place_generic_variable(generic_variable* variable) {
		if (this->is_second_time) {
			this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', false, variable, 0, this->stack_allocation_size);
			this->move_value_from_reg000_to_args_string(variable->get_active_type(), this->arguments_relative_addresses[this->variable_index], false);

			++this->variable_index;
			delete variable;
		}
		else {
			this->prologue(variable);
		}
	}

	void put_byte(std::int32_t& displacement, std::uint8_t value) {
		this->write_bytes('\xc6');
		this->write_bytes('\x85');
		this->write_bytes(displacement);
		this->write_bytes(value);

		++displacement;
	}

public:
	template<typename... args>
	module_function_call_builder(
		args&&... instruction_builder_args
	)
		:general_function_call_builder{ std::forward<args>(instruction_builder_args)... },
		associated_module{ nullptr },
		function_to_call_index{ module_mediator::module_part::function_not_found },
		current_rbx_displacement{ 0 },
		variable_index{ 0 },
		is_second_time{ false },
		stack_allocation_size{ 0 }
	{}

	virtual void visit(std::unique_ptr<engine_module> engine_module) override {
		auto found_module = this->get_file_info().modules.find(engine_module->get_id());
		if (found_module == this->get_file_info().modules.cend()) {
			this->assert_statement(false, "Module does not exist.", engine_module->get_id());
			return;
		}

		this->associated_module = &(found_module->second);
	}
	virtual void visit(std::unique_ptr<function> fnc) override {
		if (this->function_to_call_index == module_mediator::module_part::function_not_found) {
			auto found_fnc = this->associated_module->module_functions.find(fnc->get_id());
			if (found_fnc == this->associated_module->module_functions.cend()) {
				this->assert_statement(false, "Module function does not exist.", fnc->get_id());
				return;
			}

			this->function_to_call_index = found_fnc->second;
		}
		else {
			if (this->is_second_time) {
				this->zero_rax();

				this->write_bytes('\xb8'); //mov eax, imm32
				this->write_bytes(static_cast<std::int32_t>(this->get_function_table_index(fnc->get_id())));

				this->move_value_from_reg000_to_args_string(0b11, this->arguments_relative_addresses[this->variable_index], false);
				++this->variable_index;
			}
			else {
				this->prologue(fnc.release());
			}
		}
	}
	virtual void visit(std::unique_ptr<specialized_variable> variable) override { //will be called before all other arguments
		if (this->is_second_time) {
			if (variable->get_id() != 0) {
				this->variable_index = 2; //includes pointer and type for return value

				auto variable_info = this->get_variable_info(variable->get_id());
				this->write_bytes('\x48'); //lea rax, [rbp + disp32]
				this->write_bytes('\x8d');
				this->write_bytes('\x85');
				this->write_bytes(variable_info.first - this->stack_allocation_size);
				this->move_value_from_reg000_to_args_string(0b11, this->arguments_relative_addresses[0], false);

				this->write_bytes('\xb0'); //mov al, imm8
				this->write_bytes(variable_info.second);
				this->move_value_from_reg000_to_args_string(0b00, this->arguments_relative_addresses[1], false);
			}
			else {
				this->variable_index = 0;
			}
		}
		else {
			if (this->current_rbx_displacement != 0) return; //ignogre if it's not a first argument to be processed
			if (variable->get_id() != 0) {
				this->args_types.push_back(10); //pointer to a return value
				this->args_types.push_back(1); //retrun value type

				this->current_rbx_displacement =
					static_cast<std::uint8_t>(memory_layouts_builder::get_variable_size(4)) +
					static_cast<std::uint8_t>(memory_layouts_builder::get_variable_size(0)); //pointer + pointer + byte

				this->arguments_relative_addresses.push_back(0);
				this->arguments_relative_addresses.push_back(8);
			}
			else {
				this->current_rbx_displacement = 0;
			}

			this->variables.push_back(variable.release());
		}
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint8_t>> value) override {
		this->place_immediate(value.release());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint16_t>> value) override {
		this->place_immediate(value.release());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint32_t>> value) override {
		this->place_immediate(value.release());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint64_t>> value) override {
		this->place_immediate(value.release());
	}
	virtual void visit(std::unique_ptr<pointer> pointer) override {
		if (this->is_second_time) {
			this->load_pointer_info(
				this->get_variable_info(pointer->get_id()), 
				this->stack_allocation_size
			);

			this->write_bytes('\x49'); //mov rax, r15
			this->write_bytes('\x8b');
			this->write_bytes('\xc7');

			this->move_value_from_reg000_to_args_string(0b11, this->arguments_relative_addresses[this->variable_index], false);
			++this->variable_index;
		}
		else {
			this->prologue(pointer.release());
		}
	}
	virtual void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
		if (this->is_second_time) {
			this->accumulate_pointer_offset(pointer.get(), this->stack_allocation_size);
			this->add_base_address_to_pointer_dereference(pointer.get(), this->stack_allocation_size);

			this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type());
			this->move_value_from_reg000_to_args_string(pointer->get_active_type(), this->arguments_relative_addresses[this->variable_index], false);

			++this->variable_index;
		}
		else {
			this->prologue(pointer.release());
		}
	}
	virtual void visit(std::unique_ptr<regular_variable> variable) override {
		this->place_generic_variable(variable.release());
	}
	virtual void visit(std::unique_ptr<signed_variable> variable) override {
		this->place_generic_variable(variable.release());
	}

	virtual void build() override {
		this->self_call_by_type(0b1100);
		this->self_call_by_type(0b1110);
		this->self_call_by_type(0b1101);
		while (this->get_arguments_count() != 0) {
			this->self_call_next();
		}

		std::int32_t args_string_size = static_cast<std::int32_t>(1 + this->args_types.size() + this->current_rbx_displacement);
		std::int32_t displacement = -args_string_size;

		this->stack_allocation_size = args_string_size;
		this->generate_stack_allocation_code(args_string_size);

		this->put_byte(displacement, static_cast<std::uint8_t>(this->args_types.size())); //initialize args string types
		for (std::uint8_t type : this->args_types) {
			this->put_byte(displacement, type);
		}

		this->write_bytes('\x48'); //lea rbx, [rbp+disp32]
		this->write_bytes('\x8d');
		this->write_bytes('\x9d');
		this->write_bytes(displacement);

		this->is_second_time = true; //initialize args string values
		for (variable* var : this->variables) {
			var->visit(this);
		}

		this->write_bytes('\x48'); //mov rax, imm64
		this->write_bytes('\xb8');
		this->write_bytes(this->associated_module->module_id);

		this->write_bytes('\x49'); //mov r8, imm64
		this->write_bytes('\xb8');
		this->write_bytes(this->function_to_call_index);

		this->write_bytes('\x4c'); //lea r15, [rbp+disp32]
		this->write_bytes('\x8d');
		this->write_bytes('\xbd');
		this->write_bytes(-args_string_size);

		this->write_bytes('\x41'); //call [r10]
		this->write_bytes('\xff');
		this->write_bytes('\x12');

		this->generate_stack_deallocation_code(args_string_size);
	}
};

#endif // !MODULE_FUNCTION_CALL_BUILDER_H