#ifndef MACHINE_CODES_INSTRUCTION_BUILDER_H
#define MACHINE_CODES_INSTRUCTION_BUILDER_H

#include "pch.h"
#include "instruction_builder.h"

class machine_codes_instruction_builder : public instruction_builder {
private:
	const std::vector<char>* machine_codes;

protected:
	template<typename... args>
	machine_codes_instruction_builder(
		const std::vector<char>* machine_codes,
		args&&... instruction_builder_args
	)
		:machine_codes{ machine_codes },
		instruction_builder{ std::forward<args>(instruction_builder_args)... }
	{
		this->assert_statement(machine_codes, "This instruction must have machine codes."); //check whether machine codes for this instruction were passed
	}

	std::size_t get_codes_count() const { return this->machine_codes->size(); }
	char get_code(std::size_t index) const { return (*this->machine_codes)[index]; }
	char get_code_back() const { return this->machine_codes->back(); }
	char get_code_front() const { return this->machine_codes->front(); }
};

#endif