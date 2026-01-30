#ifndef SPECIALIZED_VARIABLE_H
#define SPECIALIZED_VARIABLE_H

#include "variable_with_id.h"

class specialized_variable : public variable_with_id {
public:
    specialized_variable(entity_id id)
        :variable_with_id{ id }
    {}

    void visit(instruction_builder* builder) override;
};

#endif // !SPECIALIZED_VARIABLE_H
