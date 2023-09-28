#ifndef AUXILIARY_VARIABLE_WITH_ID_H
#define AUXILIARY_VARIABLE_WITH_ID_H

#include "variable.h"
#include <stdint.h>

using entity_id = uint64_t;

class variable_with_id : public variable {
private:
	entity_id id;

public:
	variable_with_id(entity_id id)
		:id{ id },
		variable{}
	{}

	entity_id get_id() const { return this->id; }
};

#endif // !AUXILIARY_VARIABLE_WITH_ID_H