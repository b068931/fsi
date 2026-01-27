#ifndef JUMP_INSTRUCTION_FILTER_H
#define JUMP_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct jump_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::jump_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.dereferences.empty() && //jump instructions use only point variables
            instruction.immediates.empty() &&
            instruction.jump_variables.size() == 1 &&
            instruction.operands_in_order.size() == 1;
    }
};

#endif
