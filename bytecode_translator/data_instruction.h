#ifndef DATA_INSTRUCTION_FILTER_H
#define DATA_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct data_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::data_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.jump_variables.empty();
    }
};

#endif