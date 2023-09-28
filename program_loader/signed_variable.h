#ifndef SIGNED_VARIABLE_H
#define SIGNED_VARIABLE_H

#include "generic_variable.h"
#include <stdint.h>

class signed_variable : public generic_variable {
public:
	signed_variable(uint8_t real_type, uint8_t active_type, entity_id id)
		:generic_variable{ real_type, active_type, id }
	{}

	virtual void visit(instruction_builder* builder) override;
};

#endif // !SIGNED_VARIABLE_H