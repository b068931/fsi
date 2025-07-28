#ifndef LOAD_VARIABLE_STATE_INSTRUCTION_FILTER_H
#define LOAD_VARIABLE_STATE_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct load_variable_state_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::load_variable_state_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.immediates.empty();
    }
};

#endif