#ifndef ARGUMENT_FUNCTION_H
#define ARGUMENT_FUNCTION_H

#include "variable_with_id.h"

class function : public variable_with_id {
public:
	function(entity_id id)
		:variable_with_id{ id }
	{}

	virtual void visit(instruction_builder* builder) override;
};

#endif // !ARGUMENT_FUNCTION_H