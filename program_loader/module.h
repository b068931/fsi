#ifndef ARGUMENT_MODULE_H
#define ARGUMENT_MODULE_H

#include "variable_with_id.h"

class module : public variable_with_id {
public:
	module(entity_id id)
		: variable_with_id{ id }
	{}

	virtual void visit(instruction_builder* builder) override;
};

#endif // !ARGUMENT_MODULE_H