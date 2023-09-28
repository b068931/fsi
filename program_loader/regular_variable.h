#ifndef REGULAR_VARIABLE_H
#define REGULAR_VARIABLE_H

#include "generic_variable.h"
#include <stdint.h>

class regular_variable : public generic_variable {
public:
	regular_variable(uint8_t real_type, uint8_t active_type, entity_id id)
		:generic_variable{ real_type, active_type, id }
	{}

	virtual void visit(instruction_builder* builder) override;
};

#endif // !REGULAR_VARIABLE_H