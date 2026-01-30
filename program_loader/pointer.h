#ifndef ARGUMENT_POINTER_H
#define ARGUMENT_POINTER_H

#include "variable_with_id.h"

class pointer : public variable_with_id {
public:
    pointer(entity_id id)
        :variable_with_id{ id }
    {}

    void visit(instruction_builder* builder) override;
};

#endif // !ARGUMENT_POINTER_H
