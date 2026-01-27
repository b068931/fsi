#ifndef SAVE_VARIABLE_STATE_INSTRUCTION_FILTER_H
#define SAVE_VARIABLE_STATE_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct save_variable_state_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::save_variable_state_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.operands_in_order.size() == 1;
    }
};

#endif
