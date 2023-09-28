#ifndef VARIABLE_IMMEDIATE_H
#define VARIABLE_IMMEDIATE_H

#include "variable.h"

template<typename T>
class variable_imm : public variable {
private:
	T value;

public:
	variable_imm(T value)
		:value{ value },
		variable{}
	{}

	T get_value() const { return this->value; }
	virtual void visit(instruction_builder* builder) override;
};

#endif // !VARIABLE_IMMEDIATE_H