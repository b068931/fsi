#ifndef ARITHMETIC_INSTRUCTION_BUILDER_H
#define ARITHMETIC_INSTRUCTION_BUILDER_H

#include <memory> //std::unique_ptr
#include <utility> //std::forward
#include "machine_codes_instruction_builder.h"

class arithmetic_instruction_builder : public machine_codes_instruction_builder { //implements visits for immediate values and saves them in r8
private:
	std::uint8_t active_type;

	template<typename T>
	void place_immediate(T value) {
		this->assert_statement(this->get_argument_index() != 0, "Immediate cannot be used as the first argument."); //immediate can not be used as the first argument here
		this->create_immediate_instruction(value->get_value());
	}

protected:
	template<typename... args>
	arithmetic_instruction_builder(
		args&&... instruction_builder_args
	)
		:machine_codes_instruction_builder{ std::forward<args>(instruction_builder_args)... },
		active_type{}
	{
		this->assert_statement(this->check_if_active_types_match(), "Active types of the variables must match.");
	}

	std::uint8_t get_active_type() const { return this->active_type; }
	void set_active_type(std::uint8_t active_type) { this->active_type = active_type; }

public:
	virtual void visit(std::unique_ptr<variable_imm<std::uint8_t>> value) override {
		this->place_immediate(value.get());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint16_t>> value) override {
		this->place_immediate(value.get());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint32_t>> value) override {
		this->place_immediate(value.get());
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint64_t>> value) override {
		this->place_immediate(value.get());
	}
};

#endif // !ARITHMETIC_INSTRUCTION_BUILDER_H