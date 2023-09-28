#ifndef VARIABLE_OBJECT_H
#define VARIABLE_OBJECT_H

//See Instruction Builders to learn more about this class
class instruction_builder;

class variable {
public:
	variable(const variable&) = delete;
	void operator=(const variable&) = delete;

	variable(variable&&) = delete;
	void operator=(variable&&) = delete;

	variable() = default;

	virtual void visit(instruction_builder*) = 0;
	virtual ~variable() = default;
};

#endif // !VARIABLE_OBJECT_H