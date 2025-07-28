#ifndef DIFFERENT_TYPE_INSTRUCTION_FILTER_H
#define DIFFERENT_TYPE_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct different_type_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::different_type_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.immediates.empty();
    }
};

#endif