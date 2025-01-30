#ifndef SIGNED_VARIABLE_H
#define SIGNED_VARIABLE_H

#include "generic_variable.h"
#include <cstdint>

class signed_variable : public generic_variable {
public:
	signed_variable(std::uint8_t real_type, std::uint8_t active_type, entity_id id)
		:generic_variable{ real_type, active_type, id }
	{}

	virtual void visit(instruction_builder* builder) override;
};

#endif // !SIGNED_VARIABLE_H