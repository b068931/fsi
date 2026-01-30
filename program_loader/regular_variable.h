#ifndef REGULAR_VARIABLE_H
#define REGULAR_VARIABLE_H

#include "pch.h"
#include "generic_variable.h"

class regular_variable : public generic_variable {
public:
    regular_variable(std::uint8_t real_type, std::uint8_t active_type, entity_id id)
        :generic_variable{ real_type, active_type, id }
    {}

    void visit(instruction_builder* builder) override;
};

#endif // !REGULAR_VARIABLE_H
